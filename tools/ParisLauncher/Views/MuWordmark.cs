using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace ParisLauncher.Views;

public sealed class MuWordmark : Image
{
    public MuWordmark()
    {
        Source = new BitmapImage(new Uri("pack://application:,,,/Launcher;component/Assets/mu-wordmark-exact.png"));
        Stretch = Stretch.Uniform;
        RenderOptions.SetBitmapScalingMode(this, BitmapScalingMode.HighQuality);
    }
}
