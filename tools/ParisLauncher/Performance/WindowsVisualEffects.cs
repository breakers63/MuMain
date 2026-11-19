using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
namespace ParisLauncher.Performance;
public static class WindowsVisualEffects
{
    public sealed record Option(string Title, uint Get, uint Set, bool Parameter = false);
    public static readonly Option[] Options = [
        new("Animar controles y elementos de las ventanas", 0x1042, 0x1043),
        new("Atenuar elementos después de hacer clic", 0x1014, 0x1015),
        new("Animar información sobre herramientas", 0x1016, 0x1017),
        new("Atenuar información sobre herramientas", 0x1018, 0x1019),
        new("Animar menús", 0x1002, 0x1003),
        new("Atenuar menús", 0x1012, 0x1013),
        new("Deslizar cuadros combinados", 0x1004, 0x1005),
        new("Desplazamiento suave de listas", 0x1006, 0x1007),
        new("Mostrar contenido al arrastrar ventanas", 0x26, 0x25, true),
        new("Mostrar sombra bajo el puntero", 0x101A, 0x101B),
        new("Mostrar sombras bajo las ventanas", 0x1024, 0x1025),
        new("Suavizar bordes de las fuentes", 0x4A, 0x4B, true),
        new("Efectos de selección de listas", 0x100E, 0x100F),
    ];
    private static string Backup => Path.Combine(BoostPreferences.DirectoryPath, "visual-effects-backup.json");
    [DllImport("user32.dll", EntryPoint = "SystemParametersInfoW", SetLastError = true)]
    private static extern bool ReadNative(uint action, uint parameter, out int value, uint flags);
    [DllImport("user32.dll", EntryPoint = "SystemParametersInfoW", SetLastError = true)]
    private static extern bool WriteNative(uint action, uint parameter, IntPtr value, uint flags);
    public static bool Read(Option option)
    {
        if (!ReadNative(option.Get, 0, out int value, 0)) throw new Win32Exception(Marshal.GetLastWin32Error());
        return value != 0;
    }
    private static void Write(Option option, bool enabled)
    {
        uint parameter = option.Parameter && enabled ? 1u : 0;
        IntPtr value = !option.Parameter && enabled ? new IntPtr(1) : IntPtr.Zero;
        if (!WriteNative(option.Set, parameter, value, 3)) throw new Win32Exception(Marshal.GetLastWin32Error());
    }
    public static void Apply(IReadOnlyDictionary<uint, bool> desired)
    {
        var before = Options.ToDictionary(option => option.Get, Read);
        Directory.CreateDirectory(BoostPreferences.DirectoryPath);
        if (!File.Exists(Backup)) File.WriteAllText(Backup, JsonSerializer.Serialize(before));
        try { foreach (var option in Options) if (desired.TryGetValue(option.Get, out bool value)) Write(option, value); }
        catch { foreach (var option in Options) { try { Write(option, before[option.Get]); } catch (Win32Exception) { } } throw; }
    }
    public static void Restore()
    {
        if (!File.Exists(Backup)) return;
        var original = JsonSerializer.Deserialize<Dictionary<uint, bool>>(File.ReadAllText(Backup)) ?? throw new IOException("Respaldo no válido.");
        Apply(original); File.Delete(Backup);
    }
}
