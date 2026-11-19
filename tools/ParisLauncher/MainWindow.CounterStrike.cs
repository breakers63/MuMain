using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using ParisLauncher.Performance;
namespace ParisLauncher;
public partial class MainWindow
{
    readonly TextBlock counterStrikeSidebar=new(){FontSize=11,TextWrapping=TextWrapping.Wrap};
    bool queryingCounterStrike;
    private void OpenCounterStrikeDownloads(object sender,RoutedEventArgs e)=>OpenHelpLink("https://cs-unikov.net/");
    private void ConnectCounterStrike(object sender,RoutedEventArgs e)
    {
        try{System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo("steam://connect/23.26.135.135:27015"){UseShellExecute=true});}
        catch(Exception error) when(error is System.ComponentModel.Win32Exception or InvalidOperationException){MessageBox.Show(this,"Necesitas Steam y Counter-Strike instalados para conectar desde este botón.","Paris Project");}
    }
    private async Task RefreshCounterStrike()
    {
        if(queryingCounterStrike)return;queryingCounterStrike=true;
        try
        {
            var data=await CounterStrikeQuery.Read("23.26.135.135",27015,CancellationToken.None);
            string state=data is null?"Sin respuesta A2S":$"Disponible · {data.Milliseconds:0} ms";
            CounterStrikeStatus.Text=data is null?state:$"{state}\n{data.Name}";
            counterStrikeSidebar.Inlines.Clear();
            counterStrikeSidebar.Inlines.Add(new System.Windows.Documents.Run("COUNTER-STRIKE 1.6"){Foreground=Brushes.Wheat});
            counterStrikeSidebar.Inlines.Add(new System.Windows.Documents.LineBreak());
            counterStrikeSidebar.Inlines.Add(new System.Windows.Documents.Run("● "+state){Foreground=data is null?Brushes.SandyBrown:Brushes.LightGreen});
            if(counterStrikeSidebar.Parent is null)
            {
                var host=(StackPanel)Subservers.Parent;
                var line=new DockPanel();line.Children.Add(Views.ServerGameEmblem.CounterStrike());line.Children.Add(counterStrikeSidebar);
                host.Children.Insert(1,new Border{Child=line,Padding=new Thickness(10),Margin=new Thickness(0,0,0,12),Background=new SolidColorBrush(Color.FromRgb(20,22,24)),CornerRadius=new CornerRadius(5)});
            }
        }
        finally{queryingCounterStrike=false;}
    }
}
