using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed class WindowsEffectsView : StackPanel
{
    private readonly Dictionary<uint, CheckBox> toggles = [];
    private readonly TextBlock status = new() { Foreground = Brushes.LightGray, TextWrapping = TextWrapping.Wrap, Margin = new Thickness(0, 14, 0, 12) };
    public WindowsEffectsView()
    {
        Children.Add(new TextBlock { Text = "EFECTOS VISUALES DE WINDOWS", Foreground = Brushes.Wheat, FontSize = 20, Margin = new Thickness(0, 24, 0, 12) });
        var actions = new WrapPanel(); Children.Add(actions);
        AddAction(actions, "Mejor rendimiento", () => { foreach (var toggle in toggles.Values) toggle.IsChecked = false; status.Text = "Selección preparada. Pulsa Aplicar."; });
        AddAction(actions, "Mejor apariencia", () => { foreach (var toggle in toggles.Values) toggle.IsChecked = true; status.Text = "Selección preparada. Pulsa Aplicar."; });
        AddAction(actions, "Opciones completas de Windows", () => Process.Start(new ProcessStartInfo(System.IO.Path.Combine(Environment.SystemDirectory, "SystemPropertiesPerformance.exe")) { UseShellExecute = true }));
        var cards = new UniformGrid { Columns = 3 }; Children.Add(cards);
        foreach (var option in WindowsVisualEffects.Options)
        {
            var toggle = new CheckBox { Content = option.Title, Style = (Style)Application.Current.FindResource("ToggleCard"), Margin = new Thickness(6), Foreground = Brushes.WhiteSmoke, FontSize = 15, ToolTip = "ON: efecto activado · OFF: efecto desactivado" };
            toggles.Add(option.Get, toggle); cards.Children.Add(toggle);
        }
        Children.Add(status);
        var save = new WrapPanel(); Children.Add(save);
        AddAction(save, "Aplicar efectos de Windows", () => { WindowsVisualEffects.Apply(toggles.ToDictionary(row => row.Key, row => row.Value.IsChecked == true)); Reload(); status.Text = "Aplicado a Windows. Estado comprobado de nuevo."; });
        AddAction(save, "Restaurar anteriores", () => { WindowsVisualEffects.Restore(); Reload(); status.Text = "Configuración anterior restaurada."; });
        Loaded += (_, _) => Reload();
    }
    private void Reload()
    {
        foreach (var option in WindowsVisualEffects.Options)
        {
            try { toggles[option.Get].IsChecked = WindowsVisualEffects.Read(option); toggles[option.Get].IsEnabled = true; }
            catch (System.ComponentModel.Win32Exception) { toggles[option.Get].IsEnabled = false; toggles[option.Get].ToolTip = "Windows no permite consultar este efecto."; }
        }
    }
    private void AddAction(Panel panel, string title, Action action)
    {
        var button = new Button { Content = title, Margin = new Thickness(0, 4, 12, 12), Padding = new Thickness(18, 12, 18, 12) };
        panel.Children.Add(button);
        button.Click += (_, _) => { try { action(); } catch (Exception error) { status.Text = "No se completó: " + error.Message; } };
    }
}
