using System.Net.Http;
using System.Net.Http.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;
namespace ParisLauncher;
public partial class MainWindow
{
    readonly DispatcherTimer directoryTimer=new(){Interval=TimeSpan.FromSeconds(30)};
    bool directoryStarted,directoryBusy;
    private sealed record Subserver(int Id,string Name,bool Available,int[]? Ports);
    private void OpenFacebook(object sender,RoutedEventArgs e) { if(!string.IsNullOrWhiteSpace(settings.FacebookUrl)) OpenHelpLink(settings.FacebookUrl); }
    private void OpenWhatsApp(object sender,RoutedEventArgs e)=>OpenHelpLink(settings.WhatsAppUrl);
    private void OpenCommunityDiscord(object sender,RoutedEventArgs e)=>OpenHelpLink(settings.DiscordUrl);
    private void LoadCommunity(object sender,RoutedEventArgs e)
    {
        if(directoryStarted)return;directoryStarted=true;
        FacebookButton.IsEnabled=!string.IsNullOrWhiteSpace(settings.FacebookUrl);
        directoryTimer.Tick+=async(_,_)=>await RefreshDirectory();directoryTimer.Start();
        directoryTimer.Tick+=async(_,_)=>await RefreshCounterStrike();
        Closed+=(_,_)=>directoryTimer.Stop();_ = RefreshDirectory();_ = RefreshCounterStrike();
    }
    private async Task RefreshDirectory()
    {
        if(directoryBusy)return;directoryBusy=true;
        try
        {
            using var http=new HttpClient{Timeout=TimeSpan.FromSeconds(8)};
            var rows=await http.GetFromJsonAsync<Subserver[]>(settings.AccountsApiUrl.TrimEnd('/')+"/servers")??[];
            Subservers.Children.Clear();
            foreach(var row in rows)
            {
                string name=row.Name.Contains("CROSSPLAY",StringComparison.OrdinalIgnoreCase)?"SERVIDOR CROSSPLAY":row.Name.Contains("PC ONLY",StringComparison.OrdinalIgnoreCase)?"SERVIDOR PC":row.Name;
                double? ping=null;
                foreach(int port in row.Ports??[])
                {
                    var measured=await ParisLauncher.Core.LatencyProbe.MeasureAsync(settings.ServerHost,port,1,CancellationToken.None);
                    if(measured.MedianMs.HasValue) ping=ping.HasValue?Math.Min(ping.Value,measured.MedianMs.Value):measured.MedianMs;
                }
                string state=ping.HasValue?$"● Disponible · {ping:0} ms":row.Available?"● Disponible · Sin respuesta TCP":"● No disponible";
                var panel=new StackPanel();panel.Children.Add(new TextBlock{Text=name,FontSize=11,Foreground=Brushes.Wheat});
                panel.Children.Add(new TextBlock{Text=state,FontSize=11,Foreground=new SolidColorBrush(row.Available?Color.FromRgb(122,211,147):Color.FromRgb(224,145,101)),Margin=new Thickness(0,4,0,0)});
                var line=new DockPanel();var emblem=Views.ServerGameEmblem.Mu();line.Children.Add(emblem);line.Children.Add(panel);
                Subservers.Children.Add(new Border{Child=line,Background=new SolidColorBrush(Color.FromRgb(20,22,24)),BorderBrush=new SolidColorBrush(Color.FromRgb(65,57,40)),BorderThickness=new Thickness(1),CornerRadius=new CornerRadius(5),Padding=new Thickness(10),Margin=new Thickness(0,0,0,6)});
            }
            if(rows.Length==0)Subservers.Children.Add(new TextBlock{Text="Sin subservidores publicados",Foreground=Brushes.Gray});
        }
        catch(Exception e) when(e is HttpRequestException or System.Text.Json.JsonException or TaskCanceledException)
        {
            Subservers.Children.Clear();Subservers.Children.Add(new TextBlock{Text="Estado de subservidores no disponible",Foreground=Brushes.Gray,FontSize=11,TextWrapping=TextWrapping.Wrap});
        }
        finally{directoryBusy=false;}
    }
}
