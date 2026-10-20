using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace ParisLauncher.Core;

public static class ClientProcess
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool WritePrivateProfileString(string section, string key, string value, string path);

    public static bool IsRunning(string root, LauncherSettings settings)
    {
        string[] directories = [Path.GetFullPath(root), Path.GetDirectoryName(settings.Executable(root, true))!, Path.GetDirectoryName(settings.Executable(root, false))!];
        foreach (var process in Process.GetProcesses())
        {
            using (process)
            {
                if (process.Id == Environment.ProcessId) continue;
                try
                {
                    string? path = process.MainModule?.FileName;
                    if (path == null || !directories.Contains(Path.GetDirectoryName(path), StringComparer.OrdinalIgnoreCase)) continue;
                    string name = Path.GetFileNameWithoutExtension(path);
                    if (name.Equals("Main", StringComparison.OrdinalIgnoreCase) || name.StartsWith("Jugar", StringComparison.OrdinalIgnoreCase) ||
                        path.Equals(settings.Executable(root, true), StringComparison.OrdinalIgnoreCase) || path.Equals(settings.Executable(root, false), StringComparison.OrdinalIgnoreCase)) return true;
                }
                catch (Exception e) when (e is System.ComponentModel.Win32Exception or InvalidOperationException or NotSupportedException) { }
            }
        }
        return false;
    }

    public static void Start(string root, LauncherSettings settings, bool classic, string backend)
    {
        string executable = settings.Executable(root, classic);
        if (!File.Exists(executable)) throw new FileNotFoundException(classic ? "Selecciona primero el cliente Clásico en Opciones del launcher." : "No se encontró Jugar.exe junto al launcher.");
        if (string.Equals(executable, Environment.ProcessPath, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("El cliente seleccionado es el propio launcher.");
        if (IsRunning(root, settings)) throw new InvalidOperationException("El cliente ya está abierto. Ciérralo antes de volver a iniciar.");
        string directory = Path.GetDirectoryName(executable)!;
        foreach (string name in new[] { "config.ini", "boost.ini", "Performance.ini" })
        {
            string target = Path.Combine(directory, name);
            string template = target + ".template";
            if (!File.Exists(target) && File.Exists(template)) File.Copy(template, target);
        }
        if (!classic)
        {
            if (backend is not ("vulkan" or "direct3d12")) throw new InvalidDataException("Motor gráfico no válido.");
            string config = Path.Combine(directory, "config.ini");
            if (!WritePrivateProfileString("Render", "Backend", backend, config)) throw new IOException("No se pudo guardar el motor gráfico en config.ini.");
        }
        Process.Start(new ProcessStartInfo(executable) { WorkingDirectory = directory, UseShellExecute = true });
    }
}
