using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Shapes;
using System.Windows.Threading;
using ParisLauncher.Performance;
using ParisLauncher.Core;
namespace ParisLauncher.Views;
public sealed partial class NetworkAnalyzerView
{
    private sealed record Sample(DateTime Time,double? First,double? Second);
    readonly Dictionary<string,List<Sample>> destinationSamples=new();
    List<Sample> samples { get { string key=Destination();if(!destinationSamples.TryGetValue(key,out var list))destinationSamples[key]=list=[];return list; } }
    readonly Queue<string> events=[];
    readonly CancellationTokenSource lifetime=new();
    readonly DispatcherTimer timer=new(){Interval=TimeSpan.FromSeconds(2)};
    CancellationTokenSource? scanCancellation;
    string target="internet",testMode="TCPing";
    bool running=true,busy,started;
    DateTime began=DateTime.Now;
    public void SetProbeSeconds(int seconds) => timer.Interval=TimeSpan.FromSeconds(Math.Clamp(seconds,1,5));
    void Start()
    {
        if(started)return;started=true;var nic=DnsAdapter.Active().FirstOrDefault();ip.Text="Dirección IP local   "+(nic?.LocalAddress??"No disponible");adapterLabel.Text="Interfaz: "+(nic?.Name??"No disponible")+" · IP local "+(nic?.LocalAddress??"—");AddEvent("Monitorización iniciada");timer.Tick+=async(_,_)=>await Tick();timer.Start();_ = Tick();if(Window.GetWindow(this) is Window window)window.Closed+=(_,_)=>{timer.Stop();lifetime.Cancel();scanCancellation?.Cancel();};
    }
    void ResetTarget(){AddEvent("Destino seleccionado: "+Destination());Draw();}
    string Destination()=>target switch{"aws"=>(regions.SelectedItem as AwsRegion)?.Label??"AWS","gateway"=>Gateway()??"Puerta de enlace no disponible",_=>"Internet · Google + Cloudflare"};
    static string? Gateway()=>NetworkInterface.GetAllNetworkInterfaces().Where(a=>a.OperationalStatus==OperationalStatus.Up).SelectMany(a=>a.GetIPProperties().GatewayAddresses).Select(a=>a.Address).FirstOrDefault(a=>a.AddressFamily==System.Net.Sockets.AddressFamily.InterNetwork&&!a.Equals(IPAddress.Any))?.ToString();
    async Task Tick()
    {
        if(busy||!running||lifetime.IsCancellationRequested)return;busy=true;string measuredTarget=Destination();try
        {
            double? first,second=null;
            if(target=="internet"){var both=await Task.WhenAll(NetworkTests.Ping("8.8.8.8",lifetime.Token),NetworkTests.Ping("1.1.1.1",lifetime.Token));first=both[0];second=both[1];}
            else if(target=="gateway"){var gateway=Gateway();first=gateway is null?null:await NetworkTests.Ping(gateway,lifetime.Token);}
            else{var region=(AwsRegion)regions.SelectedItem;var value=await LatencyProbe.MeasureAsync(region.Host,443,1,lifetime.Token);first=value.MedianMs;}
            if(measuredTarget!=Destination())return;
            var previous=samples.LastOrDefault()?.First;samples.Add(new(DateTime.Now,first,second));if(!first.HasValue)AddEvent("Sonda sin respuesta: "+measuredTarget);else if(previous.HasValue&&first.Value>previous.Value+20)AddEvent($"Pico detectado ({first:0} ms)");Draw();
        }catch(OperationCanceledException){}catch(Exception e){health.Text="No disponible";AddEvent("Medición no disponible: "+e.Message);}finally{busy=false;}
    }
    IReadOnlyList<Sample> VisibleSamples(){int minutes=range.SelectedIndex switch{1=>5,2=>15,_=>1};return samples.Where(s=>s.Time>=DateTime.Now.AddMinutes(-minutes)).ToList();}
    void Draw()
    {
        chart.Children.Clear();double width=chart.ActualWidth;if(width<100)return;const double left=40,top=32,bottom=245;double right=width-12;var visible=VisibleSamples();var values=visible.Where(s=>s.First.HasValue).Select(s=>s.First!.Value).ToList();var all=visible.SelectMany(s=>new[]{s.First,s.Second}).Where(v=>v.HasValue).Select(v=>v!.Value).ToList();double ceiling=Math.Max(20,all.Count>0?Math.Ceiling(all.Max()/20)*20:100);
        for(int i=0;i<=4;i++){double y=top+(bottom-top)*i/4;chart.Children.Add(new Line{X1=left,Y1=y,X2=right,Y2=y,Stroke=Brush("#242B34")});Place(Text((ceiling*(4-i)/4).ToString("0"),11),0,y-8);}
        int minutes=range.SelectedIndex switch{1=>5,2=>15,_=>1};DateTime end=DateTime.Now,start=end.AddMinutes(-minutes);
        for(int i=0;i<=4;i++)Place(Text(start.AddSeconds(minutes*60*i/4d).ToString("HH:mm:ss"),11),left+(right-left)*i/4-(i==4?50:18),bottom+14);
        Plot(visible,s=>s.First,"#CB6CE5",start,end,left,right,top,bottom,ceiling);if(target=="internet")Plot(visible,s=>s.Second,"#E8B14C",start,end,left,right,top,bottom,ceiling);
        foreach(var sample in visible){double x=left+(sample.Time-start).TotalSeconds/(end-start).TotalSeconds*(right-left);chart.Children.Add(new Line{X1=x,Y1=bottom-4,X2=x+2,Y2=bottom-4,Stroke=Brush(sample.First.HasValue?"#35C6AC":"#D99849"),StrokeThickness=4});}
        if(values.Count==0){health.Text="Sin respuesta / sin muestras";metrics.Text="Latencia             —\n\nPicos                 —\n\nFluctuación          —\n\nSondas sin respuesta —\n\nConteo de fallos      —";return;}
        double jitter=values.Count>1?values.Zip(values.Skip(1),(a,b)=>Math.Abs(b-a)).Average():0;int lost=visible.Count-values.Count;int spikes=values.Zip(values.Skip(1),(a,b)=>b>a+20?1:0).Sum();health.Text=lost>0||jitter>20?"♥ Inestable":"♥ Estable";health.Foreground=Brush(lost>0?"#D9B64E":"#36D5C1");metrics.Text=$"Latencia                 {values.Last():0} ms\n\nPicos                     {spikes}\n\nFluctuación              {jitter:0.0} ms\n\nSondas sin respuesta     {100d*lost/Math.Max(1,visible.Count):0}%\n\nConteo de fallos          {lost}";summary.Text=$"Prom. {values.Average():0} ms    Mín. {values.Min():0} ms    Máx. {values.Max():0} ms\n\n● Latencia     ▰ Sonda respondida     ▰ Sonda sin respuesta";
    }
    void Plot(IReadOnlyList<Sample> source,Func<Sample,double?> select,string color,DateTime start,DateTime end,double left,double right,double top,double bottom,double ceiling)
    {
        var line=new Polyline{Stroke=Brush(color),StrokeThickness=1.5};foreach(var sample in source){var value=select(sample);if(!value.HasValue){if(line.Points.Count>0)chart.Children.Add(line);line=new Polyline{Stroke=Brush(color),StrokeThickness=1.5};continue;}line.Points.Add(new(left+(sample.Time-start).TotalSeconds/(end-start).TotalSeconds*(right-left),bottom-value.Value/ceiling*(bottom-top)));}if(line.Points.Count>0)chart.Children.Add(line);
    }
    void Place(UIElement element,double x,double y){Canvas.SetLeft(element,x);Canvas.SetTop(element,y);chart.Children.Add(element);}
    void AddEvent(string text){events.Enqueue($"{DateTime.Now:dd/MM/yyyy HH:mm:ss}   {text}");while(events.Count>200)events.Dequeue();}
    void OpenLogs()
    {
        var content=new StackPanel();var values=samples.Where(s=>s.First.HasValue).Select(s=>s.First!.Value).ToList();string stats=values.Count>0?$"Ping más alto   {values.Max():0.0} ms\nPing más bajo   {values.Min():0.0} ms\nPing promedio   {values.Average():0.0} ms":"Sin mediciones";
        var session=new StackPanel();session.Children.Add(Text("Datos de sesión",18));var heading=Text($"Iniciado {began:dd/MM/yyyy HH:mm:ss}    Servidor {Destination()}");heading.Margin=new Thickness(0,22,0,18);session.Children.Add(heading);
        var columns=new Grid();columns.ColumnDefinitions.Add(new ColumnDefinition());columns.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(16)});columns.ColumnDefinitions.Add(new ColumnDefinition());columns.Children.Add(Card(Text($"{adapterLabel.Text}\n\nProtocolo   {(target=="aws"?"TCP 443":"ICMP")}")));var statsCard=Card(Text(stats));Grid.SetColumn(statsCard,2);columns.Children.Add(statsCard);session.Children.Add(columns);content.Children.Add(Card(session));
        var eventList=new StackPanel();eventList.Children.Add(Text("Eventos",18));var entries=new StackPanel{Margin=new Thickness(0,22,0,0)};foreach(var entry in events.Reverse()){var text=Text(entry);text.Margin=new Thickness(0,12,0,12);entries.Children.Add(text);}if(events.Count==0)entries.Children.Add(Text("Sin eventos registrados todavía."));eventList.Children.Add(entries);var eventCard=Card(eventList);eventCard.MinHeight=340;content.Children.Add(eventCard);ShowDrawer("Registros registrados",content);
    }
    void OpenScanner()
    {
        var content = new SpeedTestPanel();
        var buttons = new StackPanel { Orientation = Orientation.Horizontal, HorizontalAlignment = HorizontalAlignment.Right };
        var cancel = Button("Cerrar"); var begin = Button("Iniciar prueba");
        buttons.Children.Add(cancel); buttons.Children.Add(begin);
        cancel.Click += (_, _) => CloseDrawer();
        begin.Click += async (_, _) =>
        {
            begin.IsEnabled = false;
            scanCancellation = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token);
            try
            {
                var speed = await content.Run(scanCancellation.Token);
                scanOutput.Text = $"Descarga {speed.Down:0.0} Mbps · Subida {speed.Up:0.0} Mbps";
                begin.Content = "Repetir prueba"; AddEvent("Prueba de velocidad completada");
            }
            catch (OperationCanceledException) { scanOutput.Text = "Prueba cancelada."; }
            catch (Exception e) { scanOutput.Text = "No se completó: " + e.Message; }
            finally { begin.IsEnabled = true; }
        };
        ShowDrawer("Escaneo de conexión", content, buttons);
    }

    async Task RunTools(Button button)
    {
        if(!int.TryParse(amount.Text,out int count)||count is <1 or >20){toolOutput.Text="Cantidad permitida: 1 a 20 sondas.";return;}string destination=host.Text.Trim();if(destination.Length>253||Uri.CheckHostName(destination)==UriHostNameType.Unknown){toolOutput.Text="Escribe un host o IP válido, sin URL ni parámetros.";return;}button.IsEnabled=false;
        try{toolSummary.Text="Midiendo "+testMode+"…";if(testMode=="MTR"){toolOutput.Text=await NetworkTests.Trace(destination,new Progress<string>(s=>toolOutput.Text=s),lifetime.Token);toolSummary.Text="MTR breve completado";return;}var log=new StringBuilder();var values=new List<double>();for(int i=0;i<count;i++){double? ms=testMode=="Ping"?await NetworkTests.Ping(destination,lifetime.Token):(await LatencyProbe.MeasureAsync(destination,443,1,lifetime.Token)).MedianMs;if(ms.HasValue)values.Add(ms.Value);log.AppendLine($"{i+1}. {destination}{(testMode=="TCPing"?":443/tcp":"/icmp")} · {(ms.HasValue?ms.Value.ToString("0.0")+" ms":"Sin respuesta")}");toolOutput.Text=log.ToString();}toolSummary.Foreground=Brush(values.Count>0?"#67D4B1":"#DCAD5C");toolSummary.Text=$"Estado de {testMode}\n\nEnviadas / recibidas   {count}/{values.Count}\n\nFallos    {count-values.Count}\n\nFallo     {100d*(count-values.Count)/count:0}%\n\n"+(values.Count>0?$"Promedio    {values.Average():0.0} ms\n\nMínimo    {values.Min():0.0} ms\n\nMáximo    {values.Max():0.0} ms":"Sin respuestas");}catch(OperationCanceledException){}catch(Exception e){toolOutput.Text="No se completó: "+e.Message;}finally{button.IsEnabled=true;}
    }
}
