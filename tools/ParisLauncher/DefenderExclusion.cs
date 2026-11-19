using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows;
using ParisLauncher.Core;

namespace ParisLauncher;

public static class DefenderExclusion
{
    public static void Offer()
    {
        var choice = MessageBox.Show(
            @"¿Quieres excluir C:\PARIS PROJECT de Microsoft Defender?" + "\n\n" +
            "Es opcional. Defender dejará de analizar TODOS los archivos de esta carpeta, " +
            "incluidos los que se añadan después. No guardes otros programas aquí. " +
            "Windows pedirá permiso de administrador.\n\n" +
            "Elige No para instalar con la protección habitual.",
            "Paris Project · Exclusión opcional", MessageBoxButton.YesNo,
            MessageBoxImage.Warning, MessageBoxResult.No);
        if (choice != MessageBoxResult.Yes) return;
        Apply();
    }

    private static void Apply()
    {
        try
        {
            UpdatePaths.RejectLinks(InstallerLocation.DefaultDirectory, InstallerLocation.DefaultDirectory);
            string command = "$ErrorActionPreference = 'Stop'; try { Add-MpPreference -ExclusionPath 'C:\\PARIS PROJECT' -ErrorAction Stop; exit 0 } catch { exit 1 }";
            string encoded = Convert.ToBase64String(Encoding.Unicode.GetBytes(command));
            string powershell = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
                "WindowsPowerShell", "v1.0", "powershell.exe");
            using var process = Process.Start(new ProcessStartInfo(powershell,
                "-NoProfile -NonInteractive -EncodedCommand " + encoded)
            {
                UseShellExecute = true,
                Verb = "runas"
            });
            if (process == null) throw new IOException("No se pudo abrir la configuración de Defender.");
            process.WaitForExit();
            MessageBox.Show(process.ExitCode == 0
                ? "Se añadió la exclusión. Puedes quitarla en Seguridad de Windows → Protección contra virus → Exclusiones."
                : "Defender no permitió añadir la exclusión. La instalación continuará con la protección habitual.",
                "Paris Project", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception error) when (error is Win32Exception or IOException or UnauthorizedAccessException)
        {
            MessageBox.Show("No se añadió la exclusión. La instalación continuará.\n" + error.Message,
                "Paris Project", MessageBoxButton.OK, MessageBoxImage.Information);
        }
    }
}
