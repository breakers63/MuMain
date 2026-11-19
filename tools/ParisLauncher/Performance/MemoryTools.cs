using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace ParisLauncher.Performance;

public static class MemoryTools
{
    [StructLayout(LayoutKind.Sequential)] private struct MemoryStatus
    {
        public uint Length, Load;
        public ulong TotalPhysical, AvailablePhysical, TotalPageFile, AvailablePageFile, TotalVirtual, AvailableVirtual, AvailableExtended;
    }
    [DllImport("kernel32.dll", SetLastError = true)] private static extern bool GlobalMemoryStatusEx(ref MemoryStatus status);
    [DllImport("kernel32.dll", SetLastError = true)] private static extern SafeProcessHandle OpenProcess(uint access, bool inherit, int id);
    [DllImport("kernel32.dll", SetLastError = true)] private static extern bool K32EmptyWorkingSet(SafeProcessHandle process);
    private const uint TrimProcessAccess = 0x0100 | 0x1000;
    private static readonly HashSet<string> ProtectedNames = new(StringComparer.OrdinalIgnoreCase)
    { "Main", "Jugar", "Launcher", "System", "Idle", "csrss", "winlogon", "services", "lsass", "svchost", "dwm", "MsMpEng", "NisSrv", "SecurityHealthService", "smss", "fontdrvhost", "sihost" };

    public static (ulong Total, ulong Available, uint Load) Snapshot()
    {
        var status = new MemoryStatus { Length = (uint)Marshal.SizeOf<MemoryStatus>() };
        if (!GlobalMemoryStatusEx(ref status)) throw new Win32Exception(Marshal.GetLastWin32Error());
        return (status.TotalPhysical, status.AvailablePhysical, status.Load);
    }

    public static IReadOnlyList<MemoryApplication> Applications()
    {
        using var self = Process.GetCurrentProcess();
        var rows = new List<(string Path, long Bytes)>();
        foreach (var process in Process.GetProcesses())
        {
            using (process)
            {
                try
                {
                    if (process.SessionId != self.SessionId || ProtectedNames.Contains(process.ProcessName)) continue;
                    string? path = process.MainModule?.FileName;
                    if (path is not null && !path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.Windows), StringComparison.OrdinalIgnoreCase)) rows.Add((path, process.WorkingSet64));
                }
                catch (Exception error) when (error is Win32Exception or InvalidOperationException or NotSupportedException) { }
            }
        }
        return rows.GroupBy(row => row.Path, StringComparer.OrdinalIgnoreCase)
            .Select(group => new MemoryApplication { Path = group.Key, Bytes = group.Sum(row => row.Bytes) })
            .OrderByDescending(row => row.Bytes).Take(25).ToList();
    }

    public static int TrimSelected(IReadOnlySet<string> selected, IReadOnlySet<string> excluded)
    {
        using var self = Process.GetCurrentProcess();
        int count = 0;
        foreach (var process in Process.GetProcesses())
        {
            using (process)
            {
                try
                {
                    if (process.SessionId != self.SessionId || ProtectedNames.Contains(process.ProcessName)) continue;
                    string? path = process.MainModule?.FileName;
                    if (path is null || !selected.Contains(path) || excluded.Contains(path) || path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.Windows), StringComparison.OrdinalIgnoreCase)) continue;
                    using var handle = OpenProcess(TrimProcessAccess, false, process.Id);
                    if (!handle.IsInvalid && K32EmptyWorkingSet(handle)) count++;
                }
                catch (Exception error) when (error is Win32Exception or InvalidOperationException or NotSupportedException) { }
            }
        }
        return count;
    }
}
