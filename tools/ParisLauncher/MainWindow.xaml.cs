using System.ComponentModel;
using System.IO;
using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using Microsoft.Win32;
using ParisLauncher.Core;

namespace ParisLauncher;

public partial class MainWindow : Window
{
    private readonly string root = AppContext.BaseDirectory;
    private readonly LauncherSettings settings;
    private readonly PlayerPreferences preferences;
    private readonly HttpClient http = new(new HttpClientHandler { AllowAutoRedirect = false }) { Timeout = TimeSpan.FromMinutes(20) };
    private readonly SemaphoreSlim updateGate = new(1);
    private CancellationTokenSource? updateCancellation;
    private bool initialized;
    private bool ready;
    private long lastProgress;

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
        UpdateEngineState();
    }

    private async void WindowLoaded(object sender, RoutedEventArgs e)
    {
        await DetectGraphics();
        await RefreshUpdates();
        if (ready && preferences.Remember && !Environment.GetCommandLineArgs().Contains("--choose") && (Keyboard.Modifiers & ModifierKeys.Shift) == 0)
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
    }

    private void ModeChanged(object sender, RoutedEventArgs e)
    {
        if (!initialized) return;
        UpdateEngineState();
    }

    private void UpdateEngineState()
    {
        Engines.IsEnabled = Classic.IsChecked != true;
        EngineNote.Text = Classic.IsChecked == true ? "Clásico utiliza OpenGL; no requiere elegir otro motor." : "Vulkan y DirectX 12 utilizan el mismo cliente Moderno.";
    }

    private void EnsureClientClosed()
    {
        if (ClientProcess.IsRunning(root, settings)) throw new IOException("Cierra el cliente abierto y pulsa REINTENTAR.");
    }

    private async Task RefreshUpdates()
    {
        updateCancellation?.Cancel();
        var cancellation = new CancellationTokenSource();
        updateCancellation = cancellation;
        ready = false;
        Play.IsEnabled = false;
        Play.Content = "JUGAR";
        await updateGate.WaitAsync();
        try
        {
            cancellation.Token.ThrowIfCancellationRequested();
            var progress = new Progress<UpdateProgress>(p =>
            {
                if (cancellation.IsCancellationRequested) return;
                long now = Environment.TickCount64;
                if (now - lastProgress < 100 && !p.Indeterminate && p.Percent < 95) return;
                lastProgress = now;
                UpdateStatus.Text = p.Message;
                UpdateBar.IsIndeterminate = p.Indeterminate;
                UpdateBar.Value = p.Percent;
                UpdatePercent.Text = p.Indeterminate ? "" : $"{p.Percent:0}%";
            });
            var service = new UpdateService(http, root, EnsureClientClosed);
            string status = await service.RunAsync(settings.ManifestUrl, Classic.IsChecked == true, progress, cancellation.Token, both: true);
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
                Play.Content = "REINTENTAR";
                Play.IsEnabled = true;
            }
        }
        finally
        {
            updateGate.Release();
            cancellation.Dispose();
            if (ReferenceEquals(updateCancellation, cancellation)) updateCancellation = null;
        }
    }

    private static string DescribeUpdateError(Exception error) => error switch
    {
        HttpRequestException { StatusCode: System.Net.HttpStatusCode.NotFound } => "No se encontró el manifiesto de actualización (404).",
        HttpRequestException => "No se pudo conectar al servidor de actualizaciones. Reintenta.",
        _ => error.Message,
    };

    private async void PlayClick(object sender, RoutedEventArgs e)
    {
        if (!ready) { await RefreshUpdates(); return; }
        LaunchSelected();
    }

    private void LaunchSelected()
    {
        try
        {
            preferences.Classic = Classic.IsChecked == true;
            preferences.Backend = DirectX.IsChecked == true ? "direct3d12" : "vulkan";
            preferences.Remember = Remember.IsChecked == true;
            preferences.Save();
            ClientProcess.Start(root, settings, preferences.Classic, preferences.Backend);
            Close();
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
            Close();
        }
        catch (Exception error) { MessageBox.Show(this, error.Message, "Paris Project", MessageBoxButton.OK, MessageBoxImage.Information); }
    }

    private void DragTitle(object sender, MouseButtonEventArgs e) { if (e.ChangedButton == MouseButton.Left && e.Source is not System.Windows.Controls.Button) DragMove(); }
    private void MinimizeClick(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;
    private void ExitClick(object sender, RoutedEventArgs e) => Close();
    private void WindowClosing(object? sender, CancelEventArgs e) => updateCancellation?.Cancel();
}
