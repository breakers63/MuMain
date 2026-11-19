using System.Diagnostics;
namespace ParisLauncher.Performance;
public sealed class GpuSampler : IDisposable
{
    private readonly List<PerformanceCounter> counters = [];
    private bool initialized;
    public double? Read()
    {
        try
        {
            if (!initialized)
            {
                initialized = true;
                var category = new PerformanceCounterCategory("GPU Engine");
                foreach (string instance in category.GetInstanceNames().Where(name => name.EndsWith("engtype_3D", StringComparison.OrdinalIgnoreCase)))
                    counters.Add(new PerformanceCounter("GPU Engine", "Utilization Percentage", instance, true));
                foreach (var counter in counters) counter.NextValue();
                return null;
            }
            return counters.Count == 0 ? null : Math.Clamp(counters.Sum(counter => (double)counter.NextValue()), 0, 100);
        }
        catch (Exception error) when (error is InvalidOperationException or System.ComponentModel.Win32Exception or UnauthorizedAccessException) { Dispose(); return null; }
    }
    public void Dispose() { foreach (var counter in counters) counter.Dispose(); counters.Clear(); initialized = false; }
}
