namespace ParisLauncher.Core;

public sealed record GpuInfo(string Name, ulong DedicatedBytes, ulong SharedBytes)
{
    private const double GiB = 1024.0 * 1024 * 1024;
    public string MemoryLabel => DedicatedBytes >= 512UL * 1024 * 1024
        ? $"{DedicatedBytes / GiB:0.#} GB dedicados"
        : $"Gráficos integrados · hasta {SharedBytes / GiB:0.#} GB compartidos";
}
