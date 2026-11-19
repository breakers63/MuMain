using System.Net;
using System.Net.NetworkInformation;
using System.Windows;
using System.Windows.Controls;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed class DnsBoostView : StackPanel
{
    readonly ComboBox adapters=new(){DisplayMemberPath="Label",Margin=new Thickness(0,16,0,0)};
    readonly StackPanel rows=new();
    readonly TextBlock status=RamBoostView.Label(""), updated=RamBoostView.Label("Sin medir",13);
    readonly List<DnsProvider> providers=DnsProvider.PublicResolvers();
    readonly System.Collections.Concurrent.ConcurrentDictionary<DnsProvider,double?> pings=new();
    DnsProvider? current;
    bool busy;
    DateTimeOffset lastMeasure;
    public DnsBoostView()
    {
        var header=new StackPanel();var line=new DockPanel();var buttons=new StackPanel{Orientation=Orientation.Horizontal};DockPanel.SetDock(buttons,Dock.Right);line.Children.Add(buttons);var measure=RamBoostView.Button("↻  Medir");buttons.Children.Add(measure);measure.Click+=async(_,_)=>await Run(measure,Measure);
        var restore=RamBoostView.Button("Restablecer anterior");buttons.Children.Add(restore);restore.Click+=async(_,_)=>await Run(restore,async()=>{await DnsSettings.Restore(Adapter());ReloadCurrent();return "DNS anterior restaurado. IPv6 se conserva.";});
        var labels=new StackPanel{Orientation=Orientation.Horizontal};labels.Children.Add(RamBoostView.Label("Clasificar servidores DNS",19));updated.Margin=new Thickness(22,4,0,0);updated.Foreground=RamBoostView.Color("#8799AF");labels.Children.Add(updated);line.Children.Add(labels);header.Children.Add(line);adapters.ItemsSource=DnsAdapter.Active();adapters.SelectedIndex=0;header.Children.Add(adapters);Children.Add(Card(header));
        adapters.SelectionChanged+=(_,_)=>{current=null;pings.Clear();foreach(var p in providers){p.Milliseconds=null;p.Replies=0;}ReloadCurrent();};
        Children.Add(rows);status.Margin=new Thickness(0,15,0,12);status.Foreground=RamBoostView.Color("#9AACBF");status.Visibility=Visibility.Collapsed;Children.Add(status);updated.ToolTip="DNS: mediana de tres consultas. Ping: respuesta ICMP del resolvedor, distinta al ping del juego.";ReloadCurrent();Loaded+=async(_,_)=>{if(DateTimeOffset.UtcNow-lastMeasure>TimeSpan.FromMinutes(5))await Run(measure,Measure);};
    }
    DnsAdapter Adapter()=>adapters.SelectedItem as DnsAdapter??throw new InvalidOperationException("Selecciona un adaptador activo.");
    void ReloadCurrent(){if(adapters.SelectedItem is DnsAdapter adapter){var latest=DnsAdapter.Active().FirstOrDefault(a=>a.Id==adapter.Id);if(latest is not null&&latest.CurrentServers.Length>0)current=new DnsProvider("Windows",latest.CurrentServers);}Render();}
    async Task<string> Measure()
    {
        var adapter=Adapter();var local=IPAddress.Parse(adapter.LocalAddress);var all=providers.Concat(current is null?[]:new[]{current}).ToList();
        await Task.WhenAll(all.Select(async p=>{await DnsBenchmark.Measure(p,local);pings[p]=await PingMedian(p.Addresses[0]);}));
        await Dispatcher.InvokeAsync(()=>{lastMeasure=DateTimeOffset.UtcNow;updated.Text="Actualizado ahora";Render();});return "";
    }
    static async Task<double?> PingMedian(string address)
    {
        var values=new List<double>();for(int i=0;i<3;i++){using var ping=new Ping();try{var response=await ping.SendPingAsync(address,1200);if(response.Status==IPStatus.Success)values.Add(response.RoundtripTime);}catch(PingException){}}return values.Count>=2?values.Order().ElementAt(values.Count/2):null;
    }
    void Render()
    {
        rows.Children.Clear();if(current is not null)rows.Children.Add(Row(current,"—",true,false));
        var ranked=providers.OrderBy(p=>p.Milliseconds??double.MaxValue).ToList();var fastest=ranked.FirstOrDefault(p=>p.Milliseconds.HasValue);
        var list=new StackPanel();for(int i=0;i<ranked.Count;i++){list.Children.Add(Row(ranked[i],(i+1).ToString(),false,ranked[i]==fastest));if(i<ranked.Count-1)list.Children.Add(new Border{Height=1,Background=RamBoostView.Color("#23272E"),Margin=new Thickness(0,0,0,0)});}rows.Children.Add(Card(list));
    }
    UIElement Row(DnsProvider provider,string rank,bool isCurrent,bool fastest)
    {
        var grid=new Grid{Margin=new Thickness(0,12,0,12)};grid.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(45)});foreach(double width in new[]{1.1,1.8,1.25,1.25})grid.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(width,GridUnitType.Star)});grid.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(200)});
        var number=new Border{CornerRadius=new CornerRadius(9),BorderBrush=RamBoostView.Color("#303743"),BorderThickness=new Thickness(1),Width=36,Height=38,Child=RamBoostView.Label(rank)};((TextBlock)number.Child).TextAlignment=TextAlignment.Center;grid.Children.Add(number);
        var name=RamBoostView.Label(provider.Name,15);name.FontWeight=FontWeights.SemiBold;Grid.SetColumn(name,1);grid.Children.Add(name);
        var address=RamBoostView.Label(provider.AddressLabel,13);address.Foreground=RamBoostView.Color("#8699B0");Grid.SetColumn(address,2);grid.Children.Add(address);
        var dns=RamBoostView.Label(provider.Milliseconds is double ms?$"DNS  ● {ms:0.0} ms":"DNS  Sin medir",14);dns.Foreground=RamBoostView.Color(provider.Milliseconds.HasValue?"#70C7A6":"#909CAA");Grid.SetColumn(dns,3);grid.Children.Add(dns);
        var ping=RamBoostView.Label(pings.TryGetValue(provider,out var value)?value is double v?$"Ping {v:0} ms":"Ping Sin respuesta":"Ping Sin medir",13);ping.Foreground=RamBoostView.Color(value.HasValue?"#70C7A6":"#A2AEC0");Grid.SetColumn(ping,4);grid.Children.Add(ping);
        if(isCurrent){var badge=RamBoostView.Label("● Actual",13);Grid.SetColumn(badge,5);grid.Children.Add(badge);}else{var choose=RamBoostView.Button(fastest?"⚡ Elegir el más rápido":"Elegir");choose.HorizontalAlignment=HorizontalAlignment.Right;if(fastest){choose.Background=RamBoostView.Color("#AC853D");choose.Foreground=RamBoostView.Color("#FFFFFF");}choose.Click+=async(_,_)=>await Run(choose,async()=>{await DnsSettings.Apply(Adapter(),provider);ReloadCurrent();return provider.Name+" aplicado a IPv4. Configuración anterior respaldada.";});Grid.SetColumn(choose,5);grid.Children.Add(choose);}return grid;
    }
    async Task Run(Button button,Func<Task<string>> action){if(busy)return;busy=true;button.IsEnabled=false;try{status.Visibility=Visibility.Visible;status.Text="Procesando…";status.Text=await action();status.Visibility=string.IsNullOrWhiteSpace(status.Text)?Visibility.Collapsed:Visibility.Visible;}catch(Exception e){status.Text="No se completó: "+e.Message;}finally{busy=false;button.IsEnabled=true;}}
    static Border Card(UIElement content)=>new(){Child=content,Background=RamBoostView.Color("#111316"),Padding=new Thickness(24,16,24,16),CornerRadius=new CornerRadius(7),Margin=new Thickness(0,12,0,12)};
}
