namespace ParisLauncher.Core;

public sealed class UpdateBundle
{
    public string Url { get; set; } = "";
    public long Size { get; set; }
    public string Sha256 { get; set; } = "";
    public List<string> Files { get; set; } = [];
}
