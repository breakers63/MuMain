using System.Windows;
using System.Windows.Controls;

namespace ParisLauncher.Views;

public partial class ParisVpnView : UserControl
{
    private sealed record Region(string Name, string Code, bool Default = false)
    {
        public Uri Flag => new($"pack://application:,,,/Launcher;component/Assets/Flags/{Code}.png");
    }

    public ParisVpnView()
    {
        InitializeComponent();
        Regions.ItemsSource = new Region[]
        {
            new("Argentina", "ar"),
            new("Brasil", "br", Default: true),
            new("Canadá", "ca"),
            new("Chile", "cl"),
        };
        SelectedRegion.Text = "Región · Brasil";
    }

    public void UpdateAccess(int? tier)
    {
        bool vip = tier is >= 1 and <= 3;
        AccessBadge.Text = vip ? $"ACCESO VIP · VIP {tier}" : "ACCESO VIP";
        ConnectionNote.Text = tier switch
        {
            >= 1 and <= 3 => "Configuración de nodos pendiente.",
            0 => "Necesitas un VIP activo para conectarte.",
            _ => "No se pudo comprobar el acceso VIP.",
        };
    }

    private void RegionChecked(object sender, RoutedEventArgs e)
    {
        if (sender is RadioButton { Tag: string region })
            SelectedRegion.Text = $"Región · {region}";
    }
}
