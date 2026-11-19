using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace ParisLauncher.Performance;

public static class WindowsCommand
{
    public static async Task<string> ReadPowerShell(string script)
    {
        using var process = Process.Start(new ProcessStartInfo(PowerShellPath, Encoded("[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false);" + script))
        { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true }) ?? throw new IOException("No se pudo abrir PowerShell.");
        Task<string> output = process.StandardOutput.ReadToEndAsync();
        Task<string> error = process.StandardError.ReadToEndAsync();
        await process.WaitForExitAsync();
        if (process.ExitCode != 0) throw new IOException((await error).Trim());
        return (await output).Trim();
    }

    public static async Task ApplyAsAdministrator(string apply, string rollback)
    {
        string script = "$ErrorActionPreference='Stop';try{" + apply + ";exit 0}catch{try{" + rollback + "}catch{};exit 1}";
        try
        {
            using var process = Process.Start(new ProcessStartInfo(PowerShellPath, Encoded(script))
            { UseShellExecute = true, Verb = "runas", WindowStyle = ProcessWindowStyle.Hidden }) ?? throw new IOException("No se pudo solicitar autorización de Windows.");
            await process.WaitForExitAsync();
            if (process.ExitCode != 0) throw new IOException("Windows no pudo completar el cambio. Se intentó recuperar la configuración anterior; el respaldo se conserva para restaurarlo.");
        }
        catch (Win32Exception error) when (error.NativeErrorCode == 1223)
        { throw new IOException("Se canceló la autorización de administrador. No se aplicó este cambio.", error); }
    }

    public static string Quote(string value) => "'" + value.Replace("'", "''") + "'";
    private static string Encoded(string script) => "-NoProfile -NonInteractive -EncodedCommand " + Convert.ToBase64String(Encoding.Unicode.GetBytes(script));
    private static string PowerShellPath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe");
}
