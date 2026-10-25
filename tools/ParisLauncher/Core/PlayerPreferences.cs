using System.IO;
using System.Text.Json;

namespace ParisLauncher.Core;

public sealed class PlayerPreferences
{
    public bool Classic { get; set; }
    public string Backend { get; set; } = "vulkan";
    public bool Remember { get; set; }
    private static string FilePath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Launcher", "preferences.json");

    public static PlayerPreferences Load()
    {
        try { return JsonSerializer.Deserialize<PlayerPreferences>(File.ReadAllText(FilePath)) ?? new(); }
        catch (IOException) { return new(); }
        catch (JsonException) { return new(); }
    }

    public void Save()
    {
        Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
        File.WriteAllText(FilePath, JsonSerializer.Serialize(this));
    }
}
