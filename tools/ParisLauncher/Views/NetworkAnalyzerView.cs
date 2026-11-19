using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Markup;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed partial class NetworkAnalyzerView : Grid
{
    readonly StackPanel pages=new(),monitor=new(),scanner=new(),games=new(),tools=new();
    readonly TextBlock health=Text("Esperando muestras",17),metrics=Text("—"),summary=Text("Sin mediciones"),ip=Text(""),adapterLabel=Text(""),toolSummary=Text("Sin prueba"),toolOutput=Text("Selecciona una prueba y pulsa Iniciar."),scanOutput=Text("Ejecuta el escáner para medir tu conexión."),drawerText=Text("");
    readonly Canvas chart=new(){Height=300,ClipToBounds=true};
    readonly ComboBox regions=new(){ItemsSource=AwsRegion.All(),DisplayMemberPath="Label",SelectedIndex=1,Width=270},range=new(){ItemsSource=new[]{"1 min","5 min","15 min"},SelectedIndex=0,Width=90};
    readonly TextBox host=new(){Text="8.8.8.8",Width=180,Padding=new Thickness(12,8,12,8)},amount=new(){Text="4",Width=65,Padding=new Thickness(12,8,12,8)};
    readonly Border shade=new(){Background=new SolidColorBrush(Color.FromArgb(170,0,0,0)),Visibility=Visibility.Collapsed};
    readonly Border drawer=new(){Background=Brush("#111317"),Width=640,HorizontalAlignment=HorizontalAlignment.Right,Visibility=Visibility.Collapsed,Padding=new Thickness(26)};
    public string ServerHost { get; set; }="15.235.17.79";
    public int ServerPort { get; set; }=44405;
    public NetworkAnalyzerView()
    {
        var scroll=new ScrollViewer{Content=pages,VerticalScrollBarVisibility=ScrollBarVisibility.Auto,HorizontalScrollBarVisibility=ScrollBarVisibility.Disabled,Padding=new Thickness(30,0,30,24)};Children.Add(scroll);pages.Children.Add(Text("Analizador de redes",27));
        var tabs=new StackPanel{Orientation=Orientation.Horizontal,Margin=new Thickness(0,24,0,0)};pages.Children.Add(tabs);
        foreach(var pair in new[]{("Monitoreo en tiempo real",monitor),("Escáner",scanner),("Ping para juegos",games),("Herramientas",tools)}){var button=Button(pair.Item1);tabs.Children.Add(button);button.Click+=(_,_)=>Show(pair.Item2);}
        var header=new DockPanel();var actions=new StackPanel{Orientation=Orientation.Horizontal};DockPanel.SetDock(actions,Dock.Right);header.Children.Add(actions);var toggle=Button("Apagar ⏻");actions.Children.Add(toggle);toggle.Click+=(_,_)=>{running=!running;toggle.Content=running?"Apagar ⏻":"Encender ⏻";AddEvent(running?"Monitorización iniciada":"Monitorización pausada");};var logs=Button("Ver registros");actions.Children.Add(logs);logs.Click+=(_,_)=>OpenLogs();
        var title=new StackPanel();title.Children.Add(Text("Supervisa tu servicio de internet",18));title.Children.Add(ip);header.Children.Add(title);pages.Children.Add(Card(header));
        StyleCombo(regions);StyleCombo(range);regions.DisplayMemberPath="";regions.ItemTemplate=(DataTemplate)XamlReader.Parse("""
<DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"><StackPanel Orientation="Horizontal"><Image Source="{Binding FlagUri}" Width="24" Height="17" Margin="0,0,10,0"/><TextBlock Text="{Binding Label}" Foreground="#DDE6F2" VerticalAlignment="Center"/></StackPanel></DataTemplate>
""");toolSummary.FontFamily=new FontFamily("Consolas");toolSummary.TextAlignment=TextAlignment.Center;toolSummary.VerticalAlignment=VerticalAlignment.Top;toolSummary.LineHeight=24;metrics.FontFamily=new FontFamily("Consolas");host.TextAlignment=amount.TextAlignment=TextAlignment.Center;host.VerticalContentAlignment=amount.VerticalContentAlignment=VerticalAlignment.Center;var activeAdapter=DnsAdapter.Active().FirstOrDefault();ip.Text="Dirección IP local   "+(activeAdapter?.LocalAddress??"No disponible");adapterLabel.Text="Interfaz: "+(activeAdapter?.Name??"No disponible")+" · IP local "+(activeAdapter?.LocalAddress??"—");BuildMonitor();BuildScanner();BuildTools();BuildGames();BuildStability();foreach(var page in new[]{monitor,scanner,games,tools})pages.Children.Add(page);Show(monitor);
        Children.Add(shade);Children.Add(drawer);shade.MouseLeftButtonUp+=(_,_)=>CloseDrawer();chart.SizeChanged+=(_,_)=>Draw();regions.SelectionChanged+=(_,_)=>{if(target=="aws")ResetTarget();};range.SelectionChanged+=(_,_)=>Draw();
        Loaded+=(_,_)=>Start();
    }
    void Show(StackPanel chosen){foreach(var p in new[]{monitor,scanner,games,tools})p.Visibility=p==chosen?Visibility.Visible:Visibility.Collapsed;}
    void BuildMonitor()
    {
        var controls=new DockPanel();var select=new StackPanel{Orientation=Orientation.Horizontal};controls.Children.Add(select);
        foreach(var item in new[]{("Internet","internet"),("Puerta de enlace","gateway"),("Amazon AWS","aws")}){var b=Button(item.Item1);select.Children.Add(b);b.Click+=(_,_)=>{target=item.Item2;ResetTarget();};}select.Children.Add(regions);regions.Margin=new Thickness(10,4,0,4);regions.ToolTip="Regiones AWS · endpoint público EC2, TCP 443. No son nodos de ruta Paris.";
        DockPanel.SetDock(range,Dock.Right);controls.Children.Insert(0,range);monitor.Children.Add(Card(controls));
        var body=new Grid();body.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(300)});body.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(24)});body.ColumnDefinitions.Add(new ColumnDefinition());
        var left=new StackPanel();left.Children.Add(Text("Salud de la conexión",16));health.Margin=new Thickness(12,10,12,10);health.TextAlignment=TextAlignment.Center;left.Children.Add(new Border{Child=health,Background=Brush("#143035"),CornerRadius=new CornerRadius(18)});left.Children.Add(metrics);body.Children.Add(Card(left));
        var right=new StackPanel();var legend=new DockPanel();legend.Children.Add(Text("Gráfico de latencia",16));var colors=Text("● Google     ● Cloudflare",13);colors.Foreground=Brush("#D2A0E7");colors.HorizontalAlignment=HorizontalAlignment.Right;legend.Children.Add(colors);right.Children.Add(legend);right.Children.Add(chart);summary.Margin=new Thickness(0,15,0,0);right.Children.Add(summary);var frame=Card(right);Grid.SetColumn(frame,2);body.Children.Add(frame);monitor.Children.Add(body);monitor.Children.Add(Text("Internet: ICMP a Google y Cloudflare. AWS: conexión TCP al endpoint regional. Un ICMP bloqueado no demuestra pérdida del tráfico de juego.",12));
    }
    void BuildScanner()
    {
        var top=new DockPanel();top.Children.Add(Text("Diagnóstico de conexión",20));var begin=Button("Iniciar escaneo");DockPanel.SetDock(begin,Dock.Right);top.Children.Insert(0,begin);begin.Click+=(_,_)=>OpenScanner();scanner.Children.Add(Card(top));scanner.Children.Add(Card(scanOutput));
    }
    void BuildGames()
    {
        games.Children.Add(Card(Text("Ping para juegos · MU Paris",20)));var test=Button("Medir conexión al servidor de MU");games.Children.Add(test);var output=Text("Sin medición");games.Children.Add(Card(output));test.Click+=async(_,_)=>{test.IsEnabled=false;try{var value=await ParisLauncher.Core.LatencyProbe.MeasureAsync(ServerHost,ServerPort,8,lifetime.Token);output.Text=$"{ServerHost}:{ServerPort}\n\nTCP: {value.MedianMs:0.0} ms · {value.Received}/{value.Sent} respuestas\nMínimo {value.MinMs:0.0} ms · Máximo {value.MaxMs:0.0} ms\nFluctuación {value.JitterMs:0.0} ms";}catch(Exception e){output.Text=e.Message;}finally{test.IsEnabled=true;}};
    }
    void BuildTools()
    {
        var controls=new StackPanel{Orientation=Orientation.Horizontal};controls.Children.Add(Text("Elige tu prueba",17));foreach(var name in new[]{"MTR","Ping","TCPing"}){var b=Button(name);controls.Children.Add(b);b.Click+=(_,_)=>{testMode=name;toolSummary.Text="Prueba seleccionada: "+name;};}controls.Children.Add(host);controls.Children.Add(amount);var run=Button("Iniciar ▶");controls.Children.Add(run);run.Click+=async(_,_)=>await RunTools(run);tools.Children.Add(Card(controls));adapterLabel.Margin=new Thickness(0,5,0,5);tools.Children.Add(Card(adapterLabel));var body=new Grid();body.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(280)});body.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(20)});body.ColumnDefinitions.Add(new ColumnDefinition());var statsCard=Card(toolSummary);statsCard.MinHeight=420;body.Children.Add(statsCard);var results=new StackPanel();results.Children.Add(Text("Resultados",17));toolOutput.Margin=new Thickness(0,28,0,0);results.Children.Add(toolOutput);var output=Card(results);output.MinHeight=420;Grid.SetColumn(output,2);body.Children.Add(output);tools.Children.Add(body);
    }
    void ShowDrawer(string title,UIElement content,UIElement? footer=null)
    {
        var panel=new DockPanel();var top=new DockPanel();var close=Button("✕");DockPanel.SetDock(close,Dock.Right);top.Children.Add(close);top.Children.Add(Text(title,20));DockPanel.SetDock(top,Dock.Top);panel.Children.Add(top);close.Click+=(_,_)=>CloseDrawer();if(footer is not null){DockPanel.SetDock(footer,Dock.Bottom);panel.Children.Add(footer);}panel.Children.Add(new ScrollViewer{Content=content,VerticalScrollBarVisibility=ScrollBarVisibility.Auto});drawer.Child=panel;shade.Visibility=drawer.Visibility=Visibility.Visible;
    }
    void CloseDrawer(){scanCancellation?.Cancel();shade.Visibility=drawer.Visibility=Visibility.Collapsed;}
    internal static void StyleCombo(ComboBox combo)
    {
        combo.Foreground=Brush("#DDE6F2");combo.MinHeight=42;combo.ItemContainerStyle=new Style(typeof(ComboBoxItem));combo.ItemContainerStyle.Setters.Add(new Setter(Control.BackgroundProperty,Brush("#1C2028")));combo.ItemContainerStyle.Setters.Add(new Setter(Control.ForegroundProperty,Brush("#DDE6F2")));
        combo.Template=(ControlTemplate)XamlReader.Parse("""
<ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" TargetType="ComboBox"><Grid><ToggleButton IsChecked="{Binding IsDropDownOpen,RelativeSource={RelativeSource TemplatedParent},Mode=TwoWay}"><ToggleButton.Template><ControlTemplate TargetType="ToggleButton"><Border Background="#1B1F26" BorderBrush="#303741" BorderThickness="1" CornerRadius="5" Padding="12,9"><ContentPresenter/></Border></ControlTemplate></ToggleButton.Template><Grid><ContentPresenter TextElement.Foreground="#DDE6F2" Content="{TemplateBinding SelectionBoxItem}" ContentTemplate="{TemplateBinding ItemTemplate}" Margin="0,0,20,0"/><TextBlock Text="⌄" Foreground="#DDE6F2" HorizontalAlignment="Right"/></Grid></ToggleButton><Popup x:Name="PART_Popup" IsOpen="{TemplateBinding IsDropDownOpen}" Placement="Bottom" AllowsTransparency="True"><Border Background="#1B1F26" BorderBrush="#343C49" BorderThickness="1" Padding="8" MinWidth="260"><ScrollViewer MaxHeight="360"><ItemsPresenter/></ScrollViewer></Border></Popup></Grid></ControlTemplate>
""");
    }
    static TextBlock Text(string text,double size=14)=>new(){Text=text,FontSize=size,Foreground=Brush(size>15?"#DFE7F1":"#8E9FB5"),TextWrapping=TextWrapping.Wrap,VerticalAlignment=VerticalAlignment.Center};
    static SolidColorBrush Brush(string value)=>(SolidColorBrush)new BrushConverter().ConvertFromString(value)!;
    static Button Button(string text)=>new(){Content=text,Background=Brush("#1B1F26"),Foreground=Brush("#DEE7F1"),BorderBrush=Brush("#303741"),Margin=new Thickness(4),Padding=new Thickness(15,12,15,12)};
    static Border Card(UIElement content)=>new(){Child=content,Background=Brush("#111316"),BorderBrush=Brush("#282F39"),BorderThickness=new Thickness(1),CornerRadius=new CornerRadius(7),Padding=new Thickness(24),Margin=new Thickness(0,12,0,12)};
}
