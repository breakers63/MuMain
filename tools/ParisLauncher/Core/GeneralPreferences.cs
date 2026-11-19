using System.IO;
using System.Text.Json;
using Microsoft.Win32;
namespace ParisLauncher.Core;
public sealed class GeneralPreferences
{
    public bool StartWithWindows { get; set; }
    public bool CloseToTray { get; set; }
    public bool ConnectionAlerts { get; set; }
    public bool ServerAlerts { get; set; } = true;
    public bool HardwareMonitor { get; set; }
    public int WindowWidth { get; set; } = 1600;
    public int WindowHeight { get; set; } = 950;
    public int ProbeSeconds { get; set; } = 2;
    private static string FilePath=>Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"ParisProject","Launcher","general.json");
    public static GeneralPreferences Load(){try{return JsonSerializer.Deserialize<GeneralPreferences>(File.ReadAllText(FilePath))??new();}catch(Exception e)when(e is IOException or JsonException){return new();}}
    public void Save(){Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);File.WriteAllText(FilePath+".tmp",JsonSerializer.Serialize(this));File.Move(FilePath+".tmp",FilePath,true);}
    public static void SetStartup(bool enabled,string executable)
    {
        if(!Path.IsPathFullyQualified(executable)||!File.Exists(executable))throw new IOException("Ejecutable del launcher inválido.");
        using var key=Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run");
        if(enabled)key.SetValue("ParisProjectLauncher","\""+executable+"\" --choose");else key.DeleteValue("ParisProjectLauncher",false);
    }
}
