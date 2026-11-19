using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

namespace ParisLauncher.Performance;

public sealed class IniProfile
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] private static extern uint GetPrivateProfileString(string section, string key, string fallback, StringBuilder value, uint size, string path);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] private static extern bool WritePrivateProfileString(string section, string key, string? value, string path);
    private sealed record Original(string File, string Section, string Key, string? Value);
    private const string MissingValue = "__PARIS_MISSING__";
    private readonly string directory;
    private readonly string backup;
    public IniProfile(string directory)
    {
        this.directory = Path.GetFullPath(directory);
        backup = Path.Combine(this.directory, "paris-launcher-profile-backup.json");
    }

    public void Apply(bool classic, string profile, bool shadows, bool effects, bool weather, bool fps, bool? environment = null, bool? vsync = null)
    {
        if (!Directory.Exists(directory)) throw new DirectoryNotFoundException("Primero instala el cliente.");
        var entries = File.Exists(backup) ? JsonSerializer.Deserialize<List<Original>>(File.ReadAllText(backup))! : new List<Original>();
        void Set(string file, string section, string key, string value)
        {
            if (!entries.Any(e => e.File == file && e.Section == section && e.Key == key))
            {
                var text = new StringBuilder(1024);
                GetPrivateProfileString(section, key, MissingValue, text, 1024, Path.Combine(directory, file));
                entries.Add(new(file, section, key, text.ToString() == MissingValue ? null : text.ToString()));
                File.WriteAllText(backup, JsonSerializer.Serialize(entries));
            }
            if (!WritePrivateProfileString(section, key, value, Path.Combine(directory, file))) throw new IOException("No se pudo guardar el perfil gráfico.");
        }
        Set("config.ini", "Weather", "Enabled", weather ? "1" : "0");
        if (classic)
        {
            Set("Performance.ini", "CrowdDetail", "Enabled", profile == "Calidad" ? "0" : "1");
            Set("Performance.ini", "CrowdDetail", "FullDetailPlayers", profile == "Rendimiento" ? "12" : "24");
            Set("boost.ini", "Boost", "AdaptiveFocus", "1");
            Set("boost.ini", "Boost", "RaisePriorities", "1");
            return;
        }
        Set("config.ini", "Render", "DisableBodyShadows", shadows ? "0" : "1");
        Set("config.ini", "Render", "DisableWingShadow", shadows ? "0" : "1");
        Set("config.ini", "Render", "DisableParticles", effects ? "0" : "1");
        Set("config.ini", "Render", "DisableBoids", (environment ?? profile != "Rendimiento") ? "0" : "1");
        if (vsync.HasValue) Set("config.ini", "Render", "VSync", vsync.Value ? "1" : "0");
        Set("config.ini", "Render", "FpsCap", fps ? "60" : "-1");
        Set("config.ini", "Render", "Antialiasing", profile == "Calidad" ? "4" : profile == "Equilibrado" ? "2" : "0");
        Set("config.ini", "Render", "Anisotropy", profile == "Calidad" ? "8" : "2");
    }

    public void Restore()
    {
        if (!File.Exists(backup)) return;
        foreach (var entry in JsonSerializer.Deserialize<List<Original>>(File.ReadAllText(backup))!)
        {
            if (entry.File is not ("config.ini" or "boost.ini" or "Performance.ini")) throw new InvalidDataException("Respaldo no válido.");
            if (!WritePrivateProfileString(entry.Section, entry.Key, entry.Value, Path.Combine(directory, entry.File))) throw new IOException("No se pudo restaurar el perfil.");
        }
        File.Delete(backup);
    }
}
