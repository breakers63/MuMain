using System.IO;
using System.Text.Json;

namespace ParisLauncher.Core;

public sealed class PlayerPreferences
{
    public bool HasSelectedClient { get; set; } = true;
    public bool Classic { get; set; }
    public string Backend { get; set; } = "vulkan";
    public bool Remember { get; set; }
    private static string FilePath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Launcher", "preferences.json");

    public static PlayerPreferences Load()
    {
        try { return JsonSerializer.Deserialize<PlayerPreferences>(File.ReadAllText(FilePath)) ?? new() { HasSelectedClient = false }; }
        catch (IOException) { return new() { HasSelectedClient = false }; }
        catch (JsonException) { return new() { HasSelectedClient = false }; }
    }

    public void Save()
    {
        Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
        File.WriteAllText(FilePath, JsonSerializer.Serialize(this));
    }
}
