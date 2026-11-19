using System.Net.Http;
using System.Windows;
using System.Windows.Input;
using ParisLauncher.Account;

namespace ParisLauncher.Views;

public partial class AccountWindow : Window
{
    private readonly AccountClient accounts;
    private bool registering;
    public LauncherSession? Session { get; private set; }
    public bool Registered { get; private set; }
    public AccountWindow(string api)
    {
        accounts = new AccountClient(api);
        InitializeComponent();
        Width = Math.Min(Width, SystemParameters.WorkArea.Width - 30);
        Height = Math.Min(Height, SystemParameters.WorkArea.Height - 30);
        Closed += (_, _) => accounts.Dispose();
    }
    private async void LoadedAccount(object sender, RoutedEventArgs e)
    {
        var saved = SessionStorage.Load();
        if (saved is null) return;
        Submit.IsEnabled = false;
        ErrorMessage.Text = "Comprobando sesión guardada…";
        try { if (await accounts.Validate(saved)) { Session = saved; DialogResult = true; } else SessionStorage.Clear(); }
        catch (Exception error) when (error is HttpRequestException or TaskCanceledException) { ErrorMessage.Text = "No se pudo comprobar la sesión. Revisa tu conexión."; }
        finally { Submit.IsEnabled = true; if (Session is null) ErrorMessage.Text = "Inicia sesión para continuar."; }
    }
    private void DragTitle(object sender, MouseButtonEventArgs e) { if (e.ChangedButton == MouseButton.Left) DragMove(); }
    private void CloseAccount(object sender, RoutedEventArgs e) => Close();
    private void MinimizeAccount(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;
    private void ToggleMode(object sender, RoutedEventArgs e)
    {
        registering = !registering;
        Heading.Text = registering ? "Crear cuenta" : "Inicia sesión";
        Submit.Content = registering ? "CREAR CUENTA" : "INICIAR SESIÓN";
        SwitchMode.Content = registering ? "Ya tengo una cuenta" : "¿No tienes cuenta? Crear cuenta";
        EmailPanel.Visibility = ConfirmPanel.Visibility = registering ? Visibility.Visible : Visibility.Collapsed;
        Password.Clear(); ConfirmPassword.Clear(); ErrorMessage.Text = "";
    }
    private async void SubmitAccount(object sender, RoutedEventArgs e)
    {
        if (registering && AcceptTerms.IsChecked != true) { ErrorMessage.Text = "Debes aceptar las reglas para crear tu cuenta."; return; }
        if (registering && Password.Password != ConfirmPassword.Password) { ErrorMessage.Text = "Las contraseñas no coinciden."; return; }
        Submit.IsEnabled = SwitchMode.IsEnabled = false; ErrorMessage.Text = "Conectando…";
        try
        {
            if (registering)
            {
                await accounts.Register(LoginName.Text, Password.Password, Email.Text);
                Registered = true; registering = false;
                Heading.Text = "Cuenta creada · inicia sesión";
                Submit.Content = "INICIAR SESIÓN";
                EmailPanel.Visibility = ConfirmPanel.Visibility = Visibility.Collapsed;
                SwitchMode.Content = "¿No tienes cuenta? Crear cuenta";
            }
            Session = await accounts.Login(LoginName.Text, Password.Password, RememberSession.IsChecked == true);
            if (RememberSession.IsChecked == true) SessionStorage.Save(Session); else SessionStorage.Clear();
            Password.Clear(); ConfirmPassword.Clear(); DialogResult = true;
        }
        catch (Exception error) when (error is HttpRequestException or TaskCanceledException or InvalidOperationException or System.IO.IOException)
        { ErrorMessage.Text = error is HttpRequestException or TaskCanceledException ? "No se pudo contactar al servidor de cuentas. Reintenta." : error.Message; }
        finally { Submit.IsEnabled = SwitchMode.IsEnabled = true; }
    }
}
