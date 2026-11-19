using System.ComponentModel;
using System.IO;
using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using Microsoft.Win32;
using ParisLauncher.Core;

namespace ParisLauncher;

public partial class MainWindow : Window
{
    private readonly string root = AppContext.BaseDirectory;
    private readonly LauncherSettings settings;
    private readonly PlayerPreferences preferences;
    private readonly HttpClient http = new(new HttpClientHandler { AllowAutoRedirect = false })
    {
        Timeout = TimeSpan.FromMinutes(20),
        DefaultRequestVersion = System.Net.HttpVersion.Version20,
        DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrLower,
    };
    private readonly SemaphoreSlim updateGate = new(1);
    private CancellationTokenSource? updateCancellation;
    private bool initialized;
    private bool ready;
    private const int ProgressRefreshMilliseconds = 150;
    private const int LatencySamples = 8;
    private CancellationTokenSource? latencyCancellation;
    private System.Windows.Threading.DispatcherTimer? latencyTimer;
    private bool measuring;

    public MainWindow()
    {
        settings = LauncherSettings.Load(root);
        preferences = PlayerPreferences.Load();
        http.DefaultRequestHeaders.UserAgent.ParseAdd("ParisProjectLauncher/1.0");
        http.DefaultRequestHeaders.CacheControl = new System.Net.Http.Headers.CacheControlHeaderValue { NoCache = true };
        InitializeComponent();
        Width = Math.Min(Width, SystemParameters.WorkArea.Width - 30);
        Height = Math.Min(Height, SystemParameters.WorkArea.Height - 30);
        MinHeight = Math.Min(MinHeight, Height);
        MinWidth = Math.Min(MinWidth, Width);
        Classic.IsChecked = preferences.Classic;
        Modern.IsChecked = !preferences.Classic;
        Vulkan.IsChecked = preferences.Backend != "direct3d12";
        DirectX.IsChecked = preferences.Backend == "direct3d12";
        Remember.IsChecked = preferences.Remember;
        initialized = true;
        ConfigureGeneralSettings();
        UpdateEngineState();
        RefreshPlayLabel();
        ShowPanel();
        ServerAddress.Text = $"Servidor: {settings.ServerHost}:{settings.ServerPort}";
        LauncherVersion.Text = "Launcher v" + (typeof(MainWindow).Assembly.GetName().Version?.ToString(3) ?? "");
    }

    private async void WindowLoaded(object sender, RoutedEventArgs e)
    {
        if(general.CloseToTray)EnsureTray();
        StartLatencyMonitor();
        await DetectGraphics();
        ShowInstalledClients();
        if (HasInstalledClient && !UseRunningInstallation())
            await RefreshUpdates(classicClient: IsClientInstalled(true), both: IsClientInstalled(true) && IsClientInstalled(false));
        else if (!HasInstalledClient) ShowDownloadPrompt();
        ShowInstalledClients();
        if (ready && IsClientInstalled(Classic.IsChecked == true) && preferences.Remember && !Environment.GetCommandLineArgs().Contains("--choose") && (Keyboard.Modifiers & ModifierKeys.Shift) == 0)
            LaunchSelected();
    }

    private async Task DetectGraphics()
    {
        try
        {
            var cards = await Task.Run(GpuDetector.Detect);
            GpuName.Text = cards.Count == 0 ? "No se pudo identificar la GPU" : string.Join(" / ", cards.Select(c => c.Name));
            GpuMemory.Text = cards.Count == 0 ? "Selecciona el motor gráfico que prefieras." : cards[0].MemoryLabel;
        }
        catch (Exception error) when (error is System.Runtime.InteropServices.COMException or DllNotFoundException or EntryPointNotFoundException)
        {
            GpuName.Text = "GPU no identificada";
            GpuMemory.Text = "Puedes seleccionar el motor gráfico manualmente.";
        }
        HomeGpu.Text = GpuName.Text;
    }

    private void ModeChanged(object sender, RoutedEventArgs e)
    {
        if (!initialized) return;
        preferences.HasSelectedClient = true;
        RefreshPlayLabel();
        UpdateEngineState();
    }

    private void UpdateEngineState()
    {
        Engines.IsEnabled = Classic.IsChecked != true;
        RefreshGraphicsAvailability();
        EngineNote.Text = Classic.IsChecked == true ? "Clásico utiliza OpenGL; no requiere elegir otro motor." : "Vulkan y DirectX 12 utilizan el mismo cliente Moderno.";
        HomeMode.Text = Classic.IsChecked == true ? "Clásico · OpenGL" : DirectX.IsChecked == true ? "Moderno · DirectX 12" : "Moderno · Vulkan";
    }

    private void EnsureClientClosed()
    {
        if (ClientProcess.IsRunning(root, settings)) throw new IOException("Cierra el cliente abierto y pulsa REINTENTAR.");
    }

    private bool UseRunningInstallation()
    {
        if (!ClientProcess.IsRunning(root, settings)) return false;
        ready = true;
        Play.IsEnabled = true;
        Play.Content = "ABRIR OTRO CLIENTE";
        UpdateStatus.Text = "Cliente abierto · Puedes abrir otra ventana. Las actualizaciones se aplicarán cuando cierres los clientes.";
        UpdateBar.IsIndeterminate = false;
        UpdateBar.Value = 100;
        UpdatePercent.Text = "";
        return true;
    }

    private (bool Classic, bool Both)? failedUpdate;

    private async Task RefreshUpdates(bool? classicClient = null, bool? both = null)
    {
        updateCancellation?.Cancel();
        var cancellation = new CancellationTokenSource();
        updateCancellation = cancellation;
        ready = false;
        Play.IsEnabled = false;
        HomePrimary.IsEnabled = ClassicAction.IsEnabled = ModernAction.IsEnabled = false;
        bool classic = classicClient ?? (Classic.IsChecked == true);
        bool updateBoth = both ?? HasBothClients;
        failedUpdate = null;
        RefreshPlayLabel(reset: true);
        await updateGate.WaitAsync();
        try
        {
            cancellation.Token.ThrowIfCancellationRequested();
            var progress = new LatestProgress();
            var timer = new System.Windows.Threading.DispatcherTimer(System.Windows.Threading.DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(ProgressRefreshMilliseconds)
            };
            timer.Tick += (_, _) =>
            {
                if (!cancellation.IsCancellationRequested && progress.Take() is { } value) ApplyUpdateProgress(value);
            };
            string manifestUrl = settings.ManifestUrl;
            if (string.IsNullOrWhiteSpace(manifestUrl) && (updateBoth ? !HasBothClients : !IsClientInstalled(classic)))
                throw new InvalidDataException("No hay un manifiesto configurado para descargar este cliente.");
            var service = new UpdateService(http, root, EnsureClientClosed);
            string status;
            timer.Start();
            try
            {
                status = await Task.Run(() => service.RunAsync(manifestUrl, classic, progress, cancellation.Token, both: updateBoth));
            }
            finally { timer.Stop(); }
            cancellation.Token.ThrowIfCancellationRequested();
            UpdateStatus.Text = status;
            UpdateBar.IsIndeterminate = false;
            UpdateBar.Value = 100;
            UpdatePercent.Text = "100%";
            ready = true;
            Play.IsEnabled = true;
            if (!string.IsNullOrWhiteSpace(settings.ManifestUrl))
            {
                try { DesktopShortcut.Create(root); }
                catch (Exception error) when (error is IOException or UnauthorizedAccessException or System.Runtime.InteropServices.COMException)
                {
                    UpdateStatus.Text += " · No se pudo crear el acceso directo.";
                }
            }
        }
        catch (OperationCanceledException) { }
        catch (Exception error)
        {
            if (!cancellation.IsCancellationRequested)
            {
                UpdateStatus.Text = DescribeUpdateError(error);
                UpdateBar.IsIndeterminate = false;
                UpdatePercent.Text = "";
                failedUpdate = (classic, updateBoth);
                Play.Content = "REINTENTAR";
                Play.IsEnabled = true;
            }
        }
        finally
        {
            if (ReferenceEquals(updateCancellation, cancellation))
            {
                HomePrimary.IsEnabled = ClassicAction.IsEnabled = ModernAction.IsEnabled = true;
                ShowInstalledClients();
            }
            updateGate.Release();
            cancellation.Dispose();
            if (ReferenceEquals(updateCancellation, cancellation)) updateCancellation = null;
        }
    }

    private void ApplyUpdateProgress(UpdateProgress progress)
    {
        UpdateStatus.Text = progress.Message;
        UpdateBar.IsIndeterminate = progress.Indeterminate;
        UpdateBar.Value = progress.Percent;
        UpdatePercent.Text = progress.Indeterminate ? "" : $"{progress.Percent:0}%";
    }

    private static string DescribeUpdateError(Exception error) => error switch
    {
        HttpRequestException { StatusCode: System.Net.HttpStatusCode.NotFound } => "No se encontró el manifiesto de actualización (404).",
        HttpRequestException => "No se pudo conectar al servidor de actualizaciones. Reintenta.",
        _ => error.Message,
    };

    private async void PlayClick(object sender, RoutedEventArgs e)
    {
        if (failedUpdate is { } retry)
        {
            await RefreshUpdates(classicClient: retry.Classic, both: retry.Both);
            return;
        }
        if (!preferences.HasSelectedClient)
        {
            NavClient.IsChecked = true;
            return;
        }
        if (!ready || !IsClientInstalled(Classic.IsChecked == true))
        {
            if (!HasInstalledClient && !UseDefaultInstallation()) return;
            await RefreshUpdates();
            return;
        }
        LaunchSelected();
    }

    private void LaunchSelected()
    {
        try
        {
            preferences.HasSelectedClient = true;
            preferences.Classic = Classic.IsChecked == true;
            preferences.Backend = DirectX.IsChecked == true ? "direct3d12" : "vulkan";
            preferences.Remember = Remember.IsChecked == true;
            preferences.Save();
            ClientProcess.Start(root, settings, preferences.Classic, preferences.Backend);
            if(general.ConnectionAlerts){EnsureTray();tray?.Notify("MU Paris","Se abrió el cliente seleccionado.");}
            WindowState = WindowState.Minimized;
        }
        catch (Exception error) { MessageBox.Show(this, error.Message, "Paris Project", MessageBoxButton.OK, MessageBoxImage.Information); }
    }

    private void ChooseClassic(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog { Title = "Selecciona Jugar.exe o Main.exe del cliente Clásico", Filter = "Cliente MU (*.exe)|*.exe", CheckFileExists = true };
        if (dialog.ShowDialog(this) != true) return;
        if (string.Equals(dialog.FileName, Environment.ProcessPath, StringComparison.OrdinalIgnoreCase)) return;
        settings.ClassicExecutable = dialog.FileName;
        settings.Save(root);
        UpdateStatus.Text = "Cliente Clásico configurado.";
    }

    private void ChooseInstallFolder(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFolderDialog { Title = "Elige la carpeta de Paris Project", InitialDirectory = root };
        if (dialog.ShowDialog(this) != true) return;
        string destination = Path.GetFullPath(dialog.FolderName);
        if (string.Equals(destination.TrimEnd(Path.DirectorySeparatorChar), root.TrimEnd(Path.DirectorySeparatorChar), StringComparison.OrdinalIgnoreCase)) return;
        try
        {
            if (ClientProcess.IsRunning(destination, settings)) throw new IOException("Cierra el cliente de esa carpeta antes de instalar.");
            Directory.CreateDirectory(destination);
            string launcher = Path.Combine(destination, "Launcher.exe");
            File.Copy(Path.Combine(root, "Launcher.exe"), launcher, overwrite: true);
            if (!File.Exists(Path.Combine(destination, "launcher.json"))) new LauncherSettings().Save(destination);
            System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(launcher, "--choose") { WorkingDirectory = destination, UseShellExecute = true });
            exitRequested=true;Close();
        }
        catch (Exception error) { MessageBox.Show(this, error.Message, "Paris Project", MessageBoxButton.OK, MessageBoxImage.Information); }
    }

    private void DragTitle(object sender, MouseButtonEventArgs e) { if (e.ChangedButton == MouseButton.Left && e.Source is not System.Windows.Controls.Button) DragMove(); }
    private void MinimizeClick(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;
    private void ExitClick(object sender, RoutedEventArgs e) { exitRequested=true;Close(); }
    private void WindowClosing(object? sender, CancelEventArgs e)
    {
        if(general.CloseToTray&&!exitRequested&&HideToTray()){e.Cancel=true;return;}
        updateCancellation?.Cancel();
        latencyTimer?.Stop();
        latencyCancellation?.Cancel();
    }

    // ---------- Navegación ----------
    private void NavChanged(object sender, RoutedEventArgs e) => ShowPanel();

    private void ShowPanel()
    {
        if (TrafficPanel is null || BoostPanel is null || HomePanel is null || ClientPanel is null || VpnPanel is null || NetworkPanel is null || SettingsPanel is null || PerformancePanel is null) return;
        HomePanel.Visibility = NavHome.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        ClientPanel.Visibility = NavClient.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        NetworkPanel.Visibility = NavNetwork.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        VpnPanel.Visibility = NavVpn.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        TrafficPanel.Visibility=NavTraffic.IsChecked==true?Visibility.Visible:Visibility.Collapsed;
        SettingsPanel.Visibility = NavSettings.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        NetworkWorkbench.ServerHost = settings.ServerHost; NetworkWorkbench.ServerPort = settings.ServerPort;
        BoostPanel.Executables = new[] { System.IO.Path.Combine(System.IO.Path.GetDirectoryName(settings.Executable(root, true))!, "Main.exe"), System.IO.Path.Combine(System.IO.Path.GetDirectoryName(settings.Executable(root, false))!, "Main.exe") };
        BoostPanel.Visibility = NavBoost.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        PerformancePanel.Visibility = NavPerformance.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
    }

    private void GoClient(object sender, RoutedEventArgs e) => NavClient.IsChecked = true;

    private async void GoNetwork(object sender, RoutedEventArgs e)
    {
        NavNetwork.IsChecked = true;
        await MeasureServer();
    }

    private void OpenWebsite(object sender, RoutedEventArgs e)
    {
        try { System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(settings.WebsiteUrl) { UseShellExecute = true }); }
        catch (Exception error) when (error is System.ComponentModel.Win32Exception or InvalidOperationException)
        {
            MessageBox.Show(this, "No se pudo abrir el navegador.", "Paris Project", MessageBoxButton.OK, MessageBoxImage.Information);
        }
    }

    // ---------- Ping real al servidor ----------
    private void StartLatencyMonitor()
    {
        if (latencyCancellation is not null) return;
        latencyCancellation = new CancellationTokenSource();
        latencyTimer = new System.Windows.Threading.DispatcherTimer { Interval = TimeSpan.FromSeconds(60) };
        latencyTimer.Tick += async (_, _) => await MeasureServer();
        latencyTimer.Start();
        _ = MeasureServer();
    }

    private async void MeasureClick(object sender, RoutedEventArgs e) => await MeasureServer();

    private async Task MeasureServer()
    {
        var cancellation = latencyCancellation;
        if (measuring || cancellation is null || cancellation.IsCancellationRequested) return;
        measuring = true;
        MeasureButton.IsEnabled = false;
        NetVerdict.Text = "Midiendo…";
        try
        {
            var result = await LatencyProbe.MeasureAsync(networkHost ?? settings.ServerHost, networkHost is null ? settings.ServerPort : 443, networkHost is null ? LatencySamples : 3, cancellation.Token);
            ShowLatency(result);
        }
        catch (OperationCanceledException) { }
        finally
        {
            measuring = false;
            MeasureButton.IsEnabled = true;
        }
    }

    private void ShowLatency(LatencyResult result)
    {
        RecordNetwork(result);
        if(networkHost is null)NotifyServerStatus(result.Received>0);
        var (label, color) = Classify(result);
        var brush = new SolidColorBrush((Color)ColorConverter.ConvertFromString(color));
        double? median = result.MedianMs;
        string medianText = median.HasValue ? median.Value.ToString("0") : "—";

        NetPing.Text = medianText;
        if (result.Samples.Count > 0) { foreach (double sample in result.Samples) PlotLatency(sample); }
        else if (median.HasValue) PlotLatency(median.Value);
        NetJitter.Text = result.JitterMs.HasValue ? result.JitterMs.Value.ToString("0.0") : "—";
        NetLoss.Text = result.LossPercent.ToString("0");
        NetRange.Text = result.MinMs.HasValue && result.MaxMs.HasValue ? $"{result.MinMs.Value:0}–{result.MaxMs.Value:0}" : "—";

        if (networkHost is null) {
        ServerDot.Fill = brush;
        ServerState.Text = result.Received == 0 ? "Sin respuesta" : "En línea";
        ServerPing.Text = median.HasValue ? $"{medianText} ms · {label}" : "—";
        HomePing.Text = median.HasValue ? $"{medianText} ms" : "—";
        HomePing.Foreground = brush;
        HomeVerdict.Text = result.Received == 0 ? "No se pudo contactar al servidor" : $"Conexión {label.ToLowerInvariant()}";

        }
        NetVerdict.Foreground = brush;
        NetVerdict.Text = result.Received == 0
            ? "No se pudo contactar al servidor. Revisa tu internet o el estado del servidor."
            : $"Conexión {label.ToLowerInvariant()} · {result.Received}/{result.Sent} respuestas · medido a las {DateTime.Now:HH:mm}";
    }

    private static (string Label, string Color) Classify(LatencyResult result)
    {
        if (!result.MedianMs.HasValue) return ("Sin respuesta", "#E0645A");
        if (result.LossPercent > 2 || (result.JitterMs ?? 0) > 20) return ("Inestable", "#E8A04A");
        double ms = result.MedianMs.Value;
        if (ms < 60) return ("Excelente", "#6FD08C");
        if (ms < 100) return ("Buena", "#B9D46A");
        if (ms < 150) return ("Aceptable", "#E8C15A");
        return ("Alta", "#E0645A");
    }
}
