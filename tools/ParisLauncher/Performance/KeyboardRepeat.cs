using System.ComponentModel;
using System.Runtime.InteropServices;

namespace ParisLauncher.Performance;

public static class KeyboardRepeat
{
    [DllImport("user32.dll", EntryPoint = "SystemParametersInfoW", SetLastError = true)] private static extern bool Get(uint action, uint parameter, ref uint value, uint flags);
    [DllImport("user32.dll", EntryPoint = "SystemParametersInfoW", SetLastError = true)] private static extern bool Set(uint action, uint value, IntPtr parameter, uint flags);
    public static (uint Speed, uint Delay) Read()
    {
        uint speed = 0, delay = 0;
        if (!Get(0x000A, 0, ref speed, 0) || !Get(0x0016, 0, ref delay, 0)) throw new Win32Exception(Marshal.GetLastWin32Error());
        return (speed, delay);
    }
    public static void Apply(uint speed, uint delay)
    {
        if (speed > 31 || delay > 3) throw new ArgumentException("Parámetros del teclado no válidos.");
        if (!Set(0x000B, speed, IntPtr.Zero, 3) || !Set(0x0017, delay, IntPtr.Zero, 3)) throw new Win32Exception(Marshal.GetLastWin32Error());
    }
}
