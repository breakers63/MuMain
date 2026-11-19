using System.Windows;
using System.Windows.Controls;
using ParisLauncher.Core;
namespace ParisLauncher;
public partial class MainWindow
{
    private string? networkHost;
    private readonly Queue<string> networkRecords=[];
    private void NetworkTab(object sender,RoutedEventArgs e)
    {
        string? tab=((Button)sender).Tag as string;
        NetworkMonitorBody.Visibility=tab=="monitor"?Visibility.Visible:Visibility.Collapsed;
        NetworkDiagnosticBody.Visibility=tab=="diagnostic"?Visibility.Visible:Visibility.Collapsed;
        NetworkHistoryBody.Visibility=tab=="history"?Visibility.Visible:Visibility.Collapsed;
    }
    private async void NetworkTarget(object sender,RoutedEventArgs e)
    {
        if(measuring)return;
        networkHost=(((Button)sender).Tag as string) switch {"google"=>"www.google.com","cloudflare"=>"www.cloudflare.com",_=>null};
        latencyHistory.Clear();LatencyLine.Points.Clear();ServerAddress.Text=networkHost is null?$"{settings.ServerHost}:{settings.ServerPort} · MU Paris":$"{networkHost}:443 · conexión HTTPS / TCP";
        await MeasureServer();
    }
    private void ToggleNetworkMonitor(object sender,RoutedEventArgs e)
    {
        if(latencyTimer is null)return;
        if(latencyTimer.IsEnabled){latencyTimer.Stop();MonitorToggle.Content="Reanudar";}else{latencyTimer.Start();MonitorToggle.Content="Pausar";}
    }
    private void RecordNetwork(LatencyResult value)
    {
        string destination=networkHost??settings.ServerHost;
        networkRecords.Enqueue($"{DateTime.Now:HH:mm:ss} · {destination} · TCP {value.MedianMs?.ToString("0.0")??"—"} ms · jitter {value.JitterMs?.ToString("0.0")??"—"} ms · {value.Received}/{value.Sent} respuestas");
        while(networkRecords.Count>60)networkRecords.Dequeue();NetworkHistory.Text=string.Join("\n\n",networkRecords.Reverse());
    }
}
