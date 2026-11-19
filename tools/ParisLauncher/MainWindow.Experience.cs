using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using ParisLauncher.Account;
using ParisLauncher.Performance;

namespace ParisLauncher;

public partial class MainWindow
{
    private LauncherSession? accountSession;
    private string performanceProfile = "Equilibrado";
    private readonly Queue<double> latencyHistory = new();
    public void SetAccount(LauncherSession session, string goal)
    {
        accountSession = session;
        AccountName.Text = session.LoginName;
        _ = RefreshAccountProfile();
        StartProfileRefresh();
        if (goal == "client") NavClient.IsChecked = true;
        else if (goal == "network") NavNetwork.IsChecked = true;
        else if (goal == "performance") NavPerformance.IsChecked = true;
    }
    private void SelectProfile(object sender, RoutedEventArgs e)
    {
        performanceProfile = ((Button)sender).Tag.ToString() ?? "Equilibrado";
        ShadowsOption.IsChecked = performanceProfile != "Rendimiento";
        EffectsOption.IsChecked = performanceProfile != "Rendimiento";
        WeatherOption.IsChecked = performanceProfile == "Calidad";
        FpsOption.IsChecked = false;
        EnvironmentOption.IsChecked = performanceProfile != "Rendimiento";
        VsyncOption.IsChecked = performanceProfile != "Rendimiento";
        PerformanceStatus.Text = "Perfil seleccionado: " + performanceProfile;
    }
    private void RefreshGraphicsAvailability()
    {
        if (FpsOption is null) return;
        bool modern = Classic.IsChecked != true;
        foreach (var control in new[] { ShadowsOption, EffectsOption, EnvironmentOption, FpsOption, VsyncOption })
        {
            control.IsEnabled = modern;
            control.Opacity = modern ? 1 : 0.4;
            control.ToolTip = modern ? "Se guarda en config.ini del cliente moderno." : "Este control no se aplica a OpenGL.";
        }
        GraphicsClientLabel.Text = modern ? "GRÁFICOS · VULKAN / DIRECTX 12" : "GRÁFICOS · OPENGL (CLÁSICO)";

    }
    private IniProfile SelectedProfile() => new(Path.GetDirectoryName(settings.Executable(root, Classic.IsChecked == true))!);
    private void PlayClassicCard(object sender, RoutedEventArgs e) { Classic.IsChecked = true; preferences.HasSelectedClient = true; RefreshPlayLabel(); PlayClick(sender, e); }
    private void PlayModernCard(object sender, RoutedEventArgs e) { Modern.IsChecked = true; preferences.HasSelectedClient = true; RefreshPlayLabel(); PlayClick(sender, e); }
    private async void VerifyFiles(object sender, RoutedEventArgs e)
    {
        if (string.IsNullOrWhiteSpace(settings.ManifestUrl))
        {
            ShowInstalledClients();
            UpdateStatus.Text = "Revisión local: archivos presentes comprobados. La verificación por SHA-256 requiere el manifiesto de publicación.";
            return;
        }
        await RefreshUpdates();
        ShowInstalledClients();
    }
    private void ShowInstalledClients()
    {
        void Show(TextBlock label, Button action, bool classic)
        {
            bool present = IsClientInstalled(classic);
            action.Content = present ? "▶  JUGAR" : "DESCARGAR";
            label.Text = present ? "✓  Instalado · archivos presentes" : "Pendiente de instalación";
            label.Foreground = new SolidColorBrush(present ? Color.FromRgb(137, 203, 97) : Color.FromRgb(214, 171, 80));
        }
        Show(ClassicInstall, ClassicAction, true);
        Show(ModernInstall, ModernAction, false);
        RefreshPlayLabel();
    }
    private void ApplyPerformance(object sender, RoutedEventArgs e)
    {
        try
        {
            EnsureClientClosed();
            SelectedProfile().Apply(Classic.IsChecked == true, performanceProfile, ShadowsOption.IsChecked == true, EffectsOption.IsChecked == true, WeatherOption.IsChecked == true, FpsOption.IsChecked == true, EnvironmentOption.IsChecked == true, VsyncOption.IsChecked == true);
            PerformanceStatus.Text = "Perfil guardado. Se aplica al abrir el cliente seleccionado. Puedes restaurar los valores anteriores.";
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException) { PerformanceStatus.Text = error.Message; }
    }
    private void RestorePerformance(object sender, RoutedEventArgs e)
    {
        try { EnsureClientClosed(); SelectedProfile().Restore(); PerformanceStatus.Text = "Configuración anterior restaurada."; }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException) { PerformanceStatus.Text = error.Message; }
    }
    private async void EnablePower(object sender, RoutedEventArgs e)
    {
        try { await PowerProfile.Enable(); PerformanceStatus.Text = "Plan de alto rendimiento activado. Puede consumir más batería y energía."; }
        catch (IOException error) { PerformanceStatus.Text = error.Message; }
    }
    private async void RestorePower(object sender, RoutedEventArgs e)
    {
        try { await PowerProfile.Restore(); PerformanceStatus.Text = "Plan de energía anterior restaurado."; }
        catch (IOException error) { PerformanceStatus.Text = error.Message; }
    }
    private void ShowGuide(object sender, RoutedEventArgs e) => new Views.WelcomeWindow { Owner = this }.ShowDialog();
    private void MaximizeWindow(object sender, RoutedEventArgs e) => WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
    private async void DiagnoseNetwork(object sender, RoutedEventArgs e)
    {
        try { NetworkDiagnostic.Text = await NetworkDiagnostics.Describe(settings.ServerHost); }
        catch (System.Net.Sockets.SocketException) { NetworkDiagnostic.Text = "No se pudo resolver el servidor. Revisa tu conexión y DNS."; }
    }
    private async void SignOut(object sender, RoutedEventArgs e)
    {
        if (accountSession is null) { Close(); return; }
        try { using var api = new AccountClient(settings.AccountsApiUrl); await api.Logout(accountSession); }
        catch (Exception error) when (error is System.Net.Http.HttpRequestException or TaskCanceledException or InvalidOperationException) { MessageBox.Show(this, "No se pudo cerrar la sesión en el servidor. Reintenta con conexión.", "PARIS PROJECT"); return; }
        SessionStorage.Clear();
        Application.Current.ShutdownMode = ShutdownMode.OnExplicitShutdown;
        exitRequested=true;Close();
        ((App)Application.Current).ShowAccountFlow();
    }
    private void PlotLatency(double value)
    {
        latencyHistory.Enqueue(value);
        while (latencyHistory.Count > 120) latencyHistory.Dequeue();
        double ceiling = Math.Max(100, latencyHistory.Max() * 1.2);
        ChartMaximum.Text = $"{ceiling:0} ms";
        ChartLegend.Text = $"Prom. {latencyHistory.Average():0.0} ms · Mín. {latencyHistory.Min():0.0} · Máx. {latencyHistory.Max():0.0} · {latencyHistory.Count} muestras TCP · {DateTime.Now:HH:mm:ss}";
        var points = new PointCollection();
        int index = 0;
        foreach (double sample in latencyHistory) points.Add(new Point(index++ * 760.0 / Math.Max(1, latencyHistory.Count - 1), 150 - sample * 140 / ceiling));
        LatencyLine.Points = points;
    }
}
