namespace ParisLauncher.Core;

public sealed class UpdateFile
{
    public string Path { get; set; } = "";
    public long Size { get; set; }
    public string Sha256 { get; set; } = "";
    public string Url { get; set; } = "";
    public string Client { get; set; } = "common";
    public List<string> ClassicAliases { get; set; } = [];
}
