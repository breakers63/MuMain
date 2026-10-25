namespace ParisLauncher.Core;

/// <summary>Retains one update instead of queuing work for the window per file.</summary>
public sealed class LatestProgress : IProgress<UpdateProgress>
{
    private UpdateProgress? latest;
    public void Report(UpdateProgress value) => Interlocked.Exchange(ref latest, value);
    public UpdateProgress? Take() => Interlocked.Exchange(ref latest, null);
}
