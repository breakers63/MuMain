using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using System.Windows.Markup;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed class PcBoostView : ScrollViewer
{
    readonly TextBlock status = Text("",14,"#9AA8BA");
    readonly List<BoostOption> options = BoostOption.Create().Where(o => o.Id is not ("priority" or "background")).ToList();
    readonly List<CheckBox> applications = [];
    public IEnumerable<string> Executables { get; set; } = [];
    public PcBoostView()
    {
        VerticalScrollBarVisibility=ScrollBarVisibility.Auto; HorizontalScrollBarVisibility=ScrollBarVisibility.Disabled;
        var root=new StackPanel { Margin=new Thickness(30,0,30,24) }; Content=root;
        root.Children.Add(Text("PC Boost",28,"#E9EDF4"));
        var tabs=new TabControl { Template=Parse("""
        <ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" TargetType="TabControl"><StackPanel><TabPanel IsItemsHost="True" Margin="0,24,0,0"/><ContentPresenter ContentSource="SelectedContent"/></StackPanel></ControlTemplate>
        """) }; root.Children.Add(tabs);tabs.SelectionChanged+=(_,_)=>status.Visibility=tabs.SelectedIndex==0?Visibility.Visible:Visibility.Collapsed;
        BuildSystem(Page(tabs,"Ajustes del sistema")); BuildMemory(Page(tabs,"Limpiador de RAM")); BuildDns(Page(tabs,"DNS Optimizer"));
        status.Margin=new Thickness(0,20,0,10); root.Children.Add(status);
    }
    void BuildSystem(StackPanel panel)
    {
        var header=new DockPanel(); var presets=new StackPanel { Orientation=Orientation.Horizontal, HorizontalAlignment=HorizontalAlignment.Right }; DockPanel.SetDock(presets,Dock.Right); header.Children.Add(presets);
        foreach(var name in new[]{"Predeterminado","Alto rendimiento","Personalizado"}) Action(presets,name,()=>{ if(name=="Predeterminado") options.ForEach(o=>o.Selected=false); else if(name=="Alto rendimiento") options.ForEach(o=>o.Selected=o.Id is "gpu" or "gamemode" or "capture" or "power"); return Task.FromResult("Perfil seleccionado. Pulsa Aplicar para guardar."); });
        var title=Text("Mejoras en el rendimiento del sistema",17,"#E1E7F0"); title.VerticalAlignment=VerticalAlignment.Center; header.Children.Add(title); panel.Children.Add(Surface(header));
        var cards=new UniformGrid { Columns=3, Margin=new Thickness(-12,12,-12,0) }; panel.Children.Add(cards);
        var active=SystemBoost.Load().Active;
        foreach(var option in options)
        {
            option.Selected=active.Contains(option.Id);
            var card=new Grid { Margin=new Thickness(22) }; card.RowDefinitions.Add(new RowDefinition { Height=new GridLength(54) }); card.RowDefinitions.Add(new RowDefinition { Height=GridLength.Auto }); card.RowDefinitions.Add(new RowDefinition()); card.RowDefinitions.Add(new RowDefinition { Height=new GridLength(30) });
            var icon=Text(Icon(option.Id),30,"#8795A7"); icon.FontFamily=new FontFamily("Segoe MDL2 Assets"); card.Children.Add(icon);
            var toggle=new CheckBox { Template=SwitchTemplate(), HorizontalAlignment=HorizontalAlignment.Right, VerticalAlignment=VerticalAlignment.Top, IsChecked=option.Selected, Cursor=System.Windows.Input.Cursors.Hand, ToolTip=option.Title }; toggle.Checked+=(_,_)=>option.Selected=true; toggle.Unchecked+=(_,_)=>option.Selected=false; option.PropertyChanged+=(_,_)=>toggle.IsChecked=option.Selected; card.Children.Add(toggle);
            var heading=Text(option.Title,16,"#E4EAF3"); heading.FontWeight=FontWeights.SemiBold; heading.TextWrapping=TextWrapping.NoWrap; heading.TextTrimming=TextTrimming.CharacterEllipsis; heading.ToolTip=option.Title; heading.Margin=new Thickness(0,4,0,9); Grid.SetRow(heading,1); card.Children.Add(heading);
            var description=Text(option.Description,14,"#8B9BAE"); description.MaxHeight=62; Grid.SetRow(description,2); card.Children.Add(description);
            var link=Text(option.Advanced?"Precauciones y detalles…":"Ver detalles…",13,"#C6D0DE"); link.TextDecorations=TextDecorations.Underline; link.VerticalAlignment=VerticalAlignment.Bottom; link.Cursor=System.Windows.Input.Cursors.Hand; link.MouseLeftButtonUp+=(_,_)=>status.Text=option.Description; Grid.SetRow(link,3); card.Children.Add(link);
            cards.Children.Add(new Border { Child=card, Background=Brush("#111316"), CornerRadius=new CornerRadius(7), Height=220, Margin=new Thickness(12,0,12,24) });
        }
        panel.Children.Add(Text("Unreal Engine no corresponde al motor de MU. Los ajustes avanzados se aplican solo al seleccionarlos.",13,"#8898AA"));
        var actions=new StackPanel { Orientation=Orientation.Horizontal }; panel.Children.Add(actions);
        Action(actions,"Aplicar ajustes",async()=>{ await SystemBoost.Apply(options.Where(o=>o.Selected).Select(o=>o.Id).ToHashSet(),Executables); return "Ajustes aplicados. Vuelve a abrir MU para usar la preferencia gráfica."; });
        Action(actions,"Restaurar anteriores",async()=>{ await SystemBoost.Apply(new HashSet<string>(),Executables); options.ForEach(o=>o.Selected=false); return "Ajustes anteriores restaurados."; });
    }
    void BuildMemory(StackPanel panel) => panel.Children.Add(new RamBoostView(() => Executables));
    void BuildDns(StackPanel panel) => panel.Children.Add(new DnsBoostView());
    static Border Surface(UIElement child)=>new(){Child=child,Background=Brush("#111316"),CornerRadius=new CornerRadius(7),Padding=new Thickness(24),Margin=new Thickness(0,12,0,12)};
    static TextBlock Text(string text,double size,string color)=>new(){Text=text,FontSize=size,Foreground=Brush(color),TextWrapping=TextWrapping.Wrap};
    static SolidColorBrush Brush(string color)=>(SolidColorBrush)new BrushConverter().ConvertFromString(color)!;
    static string Icon(string id)=>id switch{"gpu"=>"\uE7F4","keyboard"=>"\uE765","capture"=>"\uE714","power"=>"\uE945","cores"=>"\uE950","search"=>"\uE721","sysmain"=>"\uE964","counters"=>"\uE9D9","lastaccess"=>"\uE8A5",_=>"\uE7FC"};
    static ControlTemplate Parse(string xml)=>(ControlTemplate)XamlReader.Parse(xml);
    static ControlTemplate SwitchTemplate()=>Parse("""
    <ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" TargetType="CheckBox"><Border x:Name="track" Width="36" Height="20" CornerRadius="10" Background="#242830"><Ellipse x:Name="dot" Width="15" Height="15" Fill="#E4E7EC" HorizontalAlignment="Left" Margin="3,0"/></Border><ControlTemplate.Triggers><Trigger Property="IsChecked" Value="True"><Setter TargetName="track" Property="Background" Value="#B78D3B"/><Setter TargetName="dot" Property="HorizontalAlignment" Value="Right"/></Trigger></ControlTemplate.Triggers></ControlTemplate>
    """);
    static StackPanel Page(TabControl tabs,string title){var content=new StackPanel();tabs.Items.Add(new TabItem {Header=title,Content=content,Template=Parse("""
    <ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" TargetType="TabItem"><Border x:Name="tab" Background="#0D0F12" Padding="22,14" CornerRadius="6,6,0,0"><ContentPresenter ContentSource="Header" TextElement.Foreground="{TemplateBinding Foreground}"/></Border><ControlTemplate.Triggers><Trigger Property="IsSelected" Value="True"><Setter TargetName="tab" Property="Background" Value="#171A1F"/><Setter Property="Foreground" Value="#E8ECF2"/></Trigger></ControlTemplate.Triggers></ControlTemplate>
    """),Foreground=Brush("#8E99A8")});return content;}
    void Action(StackPanel parent,string label,Func<Task<string>> action){var button=new Button {Content=label,Margin=new Thickness(6),Padding=new Thickness(18,12,18,12),Background=Brush("#1B1E24"),Foreground=Brush("#DCE2EB"),BorderBrush=Brush("#42403A")};parent.Children.Add(button);button.Click+=async(_,_)=>{button.IsEnabled=false;try{status.Text="Procesando…";status.Text=await action();}catch(Exception e){status.Text="No se completó: "+e.Message+" Puedes restaurar los cambios guardados.";}finally{button.IsEnabled=true;}};}
}
