using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
namespace ParisLauncher.Views;
public sealed class BandwidthGauge : StackPanel
{
    readonly TextBlock value = RamBoostView.Label("—",30);
    readonly Polyline active = new(){StrokeThickness=10,Stroke=RamBoostView.Color("#D8B35F"),StrokeStartLineCap=PenLineCap.Round,StrokeEndLineCap=PenLineCap.Round};
    public BandwidthGauge(string label)
    {
        HorizontalAlignment=HorizontalAlignment.Center; Margin=new Thickness(4); Width=225;
        var canvas=new Canvas{Width=220,Height=145};
        var track=new Polyline{StrokeThickness=10,Stroke=RamBoostView.Color("#2B323D"),StrokeStartLineCap=PenLineCap.Round,StrokeEndLineCap=PenLineCap.Round};
        for(int i=0;i<=100;i++) track.Points.Add(Arc(i/100d));
        canvas.Children.Add(track);canvas.Children.Add(active);
        value.Width=220;value.TextAlignment=TextAlignment.Center;Canvas.SetTop(value,71);canvas.Children.Add(value);
        var units=RamBoostView.Label("Mbps",13);units.Width=220;units.TextAlignment=TextAlignment.Center;Canvas.SetTop(units,110);canvas.Children.Add(units);Children.Add(canvas);
        var title=RamBoostView.Label(label,15);title.TextAlignment=TextAlignment.Center;Children.Add(title);
    }
    static Point Arc(double fraction)
    {
        double angle=(150+240*fraction)*Math.PI/180;
        return new Point(110+86*Math.Cos(angle),95+86*Math.Sin(angle));
    }
    public void SetValue(double mbps)
    {
        value.Text=mbps.ToString("0.0");active.Points.Clear();
        double fraction=Math.Clamp(Math.Log10(1+mbps)/Math.Log10(10001),0,1);
        for(int i=0;i<=100*fraction;i++) active.Points.Add(Arc(i/100d));
        ToolTip="Escala logarítmica: 0–10.000 Mbps";
    }
}
