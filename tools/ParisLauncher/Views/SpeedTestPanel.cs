using System.Windows;
using System.Windows.Controls;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed class SpeedTestPanel : StackPanel
{
    readonly TextBlock status = RamBoostView.Label("Prepárate para probar tu conexión",18);
    readonly TextBlock connection = RamBoostView.Label("El proveedor y el nodo se detectarán al iniciar.",13);
    readonly BandwidthGauge down = new("Descarga"), up = new("Subida");
    public SpeedTestPanel()
    {
        Children.Add(status);
        var note = RamBoostView.Label("Medición real de descarga y subida. Hasta 240 MB por prueba. Puede competir con el juego.",13);
        note.Margin = new Thickness(0,16,0,24); Children.Add(note);
        var gauges = new Grid(); gauges.ColumnDefinitions.Add(new()); gauges.ColumnDefinitions.Add(new());
        gauges.Children.Add(down); Grid.SetColumn(up,1); gauges.Children.Add(up);
        var card = new Border { Child = gauges, Padding = new Thickness(8,22,8,22), Background = RamBoostView.Color("#15191F"), BorderBrush = RamBoostView.Color("#2D3540"), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(12) };
        Children.Add(card); connection.Margin = new Thickness(0,24,0,0); connection.TextAlignment = TextAlignment.Center; Children.Add(connection);
    }
    public async Task<(double Down,double Up)> Run(CancellationToken token)
    {
        var result = await LiveSpeedTest.Run(new Progress<LiveSpeedTest.Update>(update =>
        {
            status.Text = update.Phase == "Completado" ? "Medición completada" : update.Phase;
            if(update.Connection is not null) connection.Text = update.Connection;
            if(update.Phase == "Descarga") down.SetValue(update.Mbps);
            if(update.Phase == "Subida") up.SetValue(update.Mbps);
        }),token);
        down.SetValue(result.Down); up.SetValue(result.Up); return result;
    }
}
