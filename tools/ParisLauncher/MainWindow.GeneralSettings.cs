using System.Windows;
using ParisLauncher.Core;
namespace ParisLauncher;
public partial class MainWindow
{
    private GeneralPreferences general=GeneralPreferences.Load();
    private LauncherTray? tray;
    private bool exitRequested;
    private bool generalHooksRegistered;
    private bool? lastServerOnline;
    private void ConfigureGeneralSettings()
    {
        SettingsDashboard.Configure(general,SaveGeneralSettings,value=>GeneralPreferences.SetStartup(value,Environment.ProcessPath!),ResetGeneralSettings,()=>NavBoost.IsChecked=true,()=>NavBoost.IsChecked=true);
        ApplyGeneralWindowSize();NetworkWorkbench.SetProbeSeconds(general.ProbeSeconds);if(!generalHooksRegistered){generalHooksRegistered=true;Closed+=(_,_)=>tray?.Dispose();}
    }
    private void SaveGeneralSettings(){general.Save();ApplyGeneralWindowSize();NetworkWorkbench.SetProbeSeconds(general.ProbeSeconds);if(general.CloseToTray&&IsLoaded)EnsureTray();}
    private void ApplyGeneralWindowSize(){Width=Math.Clamp(general.WindowWidth,MinWidth,Math.Max(MinWidth,SystemParameters.WorkArea.Width-30));Height=Math.Clamp(general.WindowHeight,MinHeight,Math.Max(MinHeight,SystemParameters.WorkArea.Height-30));}
    private void ResetGeneralSettings(){try{GeneralPreferences.SetStartup(false,Environment.ProcessPath!);general=new GeneralPreferences();general.Save();ConfigureGeneralSettings();}catch(Exception error){MessageBox.Show(this,error.Message,"PARIS PROJECT");}}
    private void EnsureTray(){try{tray??=new LauncherTray(this);}catch(Exception error){UpdateStatus.Text=error.Message;}}
    private bool HideToTray(){EnsureTray();if(tray is null)return false;Hide();return true;}
    private void HeaderCloseClick(object sender,RoutedEventArgs e){if(general.CloseToTray&&HideToTray())return;ExitClick(sender,e);}
    private void NotifyServerStatus(bool online)
    {
        bool changed=lastServerOnline.HasValue&&lastServerOnline!=online;lastServerOnline=online;if(!changed||!general.ServerAlerts||online)return;EnsureTray();tray?.Notify("MU Paris","El servidor no respondió a la última medición. Revisa tu conexión o su estado.");
    }
}
