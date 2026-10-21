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
        string destination = DefaultDirectory;
        if (string.Equals(source.TrimEnd(Path.DirectorySeparatorChar), destination, StringComparison.OrdinalIgnoreCase)) return false;
        UpdatePaths.RejectLinks(destination, destination);
        Directory.CreateDirectory(destination);
        string launcher = Path.Combine(destination, "Launcher.exe");
        if (ClientProcess.IsRunning(destination, new LauncherSettings())) throw new IOException("Cierra Paris Project antes de instalar el launcher.");
        File.Copy(Environment.ProcessPath!, launcher, overwrite: true);
        if (!File.Exists(Path.Combine(destination, "launcher.json"))) new LauncherSettings().Save(destination);
        DefenderExclusion.Offer();
        Process.Start(new ProcessStartInfo(launcher, "--choose") { WorkingDirectory = destination, UseShellExecute = true });
        return true;
    }
}
