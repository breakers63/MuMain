using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;
using ParisLauncher.Account;
namespace ParisLauncher;
public partial class MainWindow
{
    private AccountProfile? profile;
    private Grid? headerOverlay;
    private DispatcherTimer? profileTimer;
    private async Task RefreshAccountProfile()
    {
        if(accountSession is null||string.IsNullOrEmpty(accountSession.Token))return;
        try{using var api=new AccountClient(settings.AccountsApiUrl);profile=await api.Profile(accountSession);VipStatus.Text=profile.Status;VipStatus.Foreground=new SolidColorBrush(profile.VipTier>0?Color.FromRgb(231,190,92):Color.FromRgb(135,192,175));}
        catch(Exception error) when(error is System.Net.Http.HttpRequestException or TaskCanceledException or InvalidOperationException){profile=null;VipStatus.Text="Estado no disponible";VipStatus.ToolTip="No se pudo consultar el estado de la cuenta. Reintenta desde Perfil.";}
        VpnPanel.UpdateAccess(profile?.VipTier);
    }
    private void StartProfileRefresh()
    {
        if(profileTimer is not null)return;profileTimer=new DispatcherTimer{Interval=TimeSpan.FromMinutes(3)};profileTimer.Tick+=async(_,_)=>await RefreshAccountProfile();profileTimer.Start();Closed+=(_,_)=>profileTimer.Stop();
    }
    private async void OpenAccountProfile(object sender,RoutedEventArgs e)
    {
        await RefreshAccountProfile();
        var content=new StackPanel();
        content.Children.Add(HeaderCard("Mis datos",$"Usuario: {accountSession?.LoginName??"—"}\n\nCorreo electrónico: {profile?.Email??"No disponible"}"));
        content.Children.Add(MembershipCard());
        OpenHeaderDrawer("Perfil",content);
    }
    private void OpenHelpCenter(object sender,RoutedEventArgs e)
    {
        OpenHelpCards();
    }
    private void OpenNotifications(object sender,RoutedEventArgs e)
    {
        var content=new StackPanel();var mark=HeaderButton("Marcar todo como leído");var empty=HeaderCard("Bandeja de entrada","No tienes notificaciones nuevas.");mark.Click+=(_,_)=>mark.IsEnabled=false;content.Children.Add(mark);content.Children.Add(empty);OpenHeaderDrawer("Bandeja de entrada",content);
    }
    private void BeginHeaderSearch(object sender,KeyboardFocusChangedEventArgs e){if(HeaderSearch.Text=="Buscar…")HeaderSearch.Clear();}
    private void EndHeaderSearch(object sender,KeyboardFocusChangedEventArgs e){if(string.IsNullOrWhiteSpace(HeaderSearch.Text))HeaderSearch.Text="Buscar…";}
    private void SearchHeader(object sender,KeyEventArgs e)
    {
        if(e.Key!=Key.Enter)return;string query=HeaderSearch.Text.Trim().ToLowerInvariant();
        if(query.Contains("dns")||query.Contains("ram")||query.Contains("boost"))NavBoost.IsChecked=true;
        else if(query.Contains("vpn"))NavVpn.IsChecked=true;
        else if(query.Contains("red")||query.Contains("ping")||query.Contains("conex"))NavNetwork.IsChecked=true;
        else if(query.Contains("biblioteca")||query.Contains("cliente")||query.Contains("jugar"))NavClient.IsChecked=true;
        else if(query.Contains("rendimiento"))NavPerformance.IsChecked=true;
        else if(query.Contains("ajuste")||query.Contains("opcion"))NavSettings.IsChecked=true;
        else if(query.Contains("perfil")||query.Contains("vip"))OpenAccountProfile(sender,new RoutedEventArgs());
        else NavHome.IsChecked=true;e.Handled=true;
    }
    private void OpenHeaderDrawer(string title,UIElement content)
    {
        var root=(Grid)((Border)Content).Child;if(headerOverlay is not null)root.Children.Remove(headerOverlay);headerOverlay=new Grid();Grid.SetColumnSpan(headerOverlay,2);var shade=new Border{Background=new SolidColorBrush(Color.FromArgb(170,0,0,0))};headerOverlay.Children.Add(shade);shade.MouseLeftButtonDown+=(_,_)=>root.Children.Remove(headerOverlay);
        var dock=new DockPanel{Margin=new Thickness(26)};var top=new DockPanel();var close=new Button{Content="×",FontSize=25,Width=36,Height=36,Style=(Style)FindResource("HeaderIcon")};DockPanel.SetDock(close,Dock.Right);top.Children.Add(close);if(title=="Perfil"){var logout=HeaderButton("Cerrar sesión  ↪");logout.Margin=new Thickness(0,0,12,0);logout.Click+=SignOut;DockPanel.SetDock(logout,Dock.Right);top.Children.Add(logout);}top.Children.Add(new TextBlock{Text=title,FontSize=21,Foreground=Brushes.White,VerticalAlignment=VerticalAlignment.Center});DockPanel.SetDock(top,Dock.Top);dock.Children.Add(top);close.Click+=(_,_)=>root.Children.Remove(headerOverlay);dock.Children.Add(new ScrollViewer{Content=content,VerticalScrollBarVisibility=ScrollBarVisibility.Auto});headerOverlay.Children.Add(new Border{Width=590,HorizontalAlignment=HorizontalAlignment.Right,Background=new SolidColorBrush(Color.FromRgb(17,19,23)),Child=dock});root.Children.Add(headerOverlay);
    }
    private static Border HeaderCard(string title,string detail)
    {
        var stack=new StackPanel();stack.Children.Add(new TextBlock{Text=title,FontSize=17,Foreground=Brushes.White,Margin=new Thickness(0,0,0,20)});stack.Children.Add(new TextBlock{Text=detail,TextWrapping=TextWrapping.Wrap,Foreground=new SolidColorBrush(Color.FromRgb(154,174,195)),FontSize=14});return new Border{Child=stack,Padding=new Thickness(24),Margin=new Thickness(0,24,0,0),BorderBrush=new SolidColorBrush(Color.FromRgb(50,58,70)),BorderThickness=new Thickness(1),CornerRadius=new CornerRadius(7)};
    }
    private static Button HeaderButton(string text)=>new(){Content=text,Margin=new Thickness(0,16,0,0),Padding=new Thickness(16,12,16,12),Background=new SolidColorBrush(Color.FromRgb(17,20,25)),Foreground=Brushes.White,BorderBrush=new SolidColorBrush(Color.FromRgb(65,75,89))};
}
