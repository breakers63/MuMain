using System.IO;
using System.Windows;
using System.Security.Cryptography;
using System.Text;
using ParisLauncher.Core;
using ParisLauncher.Views;

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
            ShowAccountFlow();
        }
        catch (Exception error)
        {
            MessageBox.Show(error.Message, "Paris Project", MessageBoxButton.OK, MessageBoxImage.Warning);
            Shutdown(1);
        }
    }

    public void ShowAccountFlow()
    {
        ShutdownMode = ShutdownMode.OnExplicitShutdown;
        var settings = LauncherSettings.Load(AppContext.BaseDirectory);
        var account = new AccountWindow(settings.AccountsApiUrl);
        if (account.ShowDialog() != true || account.Session is null) { Shutdown(); return; }
        string goal = "home";
        if (account.Registered)
        {
            var welcome = new WelcomeWindow();
            welcome.ShowDialog();
            goal = welcome.Goal;
        }
        var main = new MainWindow();
        main.SetAccount(account.Session, goal);
        MainWindow = main;
        ShutdownMode = ShutdownMode.OnMainWindowClose;
        main.Show();
    }

    protected override void OnExit(ExitEventArgs e)
    {
        instance?.Dispose();
        base.OnExit(e);
    }
}
