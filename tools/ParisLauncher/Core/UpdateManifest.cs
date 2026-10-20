namespace ParisLauncher.Core;

public sealed class UpdateManifest
{
    public string Version { get; set; } = "";
    public List<UpdateFile> Files { get; set; } = [];
}
