using System.IO;

namespace ParisLauncher.Performance;

public sealed class MemoryApplication
{
    public string Path { get; init; } = "";
    public long Bytes { get; init; }
    public bool Selected { get; set; }
    public string Name => System.IO.Path.GetFileNameWithoutExtension(Path);
    public string Usage => $"{Bytes / 1048576.0:N0} MB de working set";
}
