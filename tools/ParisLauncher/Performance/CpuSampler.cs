using System.Runtime.InteropServices;
namespace ParisLauncher.Performance;
public sealed class CpuSampler
{
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetSystemTimes(out ulong idle, out ulong kernel, out ulong user);
    private ulong previousIdle, previousTotal;
    public double? Read()
    {
        if (!GetSystemTimes(out ulong idle, out ulong kernel, out ulong user)) return null;
        ulong total = kernel + user;
        ulong elapsed = total - previousTotal;
        double? value = previousTotal == 0 || elapsed == 0 ? null : Math.Clamp(100.0 * (1 - (double)(idle - previousIdle) / elapsed), 0, 100);
        previousIdle = idle; previousTotal = total;
        return value;
    }
}
