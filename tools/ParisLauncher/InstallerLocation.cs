using System.Diagnostics;
using System.IO;
using System.Text.Json;
using ParisLauncher.Core;

namespace ParisLauncher;

public static class InstallerLocation
{
    public const string DefaultDirectory = @"C:\PARIS PROJECT";

    public static bool RelocateFreshDownload()
    {
        string source = AppContext.BaseDirectory;
        if (File.Exists(Path.Combine(source, "Jugar.exe"))) return false;
        string configuration = Path.Combine(source, "launcher.json");
        if (File.Exists(configuration))
        {
            var existing = JsonSerializer.Deserialize<LauncherSettings>(File.ReadAllText(configuration), LauncherSettings.JsonOptions);
            if (existing != null && File.Exists(existing.Executable(source, false))) return false;
        }
        if (IsDefaultDirectory(source)) return false;
        OpenDefaultInstallation();
        return true;
    }

    public static bool IsDefaultDirectory(string path) =>
        string.Equals(Path.TrimEndingDirectorySeparator(Path.GetFullPath(path)), DefaultDirectory, StringComparison.OrdinalIgnoreCase);

    public static void OpenDefaultInstallation()
    {
        string destination = DefaultDirectory;
        UpdatePaths.RejectLinks(destination, destination);
        Directory.CreateDirectory(destination);
        string launcher = Path.Combine(destination, "Launcher.exe");
        File.Copy(Environment.ProcessPath!, launcher, overwrite: true);
        var settings = LauncherSettings.Load(destination);
        if (!File.Exists(Path.Combine(destination, "launcher.json"))) settings.Save(destination);
        InstallationPrompts.OfferDefenderOnce(destination, settings, DefenderExclusion.Offer);
        Process.Start(new ProcessStartInfo(launcher, "--choose") { WorkingDirectory = destination, UseShellExecute = true });
    }
}
