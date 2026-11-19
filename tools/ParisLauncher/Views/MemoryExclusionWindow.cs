using System.IO;
using System.Windows;
using System.Windows.Controls;
using ParisLauncher.Performance;
using Microsoft.Win32;
namespace ParisLauncher.Views;
public sealed class MemoryExclusionWindow : Window
{
    readonly List<CheckBox> rows=[];
    readonly HashSet<string> selected=new(StringComparer.OrdinalIgnoreCase);
    public IEnumerable<string> SelectedPaths=>selected;
    public MemoryExclusionWindow()
    {
        Title="Agregar aplicación · PARIS PROJECT";Width=760;Height=650;MinWidth=600;MinHeight=450;WindowStartupLocation=WindowStartupLocation.CenterOwner;Background=RamBoostView.Color("#101216");Foreground=RamBoostView.Color("#E1E8F2");
        var root=new DockPanel{Margin=new Thickness(28)};Content=root;
        var top=new StackPanel();DockPanel.SetDock(top,Dock.Top);root.Children.Add(top);top.Children.Add(RamBoostView.Label("Agregar aplicación",23));var hint=RamBoostView.Label("Las aplicaciones elegidas quedarán excluidas de la limpieza.");hint.Margin=new Thickness(0,10,0,22);top.Children.Add(hint);
        var tabs=new StackPanel{Orientation=Orientation.Horizontal};top.Children.Add(tabs);var processes=RamBoostView.Button("Procesos en ejecución");tabs.Children.Add(processes);var browse=RamBoostView.Button("Examinar archivo");tabs.Children.Add(browse);
        var search=new TextBox{Margin=new Thickness(0,18,0,16),Padding=new Thickness(12),ToolTip="Buscar por nombre o ruta"};top.Children.Add(search);
        var bottom=new StackPanel{Orientation=Orientation.Horizontal,HorizontalAlignment=HorizontalAlignment.Right,Margin=new Thickness(0,20,0,0)};DockPanel.SetDock(bottom,Dock.Bottom);root.Children.Add(bottom);var cancel=RamBoostView.Button("Cancelar");cancel.Click+=(_,_)=>DialogResult=false;bottom.Children.Add(cancel);var accept=RamBoostView.Button("Agregar seleccionadas");accept.Click+=(_,_)=>{foreach(var row in rows.Where(r=>r.IsChecked==true))selected.Add((string)row.Tag);if(selected.Count>0)DialogResult=true;};bottom.Children.Add(accept);
        var list=new StackPanel();root.Children.Add(new ScrollViewer{Content=list,VerticalScrollBarVisibility=ScrollBarVisibility.Auto});
        void Load(){list.Children.Clear();rows.Clear();foreach(var app in MemoryTools.Applications()){var row=new CheckBox{Content=app.Name+"  ("+Path.GetFileName(app.Path)+")",Tag=app.Path,Foreground=RamBoostView.Color("#CED8E6"),Margin=new Thickness(6,12,6,12),ToolTip=app.Path};rows.Add(row);list.Children.Add(row);}}Load();processes.Click+=(_,_)=>Load();
        search.TextChanged+=(_,_)=>{foreach(var row in rows)row.Visibility=((string)row.Tag).Contains(search.Text,StringComparison.OrdinalIgnoreCase)?Visibility.Visible:Visibility.Collapsed;};
        browse.Click+=(_,_)=>{var dialog=new OpenFileDialog{Title="Excluir aplicación de la limpieza",Filter="Aplicaciones (*.exe)|*.exe",Multiselect=true,CheckFileExists=true};if(dialog.ShowDialog(this)==true){foreach(var path in dialog.FileNames)selected.Add(Path.GetFullPath(path));DialogResult=true;}};
    }
}
