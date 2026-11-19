namespace ParisLauncher.Performance;
public static class MemoryTrimPolicy
{
    public static bool Due(DateTimeOffset previous, DateTimeOffset now, int hours) => now >= previous && now - previous >= TimeSpan.FromHours(Math.Clamp(hours,1,24));
}
