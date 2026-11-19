using System.Windows;
using System.Windows.Controls;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Threading;
using ParisLauncher.Performance;
using ParisLauncher.Core;
namespace ParisLauncher.Views;
public sealed class GeneralSettingsView : ScrollViewer
{
    GeneralPreferences? current;
    readonly TextBlock hardware=RamBoostView.Label("");
    readonly DispatcherTimer hardwareTimer=new(){Interval=TimeSpan.FromSeconds(3)};
    readonly StackPanel root=new(){Margin=new Thickness(30,0,30,24)};
    public GeneralSettingsView(){hardwareTimer.Tick+=(_,_)=>UpdateHardware();Loaded+=(_,_)=>hardwareTimer.Start();Unloaded+=(_,_)=>hardwareTimer.Stop();Content=root;VerticalScrollBarVisibility=ScrollBarVisibility.Auto;HorizontalScrollBarVisibility=ScrollBarVisibility.Disabled;}
    public void Configure(GeneralPreferences p,Action save,Action<bool> startup,Action reset,Action pcBoost,Action dns)
    {
        current=p;if(hardware.Parent is Panel previous)previous.Children.Remove(hardware);root.Children.Clear();var title=new DockPanel();var defaults=RamBoostView.Button("Restablecer valores predeterminados");DockPanel.SetDock(defaults,Dock.Right);title.Children.Add(defaults);title.Children.Add(RamBoostView.Label("Ajustes generales",26));defaults.Click+=(_,_)=>reset();root.Children.Add(title);
        var preferences=new DockPanel();preferences.Children.Add(RamBoostView.Label("Preferencias",18));var choices=new StackPanel{Orientation=Orientation.Horizontal,HorizontalAlignment=HorizontalAlignment.Right};choices.Children.Add(RamBoostView.Button("Español (ES)"));var size=new ComboBox{ItemsSource=new[]{"1280 × 800","1440 × 810","1600 × 950","1920 × 1080"},SelectedItem=$"{p.WindowWidth} × {p.WindowHeight}",Width=180,Height=40,Margin=new Thickness(12,0,0,0)};NetworkAnalyzerView.StyleCombo(size);choices.Children.Add(size);preferences.Children.Add(choices);root.Children.Add(Card(preferences));size.SelectionChanged+=(_,_)=>{if(size.SelectedItem is string value){var parts=value.Split('×');p.WindowWidth=int.Parse(parts[0].Trim());p.WindowHeight=int.Parse(parts[1].Trim());save();}};
        var grid=new Grid();grid.ColumnDefinitions.Add(new ColumnDefinition());grid.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(24)});grid.ColumnDefinitions.Add(new ColumnDefinition());grid.RowDefinitions.Add(new RowDefinition());grid.RowDefinitions.Add(new RowDefinition());root.Children.Add(grid);
        var behavior=Section("▣  Comportamiento de la aplicación");Toggle(behavior,"Iniciar al iniciar Windows",p.StartWithWindows,v=>{startup(v);p.StartWithWindows=v;save();});Toggle(behavior,"Minimizar a la bandeja al cerrar",p.CloseToTray,v=>{p.CloseToTray=v;save();});Add(grid,behavior,0,0);
        var alerts=Section("♧  Notificaciones");Toggle(alerts,"Avisar al abrir una conexión de juego",p.ConnectionAlerts,v=>{p.ConnectionAlerts=v;save();});Toggle(alerts,"Alertas de servidor no disponible",p.ServerAlerts,v=>{p.ServerAlerts=v;save();});Add(grid,alerts,2,0);
        var optimize=Section("⚡  Opciones de optimización");var boost=RamBoostView.Button("Configurar PC Boost");boost.Click+=(_,_)=>pcBoost();optimize.Children.Add(boost);Disabled(optimize,"Usar rutas duales","Requiere nodos y un servicio de túneles. No está disponible en esta versión.");var precision=new ComboBox{ItemsSource=new[]{"Alta · 1 segundo","Equilibrada · 2 segundos","Ligera · 5 segundos"},SelectedIndex=p.ProbeSeconds==1?0:p.ProbeSeconds==5?2:1,Margin=new Thickness(0,12,0,12)};optimize.Children.Add(RamBoostView.Label("Precisión del monitoreo",14));NetworkAnalyzerView.StyleCombo(precision);optimize.Children.Add(precision);precision.SelectionChanged+=(_,_)=>{p.ProbeSeconds=precision.SelectedIndex switch{0=>1,2=>5,_=>2};save();};Disabled(optimize,"Protocolo de redirección · No disponible","No se redirige tráfico sin un servicio de túneles.");Add(grid,optimize,0,1);
        var advanced=Section("⚒  Opciones avanzadas");Toggle(advanced,"Monitoreo de hardware",p.HardwareMonitor,v=>{p.HardwareMonitor=v;save();UpdateHardware();});advanced.Children.Add(hardware);UpdateHardware();Disabled(advanced,"Atajo para reiniciar conexiones","No se reinician las conexiones del sistema desde este launcher.");advanced.Children.Add(RamBoostView.Label("Versión IP del análisis: IPv4",14));var restoreDns=RamBoostView.Button("Restaurar configuración DNS");restoreDns.Click+=(_,_)=>dns();advanced.Children.Add(restoreDns);Add(grid,advanced,2,1);
    }
    void UpdateHardware(){hardware.Visibility=current?.HardwareMonitor==true?Visibility.Visible:Visibility.Collapsed;if(current?.HardwareMonitor==true){var m=MemoryTools.Snapshot();hardware.Text=$"RAM: {m.Load}% usada · {m.Available/1073741824d:F1} GB disponibles";}}
    static StackPanel Section(string title){var panel=new StackPanel();panel.Children.Add(new TextBlock{Text=title,FontSize=18,Foreground=RamBoostView.Color("#E4EAF3"),FontWeight=FontWeights.SemiBold,Margin=new Thickness(0,0,0,22)});return panel;}
    static void Add(Grid grid,StackPanel content,int col,int row){var card=Card(content);Grid.SetColumn(card,col);Grid.SetRow(card,row);grid.Children.Add(card);}
    static Border Card(UIElement content)=>new(){Child=content,Padding=new Thickness(24),Background=RamBoostView.Color("#111316"),CornerRadius=new CornerRadius(7),Margin=new Thickness(0,12,0,12)};
    static void Disabled(StackPanel parent,string title,string detail){Toggle(parent,title,false,_=>{});var option=(CheckBox)parent.Children[parent.Children.Count-1];option.IsEnabled=false;option.ToolTip=detail;}

    static void Toggle(StackPanel parent,string title,bool value,Action<bool> changed)
    {
        var check=new CheckBox{Content=title,IsChecked=value,Foreground=RamBoostView.Color("#DDE5EF"),Margin=new Thickness(0,16,0,20)};check.Template=(ControlTemplate)XamlReader.Parse("""
<ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" TargetType="CheckBox"><StackPanel Orientation="Horizontal"><Border x:Name="track" Width="36" Height="20" CornerRadius="10" Background="#242830" Margin="0,0,14,0"><Ellipse x:Name="dot" Width="15" Height="15" Fill="#E4E7EC" HorizontalAlignment="Left" Margin="3,0"/></Border><ContentPresenter VerticalAlignment="Center"/></StackPanel><ControlTemplate.Triggers><Trigger Property="IsChecked" Value="True"><Setter TargetName="track" Property="Background" Value="#1EBE9C"/><Setter TargetName="dot" Property="HorizontalAlignment" Value="Right"/></Trigger></ControlTemplate.Triggers></ControlTemplate>
""");parent.Children.Add(check);bool updating=false;void Update(){if(updating)return;try{changed(check.IsChecked==true);}catch(Exception e){updating=true;check.IsChecked=!(check.IsChecked==true);updating=false;MessageBox.Show(e.Message,"PARIS PROJECT");}}check.Checked+=(_,_)=>Update();check.Unchecked+=(_,_)=>Update();
    }
}
