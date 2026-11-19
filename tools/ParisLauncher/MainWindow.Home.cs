using System.IO;
using System.Windows;

namespace ParisLauncher;

public partial class MainWindow
{
    private void HomeSupport(object sender, System.Windows.RoutedEventArgs e) => OpenHelpLink(settings.SupportUrl);

    private void SelectClientClick(object sender, System.Windows.RoutedEventArgs e)
    {
        preferences.HasSelectedClient = true;
        RefreshPlayLabel();
    }

    private bool HasBothClients => IsClientInstalled(true) && IsClientInstalled(false);

    private async void HomePrimaryClick(object sender, RoutedEventArgs e)
    {
        if (HasBothClients)
        {
            NavClient.IsChecked = true;
            return;
        }

        if (!HasInstalledClient && !UseDefaultInstallation()) return;
        await RefreshUpdates(both: true);
    }

    private bool HasInstalledClient => IsClientInstalled(true) || IsClientInstalled(false);

    private bool IsClientInstalled(bool classic)
    {
        string executable = settings.Executable(root, classic);
        return File.Exists(executable)
            && File.Exists(Path.Combine(Path.GetDirectoryName(executable)!, "Main.exe"));
    }

    private bool UseDefaultInstallation()
    {
        if (InstallerLocation.IsDefaultDirectory(root)) return true;
        try
        {
            InstallerLocation.OpenDefaultInstallation();
            exitRequested = true;
            Close();
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or System.ComponentModel.Win32Exception)
        {
            UpdateStatus.Text = "No se pudo preparar C:\\PARIS PROJECT. " + error.Message;
        }
        return false;
    }

    private void ShowDownloadPrompt()
    {
        UpdateStatus.Text = @"Instalación en C:\PARIS PROJECT";
        UpdateBar.Value = 0;
        UpdatePercent.Text = "";
        Play.IsEnabled = true;
        RefreshPlayLabel(reset: true);
    }

    private void RefreshPlayLabel(bool reset = false)
    {
        HomePrimary.Content = HasBothClients ? "ELEGIR CLIENTE" : "DESCARGAR";
        HomePrimary.Tag = HasBothClients ? "\uE82D" : "\uE896";

        if (!reset && (Equals(Play.Content, "REINTENTAR") || Equals(Play.Content, "ABRIR OTRO CLIENTE")))
            return;

        bool selectedInstalled = IsClientInstalled(Classic.IsChecked == true);
        Play.Content = !HasInstalledClient || (preferences.HasSelectedClient && !selectedInstalled)
            ? "DESCARGAR"
            : preferences.HasSelectedClient ? "JUGAR" : "ELEGIR CLIENTE";
    }
}
