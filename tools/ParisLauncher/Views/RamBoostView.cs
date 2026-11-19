using System.IO;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;
using System.Windows.Markup;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed class RamBoostView : StackPanel
{
    readonly BoostPreferences preferences=BoostPreferences.Load();
    readonly Func<IEnumerable<string>> gamePaths;
    readonly TextBlock usage=Label(""), state=Label(""), result=Label("Listo para limpiar memoria recuperable de aplicaciones. No se cierran programas.");
    readonly ProgressBar bar=new(){Maximum=100,Height=24,Margin=new Thickness(0,24,0,20),Foreground=Color("#B58D42"),Background=Color("#20242B"),BorderThickness=new Thickness(0)};
    readonly StackPanel exceptions=new();
    readonly DispatcherTimer timer=new(){Interval=TimeSpan.FromSeconds(3)};
    bool cleaning, hadGame;
    public RamBoostView(Func<IEnumerable<string>> paths)
    {
        gamePaths=paths; preferences.TrimIntervalHours=Math.Clamp(preferences.TrimIntervalHours,1,24);
        var main=new StackPanel(); var header=new DockPanel(); var clean=Button("✦  Limpiar"); clean.Background=Color("#B38B3E"); clean.Foreground=Color("#FFFFFF"); DockPanel.SetDock(clean,Dock.Right); header.Children.Add(clean); clean.Click+=async(_,_)=>await Clean();
        var heading=new StackPanel{Orientation=Orientation.Horizontal}; var title=Label("Limpiador de RAM",19); title.FontWeight=FontWeights.SemiBold; heading.Children.Add(title); state.Margin=new Thickness(20,3,0,0); heading.Children.Add(state); header.Children.Add(heading); main.Children.Add(header); main.Children.Add(bar); main.Children.Add(usage); Children.Add(Card(main));
        var columns=new Grid(); columns.ColumnDefinitions.Add(new ColumnDefinition()); columns.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(24)}); columns.ColumnDefinitions.Add(new ColumnDefinition()); Children.Add(columns);
        var routine=new StackPanel(); var routineHeader=new StackPanel{Orientation=Orientation.Horizontal}; routineHeader.Children.Add(Label("Rutina",17)); var automatic=new CheckBox{Content="Automatizar limpiezas",IsChecked=preferences.AutomaticMemoryTrim,Foreground=Color("#D9E1EC"),Margin=new Thickness(24,0,0,0)}; automatic.Template=(ControlTemplate)XamlReader.Parse("""
<ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" TargetType="CheckBox"><StackPanel Orientation="Horizontal"><Border x:Name="track" Width="36" Height="20" CornerRadius="10" Background="#242830" Margin="0,0,14,0"><Ellipse x:Name="dot" Width="15" Height="15" Fill="#E4E7EC" HorizontalAlignment="Left" Margin="3,0"/></Border><ContentPresenter VerticalAlignment="Center"/></StackPanel><ControlTemplate.Triggers><Trigger Property="IsChecked" Value="True"><Setter TargetName="track" Property="Background" Value="#B78D3B"/><Setter TargetName="dot" Property="HorizontalAlignment" Value="Right"/></Trigger></ControlTemplate.Triggers></ControlTemplate>
""");routineHeader.Children.Add(automatic); routine.Children.Add(routineHeader);
        var intro=Label("Ejecutar cuando:"); intro.Foreground=Color("#8899AF"); intro.Margin=new Thickness(0,28,0,18); routine.Children.Add(intro);
        var onGame=new RadioButton{Content="Comienza una sesión de juego",GroupName="RamRoutine",IsChecked=preferences.TrimOnGameStart,Foreground=Color("#D9E1EC"),Margin=new Thickness(0,0,0,20)};
        var onPressure=new RadioButton{Content="Hay presión de memoria (80% o más)",GroupName="RamRoutine",IsChecked=!preferences.TrimOnGameStart,Foreground=Color("#D9E1EC"),Margin=new Thickness(0,0,0,18)}; routine.Children.Add(onGame);routine.Children.Add(onPressure);
        var interval=new ComboBox{ItemsSource=new[]{1,3,6,12,24},SelectedItem=preferences.TrimIntervalHours,Height=36,Width=180,HorizontalAlignment=HorizontalAlignment.Left};routine.Children.Add(Label("Intervalo mínimo entre limpiezas (horas)",13));interval.Template=(ControlTemplate)XamlReader.Parse("""
<ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" TargetType="ComboBox"><Grid><ToggleButton Background="#1B1F25" Foreground="#DDE4EE" IsChecked="{Binding IsDropDownOpen,RelativeSource={RelativeSource TemplatedParent},Mode=TwoWay}"><ToggleButton.Template><ControlTemplate TargetType="ToggleButton"><Border Background="#1B1F25" CornerRadius="5" Padding="12,8"><Grid><ContentPresenter/><TextBlock Text="⌄" HorizontalAlignment="Right" Foreground="#DDE4EE"/></Grid></Border></ControlTemplate></ToggleButton.Template><ContentPresenter Content="{TemplateBinding SelectionBoxItem}" HorizontalAlignment="Left"/></ToggleButton><Popup x:Name="PART_Popup" IsOpen="{TemplateBinding IsDropDownOpen}" Placement="Bottom" AllowsTransparency="True"><Border Background="#1B1F25" Padding="12" MinWidth="180"><ItemsPresenter/></Border></Popup></Grid></ControlTemplate>
""");interval.Foreground=Color("#DDE4EE");routine.Children.Add(interval);
        automatic.Checked+=(_,_)=>Save(); automatic.Unchecked+=(_,_)=>Save();onGame.Checked+=(_,_)=>Save();onPressure.Checked+=(_,_)=>Save();interval.SelectionChanged+=(_,_)=>Save();
        void Save(){preferences.AutomaticMemoryTrim=automatic.IsChecked==true;preferences.TrimOnGameStart=onGame.IsChecked==true;preferences.TrimIntervalHours=interval.SelectedItem is int hours?hours:12;preferences.Save();}
        var note=Label("Se ejecuta mientras el launcher esté abierto. La rutina comienza desactivada hasta que la habilites.",13);note.Foreground=Color("#8899AF");note.Margin=new Thickness(0,22,0,0);routine.Children.Add(note);columns.Children.Add(Card(routine));
        var allowed=new StackPanel();var allowedHead=new DockPanel();var add=Button("+  Agregar aplicación");DockPanel.SetDock(add,Dock.Right);allowedHead.Children.Add(add);allowedHead.Children.Add(Label("Lista de excepciones",17));allowed.Children.Add(allowedHead);allowed.Children.Add(exceptions);var allowedCard=Card(allowed);Grid.SetColumn(allowedCard,2);columns.Children.Add(allowedCard);
        add.Click+=(_,_)=>{var picker=new MemoryExclusionWindow();picker.Owner=Window.GetWindow(this);if(picker.ShowDialog()==true){preferences.ExcludedApplications=preferences.ExcludedApplications.Concat(picker.SelectedPaths).Distinct(StringComparer.OrdinalIgnoreCase).ToList();preferences.Save();RefreshExceptions();}};
        result.Margin=new Thickness(0,18,0,12);result.Foreground=Color("#9CACC0");Children.Add(result);RefreshExceptions();Refresh();
        timer.Tick+=async(_,_)=>{try{Refresh();bool game=GameRunning();bool started=game&&!hadGame;hadGame=game;if(preferences.AutomaticMemoryTrim&&!cleaning&&MemoryTrimPolicy.Due(preferences.LastTrim,DateTimeOffset.UtcNow,preferences.TrimIntervalHours)&& (preferences.TrimOnGameStart?started:MemoryTools.Snapshot().Load>=80))await Clean();}catch(Exception error){result.Text="No se pudo actualizar RAM: "+error.Message;}};
        bool attached=false; Loaded+=(_,_)=>{if(attached)return;attached=true;hadGame=GameRunning();timer.Start();if(Window.GetWindow(this) is Window window)window.Closed+=(_,_)=>timer.Stop();};
    }
    bool GameRunning(){var paths=gamePaths().Select(Path.GetFullPath).ToHashSet(StringComparer.OrdinalIgnoreCase);foreach(var process in Process.GetProcessesByName("Main")){using(process){try{if(process.MainModule?.FileName is string path&&paths.Contains(path))return true;}catch(System.ComponentModel.Win32Exception){}catch(InvalidOperationException){}}}return false;}
    async Task Clean()
    {
        if(cleaning)return;cleaning=true;try{var before=MemoryTools.Snapshot();result.Text="Limpiando memoria recuperable…";var excluded=preferences.ExcludedApplications.Concat(gamePaths()).ToHashSet(StringComparer.OrdinalIgnoreCase);int count=await Task.Run(()=>MemoryTools.TrimSelected(MemoryTools.Applications().Select(a=>a.Path).ToHashSet(StringComparer.OrdinalIgnoreCase),excluded));preferences.LastTrim=DateTimeOffset.UtcNow;preferences.Save();await Task.Delay(400);var after=MemoryTools.Snapshot();double released=((double)after.Available-before.Available)/1048576d;result.Text=$"Completado: {count} procesos atendidos. Cambio de RAM disponible: {released:+0;-0;0} MB. Las excepciones se conservaron. La memoria puede volver a cargarse.";Refresh();}catch(Exception error){result.Text="No se completó la limpieza: "+error.Message;}finally{cleaning=false;}
    }
    void Refresh(){var m=MemoryTools.Snapshot();bar.Value=m.Load;usage.Text=$"● Usado {(m.Total-m.Available)/1073741824d:F1} GB       ○ Libre {m.Available/1073741824d:F1} GB                              {m.Total/1073741824d:F0} GB en total";state.Text=m.Load>=80?"● Presión de memoria":"● Memoria disponible";state.Foreground=Color(m.Load>=80?"#D5BA64":"#71C6A3");}
    void RefreshExceptions(){exceptions.Children.Clear();if(preferences.ExcludedApplications.Count==0){var empty=Label("ⓘ\n\nNo se han añadido excepciones\nMantén tus aplicaciones esenciales intactas.");empty.TextAlignment=TextAlignment.Center;empty.Foreground=Color("#91A1B6");empty.Margin=new Thickness(0,48,0,34);exceptions.Children.Add(empty);}foreach(string path in preferences.ExcludedApplications.ToList()){var row=new DockPanel{Margin=new Thickness(0,10,0,0)};var remove=Button("Quitar");DockPanel.SetDock(remove,Dock.Right);row.Children.Add(remove);var name=Label(Path.GetFileName(path));name.ToolTip=path;name.VerticalAlignment=VerticalAlignment.Center;row.Children.Add(name);exceptions.Children.Add(row);remove.Click+=(_,_)=>{preferences.ExcludedApplications.Remove(path);preferences.Save();RefreshExceptions();};}}
    internal static SolidColorBrush Color(string hex)=>(SolidColorBrush)new BrushConverter().ConvertFromString(hex)!;
    internal static TextBlock Label(string text,double size=14)=>new(){Text=text,FontSize=size,Foreground=Color("#E0E7F1"),TextWrapping=TextWrapping.Wrap,VerticalAlignment=VerticalAlignment.Center};
    internal static Button Button(string text)=>new(){Content=text,Padding=new Thickness(16,11,16,11),Margin=new Thickness(4),Background=Color("#1B1F25"),Foreground=Color("#DAE3EF"),BorderBrush=Color("#434B56")};
    static Border Card(UIElement child)=>new(){Child=child,Background=Color("#111316"),CornerRadius=new CornerRadius(7),Padding=new Thickness(24),Margin=new Thickness(0,12,0,12)};
}
