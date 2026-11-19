using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace ParisLauncher;

public partial class MainWindow
{
    private Border MembershipCard()
    {
        var panel = new StackPanel();
        panel.Children.Add(new TextBlock { Text = "Detalles de la cuenta", FontSize = 17, Foreground = Brushes.White, Margin = new Thickness(0, 0, 0, 30) });
        var state = new TextBlock { Text = profile is null ? "Estado no disponible" : profile.VipTier == 0 ? "Sin suscripción" : profile.Status, TextAlignment = TextAlignment.Center, Foreground = Brushes.White, FontSize = 16, Margin = new Thickness(0, 15, 0, 14) };
        panel.Children.Add(state);
        panel.Children.Add(new TextBlock { Text = profile is null ? "No se pudo consultar el estado de la cuenta." : profile.VipTier == 0 ? "No tienes un plan activo." : profile.Expiry, TextAlignment = TextAlignment.Center, Foreground = new SolidColorBrush(Color.FromRgb(135, 155, 178)), Margin = new Thickness(0, 0, 0, 16) });
        if (profile?.VipTier == 0)
        {
            var subscribe = new Button { Content = "Suscríbete ahora", Background = new SolidColorBrush(Color.FromRgb(246, 39, 46)), Foreground = Brushes.White, BorderBrush = Brushes.Transparent, HorizontalAlignment = HorizontalAlignment.Center, Padding = new Thickness(20, 13, 20, 13) };
            subscribe.Click += (_, _) => OpenHelpLink(settings.VipPurchaseUrl);
            panel.Children.Add(subscribe);
        }
        return new Border { MinHeight = 245, VerticalAlignment = VerticalAlignment.Top, Child = panel, Padding = new Thickness(24), Margin = new Thickness(0, 24, 0, 0), BorderBrush = new SolidColorBrush(Color.FromRgb(50, 58, 70)), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(7) };
    }

    private void OpenHelpCards()
    {
        var content = new StackPanel();
        content.Children.Add(new TextBlock { Text = "Obtén ayuda a través de los siguientes canales", Foreground = new SolidColorBrush(Color.FromRgb(142, 159, 180)), Margin = new Thickness(0, 12, 0, 4) });
        content.Children.Add(HelpTile("?", "FAQ", "Preguntas frecuentes", OpenFrequentlyAsked, false));
        content.Children.Add(HelpTile("\uE8F2", "Soporte", "Habla con soporte", () => OpenHelpLink(settings.SupportUrl)));
        content.Children.Add(HelpTile("\uE902", "Discord", "Únete a la comunidad", () => OpenHelpLink(settings.DiscordUrl)));
        content.Children.Add(HelpTile("\uE774", "Web oficial", "Visita MU Paris", () => OpenHelpLink(settings.WebsiteUrl)));
        OpenHeaderDrawer("Centro de ayuda", content);
    }

    private void OpenFrequentlyAsked()
    {
        var content = new StackPanel();
        content.Children.Add(HeaderCard("Cuenta y acceso", "Usa tu cuenta de MU Paris para entrar al launcher y al juego."));
        content.Children.Add(HeaderCard("PC Boost", "Selecciona los cambios y pulsa Aplicar. Puedes restaurar los valores anteriores."));
        content.Children.Add(HeaderCard("Red y DNS", "Las mediciones no cambian la ruta del juego. Elegir un DNS es una acción separada y conserva un respaldo."));
        OpenHeaderDrawer("Preguntas frecuentes", content);
    }

    private Button HelpTile(string icon, string title, string subtitle, Action action, bool external = true)
    {
        var grid = new Grid { Width = 450 };
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(48) });
        grid.ColumnDefinitions.Add(new ColumnDefinition());
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(28) });
        var symbol = new TextBlock { Text = icon, FontSize = 22, FontFamily = new FontFamily(icon == "?" ? "Segoe UI" : "Segoe MDL2 Assets"), Foreground = Brushes.White, HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
        grid.Children.Add(new Border { Child = symbol, Width = 36, Height = 38, Background = new SolidColorBrush(Color.FromRgb(28, 33, 41)), CornerRadius = new CornerRadius(4) });
        var labels = new StackPanel { VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(8, 0, 0, 0) };
        labels.Children.Add(new TextBlock { Text = title, FontSize = 15, Foreground = Brushes.White });
        labels.Children.Add(new TextBlock { Text = subtitle, FontSize = 14, Foreground = new SolidColorBrush(Color.FromRgb(135, 155, 178)), Margin = new Thickness(0, 7, 0, 0) });
        Grid.SetColumn(labels, 1); grid.Children.Add(labels);
        var arrow = new TextBlock { Text = external ? "↗" : "›", FontSize = 23, Foreground = Brushes.White, VerticalAlignment = VerticalAlignment.Center, HorizontalAlignment = HorizontalAlignment.Right };
        Grid.SetColumn(arrow, 2); grid.Children.Add(arrow);
        var button = new Button { Content = grid, Height = 94, Margin = new Thickness(0, 22, 0, 0), Padding = new Thickness(22, 16, 22, 16), Background = new SolidColorBrush(Color.FromRgb(17, 20, 25)), BorderBrush = new SolidColorBrush(Color.FromRgb(50, 59, 71)) };
        button.Click += (_, _) => action();
        return button;
    }

    private void OpenHelpLink(string address)
    {
        if (!Uri.TryCreate(address, UriKind.Absolute, out var uri) || uri.Scheme != "https") { MessageBox.Show(this, "El enlace no está configurado correctamente.", "PARIS PROJECT"); return; }
        try { Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true }); }
        catch (Exception error) when (error is System.ComponentModel.Win32Exception or InvalidOperationException) { MessageBox.Show(this, "No se pudo abrir el navegador.", "PARIS PROJECT"); }
    }
}
