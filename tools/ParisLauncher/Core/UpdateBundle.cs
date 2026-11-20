namespace ParisLauncher.Core;

public sealed class UpdateBundle
{
    public string Url { get; set; } = "";
    public long Size { get; set; }
    public string Sha256 { get; set; } = "";
    public List<string> Files { get; set; } = [];
    // Verified legacy ZIP entries may be reused under the current client layout.
    public Dictionary<string, string>? EntryMap { get; set; }
}
