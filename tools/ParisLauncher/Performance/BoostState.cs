namespace ParisLauncher.Performance;

public sealed class BoostState
{
    public List<string> Active { get; set; } = [];
    public List<RegistryChange> Originals { get; set; } = [];
    public Dictionary<string, bool> ServiceRunning { get; set; } = [];
    public uint? KeyboardSpeed { get; set; }
    public uint? KeyboardDelay { get; set; }
    public int? LastAccess { get; set; }
}
