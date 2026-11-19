using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace ParisLauncher.Views;

public partial class WelcomeWindow : Window
{
    private int step;
    public string Goal { get; private set; } = "home";
    public WelcomeWindow()
    {
        InitializeComponent();
        Width = Math.Min(Width, SystemParameters.WorkArea.Width - 30);
        Height = Math.Min(Height, SystemParameters.WorkArea.Height - 30);
    }
    private void ChooseGoal(object sender, RoutedEventArgs e) => Goal = ((RadioButton)sender).Tag.ToString() ?? "home";
    private void DragTitle(object sender, MouseButtonEventArgs e) { if (e.ChangedButton == MouseButton.Left) DragMove(); }
    private void CloseWelcome(object sender, RoutedEventArgs e) => Close();
    private void Skip(object sender, RoutedEventArgs e) => DialogResult = true;
    private void Next(object sender, RoutedEventArgs e) { if (step == 2) { DialogResult = true; return; } step++; Refresh(); }
    private void Back(object sender, RoutedEventArgs e) { step = Math.Max(0, step - 1); Refresh(); }
    private void Refresh()
    {
        StepOne.Visibility = step == 0 ? Visibility.Visible : Visibility.Collapsed;
        StepTwo.Visibility = step == 1 ? Visibility.Visible : Visibility.Collapsed;
        StepThree.Visibility = step == 2 ? Visibility.Visible : Visibility.Collapsed;
        StepLabel.Text = $"0{step + 1} / 03";
        BackButton.Visibility = step == 0 ? Visibility.Collapsed : Visibility.Visible;
        NextButton.Content = step == 2 ? "FINALIZAR →" : "SIGUIENTE →";
    }
}
