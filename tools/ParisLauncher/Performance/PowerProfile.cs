using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;

namespace ParisLauncher.Performance;

public static class PowerProfile
{
    private static readonly string Backup = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Launcher", "power-plan.txt");
    public static async Task Enable()
    {
        string output = await Run("/getactivescheme");
        var match = Regex.Match(output, @"[a-fA-F0-9]{8}-(?:[a-fA-F0-9]{4}-){3}[a-fA-F0-9]{12}");
        if (!match.Success) throw new IOException("No se pudo identificar el plan de energía actual.");
        if (!File.Exists(Backup)) { Directory.CreateDirectory(Path.GetDirectoryName(Backup)!); await File.WriteAllTextAsync(Backup, match.Value); }
        await Run("/setactive SCHEME_MIN");
    }
    public static async Task Restore()
    {
        if (!File.Exists(Backup)) return;
        if (!Guid.TryParse(await File.ReadAllTextAsync(Backup), out var id)) throw new IOException("Respaldo de energía no válido.");
        await Run("/setactive " + id);
        File.Delete(Backup);
    }
    private static async Task<string> Run(string arguments)
    {
        using var process = Process.Start(new ProcessStartInfo("powercfg.exe", arguments) { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true }) ?? throw new IOException("No se pudo abrir powercfg.");
        string output = await process.StandardOutput.ReadToEndAsync();
        string error = await process.StandardError.ReadToEndAsync();
        await process.WaitForExitAsync();
        if (process.ExitCode != 0) throw new IOException("Windows no permitió cambiar el plan de energía. " + error.Trim());
        return output;
    }
}
