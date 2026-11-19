using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
namespace ParisLauncher.Views;
public sealed class CounterStrikeWordmark : Viewbox
{
    public CounterStrikeWordmark()
    {
        var grid=new Grid{Width=400,Height=88};
        var name=new StackPanel{Orientation=Orientation.Horizontal,HorizontalAlignment=HorizontalAlignment.Left};
        name.Children.Add(Lettering("Counter",36));
        var figure = new Viewbox { Width = 40, Height = 45, Margin = new Thickness(3, 0, 2, 0) };
        figure.Child = new Path
        {
            Data = Geometry.Parse("F1 M13,7 C11,3 15,0 19,1 C22,2 24,5 22,9 L20,12 L24,14 L30,11 L37,11 L37,9 L41,9 L41,12 L49,12 L49,15 L38,15 L36,20 L33,19 L32,16 L25,20 L20,18 L18,27 L23,32 L27,42 L23,45 L18,36 L13,32 L9,38 L6,46 L1,45 L3,36 L8,27 L9,17 L5,24 L2,22 L6,13 L12,10 Z"),
            Fill = Brushes.WhiteSmoke,
            Stretch = Stretch.Uniform,
        };
        name.Children.Add(figure);name.Children.Add(Lettering("Strike 1.6",36));grid.Children.Add(name);
        var subtitle=new Grid{Width=260,Height=28,VerticalAlignment=VerticalAlignment.Bottom,HorizontalAlignment=HorizontalAlignment.Left,Margin=new Thickness(20,0,0,0)};
        subtitle.ColumnDefinitions.Add(new ColumnDefinition());subtitle.ColumnDefinitions.Add(new ColumnDefinition{Width=new GridLength(160)});subtitle.ColumnDefinitions.Add(new ColumnDefinition());
        var gold=new SolidColorBrush(Color.FromRgb(231,199,119));
        subtitle.Children.Add(new Border{Height=1,Background=gold,VerticalAlignment=VerticalAlignment.Center});
        var paris=new TextBlock{Text="P A R I S",FontFamily=new FontFamily("Georgia"),FontSize=24,Foreground=gold,TextAlignment=TextAlignment.Center};Grid.SetColumn(paris,1);subtitle.Children.Add(paris);
        var rule=new Border{Height=1,Background=gold,VerticalAlignment=VerticalAlignment.Center};Grid.SetColumn(rule,2);subtitle.Children.Add(rule);grid.Children.Add(subtitle);
        Child=grid;Stretch=Stretch.Uniform;
    }
    private static TextBlock Lettering(string text,double size)=>new(){Text=text,FontFamily=new FontFamily("Bahnschrift SemiCondensed"),FontWeight=FontWeights.Bold,FontSize=size,Foreground=new SolidColorBrush(Color.FromRgb(244,236,217))};
}
