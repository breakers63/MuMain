using System.IO;
using System.Text.Json;

namespace ParisLauncher.Core;

public sealed class LauncherSettings
{
    public string ModernExecutable { get; set; } = "Vulkan/Jugar.exe";
    public string ClassicExecutable { get; set; } = "Classic/Jugar.exe";
    public string ManifestUrl { get; set; } = "https://patch.hispanogaming.com/windows/manifest.json";
    public bool DefenderPromptShown { get; set; }
    public string ServerHost { get; set; } = "15.235.17.79";
    public int ServerPort { get; set; } = 44405;
    public string WebsiteUrl { get; set; } = "https://muparis.com";
    public string DiscordUrl { get; set; } = "https://discord.gg/m6DGRkRNgr";
    public string WhatsAppUrl { get; set; } = "https://chat.whatsapp.com/DnXYHX9GlHD0jXGSs4G3jI";
    public string FacebookUrl { get; set; } = "https://www.facebook.com/share/14v5NjgMysj/?mibextid=wwXIfr";
    public string VipPurchaseUrl { get; set; } = "https://muparis.com/usercp/pariscoins";
    public string SupportUrl { get; set; } = "https://muparis.com/Soporte/";
    public string AccountsApiUrl { get; set; } = "https://ws.hispanogaming.com/launcher-api/";
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
        var loaded = JsonSerializer.Deserialize<LauncherSettings>(File.ReadAllText(path), JsonOptions)
            ?? throw new InvalidDataException("La configuración del launcher está vacía.");
        if (string.IsNullOrWhiteSpace(loaded.DiscordUrl)) loaded.DiscordUrl = "https://discord.gg/m6DGRkRNgr";
        if (loaded.ManifestUrl is "https://patch.hispanogaming.com/windows/manifest-zip.json" or "https://patch.hispanogaming.com/windows/manifest-separated.json")
            loaded.ManifestUrl = "https://patch.hispanogaming.com/windows/manifest.json";
        return loaded;
    }

    public void Save(string root) => File.WriteAllText(Path.Combine(root, "launcher.json"), JsonSerializer.Serialize(this, JsonOptions));

    public string Executable(string root, bool classic) => Path.GetFullPath(Path.Combine(root, classic ? ClassicExecutable : ModernExecutable));
}
