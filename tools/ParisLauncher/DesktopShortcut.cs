using System.IO;
using System.Runtime.InteropServices;

namespace ParisLauncher;

public static class DesktopShortcut
{
    public static void Create(string root)
    {
        string executable = Path.Combine(root, "Launcher.exe");
        if (!File.Exists(executable)) return;
        Type shellType = Type.GetTypeFromProgID("WScript.Shell") ?? throw new InvalidOperationException("No se puede crear el acceso directo.");
        object shell = Activator.CreateInstance(shellType)!;
        object? shortcut = null;
        try
        {
            dynamic automation = shell;
            shortcut = automation.CreateShortcut(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "Paris Project.lnk"));
            dynamic link = shortcut;
            link.TargetPath = executable;
            link.WorkingDirectory = root;
            link.IconLocation = executable + ",0";
            link.Description = "Paris Project · Clásico y Moderno";
            link.Save();
        }
        finally
        {
            if (shortcut != null) Marshal.FinalReleaseComObject(shortcut);
            Marshal.FinalReleaseComObject(shell);
        }
    }
}
