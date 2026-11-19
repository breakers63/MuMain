using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace ParisLauncher.Views;

public static class ServerGameEmblem
{
    private const int FrameSize = 40;
    private const int IconSize = 32;
    private static readonly ImageSource MuLogo = Load("mu-classic-transparent.png");
    private static readonly ImageSource CounterStrikeLogo = Load("cs-classic-transparent.png");

    public static FrameworkElement Mu() => Frame(new Image
    {
        Source = MuLogo,
        Stretch = Stretch.Uniform,
    });

    public static FrameworkElement CounterStrike() => Frame(new Border
    {
        Background = Brushes.WhiteSmoke,
        OpacityMask = new ImageBrush(CounterStrikeLogo) { Stretch = Stretch.Uniform },
    });

    private static ImageSource Load(string file)
    {
        var bitmap = new BitmapImage(new Uri($"pack://application:,,,/Launcher;component/Assets/{file}"));
        bitmap.Freeze();
        return bitmap;
    }

    private static Border Frame(FrameworkElement icon)
    {
        icon.Width = IconSize;
        icon.Height = IconSize;
        icon.HorizontalAlignment = HorizontalAlignment.Center;
        icon.VerticalAlignment = VerticalAlignment.Center;
        RenderOptions.SetBitmapScalingMode(icon, BitmapScalingMode.HighQuality);

        return new Border
        {
            Child = icon,
            Width = FrameSize,
            Height = FrameSize,
            Margin = new Thickness(0, 0, 9, 0),
            CornerRadius = new CornerRadius(5),
            Background = new SolidColorBrush(Color.FromRgb(27, 30, 34)),
            VerticalAlignment = VerticalAlignment.Center,
            UseLayoutRounding = true,
        };
    }
}
