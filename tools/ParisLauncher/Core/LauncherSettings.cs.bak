using System.IO;
using System.Text.Json;

namespace ParisLauncher.Core;

public sealed class LauncherSettings
{
    public string ModernExecutable { get; set; } = "Jugar.exe";
    public string ClassicExecutable { get; set; } = "Classic/Jugar.exe";
    public string ManifestUrl { get; set; } = "https://patch.hispanogaming.com/windows/manifest.json";
    public bool DefenderPromptShown { get; set; }
    public static JsonSerializerOptions JsonOptions { get; } = new() { WriteIndented = true, PropertyNameCaseInsensitive = true };

    public static LauncherSettings Load(string root)
    {
        string path = Path.Combine(root, "launcher.json");
        if (!File.Exists(path))
        {
            var settings = new LauncherSettings();
            settings.Save(root);
            return settings;
        }
        return JsonSerializer.Deserialize<LauncherSettings>(File.ReadAllText(path), JsonOptions)
            ?? throw new InvalidDataException("La configuración del launcher está vacía.");
    }

    public void Save(string root) => File.WriteAllText(Path.Combine(root, "launcher.json"), JsonSerializer.Serialize(this, JsonOptions));

    public string Executable(string root, bool classic) => Path.GetFullPath(Path.Combine(root, classic ? ClassicExecutable : ModernExecutable));
}
