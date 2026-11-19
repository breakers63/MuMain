using System.IO;
using System.Text.Json;

namespace ParisLauncher.Performance;

public sealed class BoostPreferences
{
    public List<string> SelectedApplications { get; set; } = [];
    public List<string> ExcludedApplications { get; set; } = [];
    public bool AutomaticMemoryTrim { get; set; }
    public bool TrimOnGameStart { get; set; }
    public int TrimIntervalHours { get; set; } = 12;
    public DateTimeOffset LastTrim { get; set; }
    public bool GamePriority { get; set; }
    public bool BackgroundPriority { get; set; }
    public static string DirectoryPath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Launcher", "PCBoost");
    private static string FilePath => Path.Combine(DirectoryPath, "preferences.json");
    public static BoostPreferences Load()
    {
        try { return JsonSerializer.Deserialize<BoostPreferences>(File.ReadAllText(FilePath)) ?? new(); }
        catch (Exception error) when (error is IOException or JsonException) { return new(); }
    }
    public void Save()
    {
        Directory.CreateDirectory(DirectoryPath);
        string temporary = FilePath + ".tmp";
        File.WriteAllText(temporary, JsonSerializer.Serialize(this));
        File.Move(temporary, FilePath, overwrite: true);
    }
}
