using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed class TrafficLimiterView : Grid
{
    readonly ComboBox applications = new() { DisplayMemberPath="Name", MinWidth=260, Margin=new Thickness(0,12,0,12) };
    readonly TextBox limit = new() { Text="2", Width=120, TextAlignment=TextAlignment.Center, Padding=new Thickness(12) };
    readonly TextBlock result = RamBoostView.Label("");
    readonly Button applyLimit = RamBoostView.Button("Aplicar límite");
    readonly Border shade = new() { Background=RamBoostView.Color("#AA000000"), Visibility=Visibility.Collapsed };
    readonly Border drawer = new() { Width=620, HorizontalAlignment=HorizontalAlignment.Right, Background=RamBoostView.Color("#111317"), Visibility=Visibility.Collapsed, Padding=new Thickness(28) };
    CancellationTokenSource? test;
    public TrafficLimiterView()
    {
        var root=new StackPanel { Margin=new Thickness(30,0,30,24) };
        Children.Add(new ScrollViewer { Content=root, VerticalScrollBarVisibility=ScrollBarVisibility.Auto });
        root.Children.Add(RamBoostView.Label("Limitador de tráfico",27));
        var center=new StackPanel { Margin=new Thickness(0,70,0,60), HorizontalAlignment=HorizontalAlignment.Center };
        var power=RamBoostView.Button("⏻");power.FontSize=40;power.Width=100;power.Height=100;
        center.Children.Add(power);center.Children.Add(RamBoostView.Label("Haz clic para medir tu ancho de banda",18));
        center.Children.Add(RamBoostView.Label("Límites de subida por aplicación · MU queda excluido",14));
        root.Children.Add(new Border { Child=center, Background=RamBoostView.Color("#111316"), CornerRadius=new CornerRadius(7), Margin=new Thickness(0,28,0,24) });
        power.Click+=(_,_)=>OpenMeasurement();
        root.Children.Add(RamBoostView.Label("Aplicación a limitar",18));
        NetworkAnalyzerView.StyleCombo(applications);applications.ItemsSource=MemoryTools.Applications();root.Children.Add(applications);
        var refresh=RamBoostView.Button("Actualizar aplicaciones");refresh.Click+=(_,_)=>applications.ItemsSource=MemoryTools.Applications();root.Children.Add(refresh);
        root.Children.Add(RamBoostView.Label("Límite de subida (Mbps)"));root.Children.Add(limit);
        var actions=new StackPanel { Orientation=Orientation.Horizontal };applyLimit.IsEnabled=false;actions.Children.Add(applyLimit);
        var remove=RamBoostView.Button("Quitar límites Paris");actions.Children.Add(remove);root.Children.Add(actions);root.Children.Add(result);
        applyLimit.Click+=async(_,_)=>await Apply();
        remove.Click+=async(_,_)=>{remove.IsEnabled=false;try{await TrafficLimiter.RemoveAll();result.Text="Se retiraron los límites creados por Paris.";}catch(Exception e){result.Text=e.Message;}finally{remove.IsEnabled=true;}};
        Children.Add(shade);Children.Add(drawer);Unloaded+=(_,_)=>test?.Cancel();
    }
    async Task Apply()
    {
        if(applications.SelectedItem is not MemoryApplication selected){result.Text="Selecciona la aplicación que quieres limitar.";return;}
        if(!decimal.TryParse(limit.Text,NumberStyles.Number,CultureInfo.CurrentCulture,out decimal rate)){result.Text="Introduce un límite numérico en Mbps.";return;}
        applyLimit.IsEnabled=false;
        try{await TrafficLimiter.Apply(selected.Path,rate);result.Text=$"Límite aplicado: {rate} Mbps a {selected.Name}.";}
        catch(Exception e){result.Text=e.Message;}
        finally{applyLimit.IsEnabled=true;}
    }
    void OpenMeasurement()
    {
        var panel=new DockPanel();var buttons=new StackPanel{Orientation=Orientation.Horizontal,HorizontalAlignment=HorizontalAlignment.Right};
        DockPanel.SetDock(buttons,Dock.Bottom);panel.Children.Add(buttons);
        var cancel=RamBoostView.Button("Cerrar");var begin=RamBoostView.Button("Iniciar prueba");begin.Background=RamBoostView.Color("#F6272E");buttons.Children.Add(cancel);buttons.Children.Add(begin);
        cancel.Click+=(_,_)=>{test?.Cancel();shade.Visibility=drawer.Visibility=Visibility.Collapsed;};
        var content=new StackPanel();content.Children.Add(RamBoostView.Label("Medición de ancho de banda",22));
        var speedPanel=new SpeedTestPanel{Margin=new Thickness(0,35,0,0)};content.Children.Add(speedPanel);
        var status=RamBoostView.Label("");content.Children.Add(status);panel.Children.Add(content);drawer.Child=panel;shade.Visibility=drawer.Visibility=Visibility.Visible;
        begin.Click+=async(_,_)=>
        {
            begin.IsEnabled=false;test?.Dispose();test=new CancellationTokenSource();status.Text="";
            try{var speed=await speedPanel.Run(test.Token);begin.Content="Repetir prueba";applyLimit.IsEnabled=true;limit.Text=Math.Max(0.1,Math.Round(speed.Up*0.7,1)).ToString(CultureInfo.CurrentCulture);status.Text="Selecciona una aplicación y revisa el límite antes de aplicarlo.";}
            catch(OperationCanceledException){status.Text="Prueba cancelada.";}
            catch(Exception e){status.Text="No se completó: "+e.Message;}
            finally{begin.IsEnabled=true;}
        };
    }
}
