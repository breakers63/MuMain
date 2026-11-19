using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
namespace ParisLauncher.Views;
public sealed class ResourceChartCard : Border
{
    private const int Capacity = 60;
    private readonly Queue<double> history = new();
    private readonly TextBlock value = new() { FontSize = 23, Foreground = Brushes.WhiteSmoke, Margin = new Thickness(0, 10, 0, 12) };
    private readonly Polyline line;
    private readonly bool percent;
    public ResourceChartCard(string title, Brush color, bool percent = true)
    {
        this.percent = percent;
        Background = new SolidColorBrush(Color.FromRgb(17, 19, 23)); CornerRadius = new CornerRadius(8); Padding = new Thickness(20); Margin = new Thickness(6);
        var panel = new StackPanel(); Child = panel;
        panel.Children.Add(new TextBlock { Text = title, Foreground = color, FontSize = 16 }); panel.Children.Add(value);
        var canvas = new Canvas { Width = 500, Height = 100, ClipToBounds = true };
        for (int y = 0; y <= 100; y += 25) canvas.Children.Add(new Line { X2 = 500, Y1 = y, Y2 = y, Stroke = Brushes.DarkSlateGray, Opacity = 0.4 });
        line = new Polyline { Stroke = color, StrokeThickness = 2 }; canvas.Children.Add(line);
        panel.Children.Add(new Viewbox { Height = 100, Stretch = Stretch.Fill, Child = canvas });
        panel.Children.Add(new TextBlock { Text = "Últimas 60 muestras · 1 muestra/s", Foreground = Brushes.Gray, Margin = new Thickness(0, 8, 0, 0), FontSize = 12 });
    }
    public void Sample(double? sample, string label)
    {
        value.Text = label;
        if (!sample.HasValue) return;
        history.Enqueue(sample.Value); while (history.Count > Capacity) history.Dequeue();
        double ceiling = percent ? 100 : Math.Max(1, history.Max() * 1.15);
        var points = new PointCollection(); int index = Capacity - history.Count;
        foreach (double point in history) points.Add(new Point(index++ * 500.0 / (Capacity - 1), 100 - point / ceiling * 100));
        line.Points = points;
    }
}
