using System.IO;
using System.Windows;
using System.Security.Cryptography;
using System.Text;

namespace ParisLauncher;

public partial class App : Application
{
    private Mutex? instance;
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);
        try
        {
            if (InstallerLocation.RelocateFreshDownload()) { Shutdown(); return; }
            string identity = Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Path.GetFullPath(AppContext.BaseDirectory).ToUpperInvariant())));
            instance = new Mutex(true, "Local\\ParisProjectLauncher-" + identity, out bool created);
            if (!created)
            {
                MessageBox.Show("El launcher de esta instalación ya está abierto.", "Paris Project");
                Shutdown();
                return;
            }
            new MainWindow().Show();
        }
        catch (Exception error)
        {
            MessageBox.Show(error.Message, "Paris Project", MessageBoxButton.OK, MessageBoxImage.Warning);
            Shutdown(1);
        }
    }

    protected override void OnExit(ExitEventArgs e)
    {
        instance?.Dispose();
        base.OnExit(e);
    }
}
