using System.Diagnostics;
using ParisLauncher.Core;

internal static class MultiClientChecks
{
    public static async Task Run()
    {
        string root = Path.Combine(Path.GetTempPath(), "ParisMultiClient-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        string executable = Path.Combine(root, "Main.exe");
        string? previous = Environment.GetEnvironmentVariable("PARIS_MULTICLIENT_PROBE");
        try
        {
            foreach (string file in Directory.GetFiles(AppContext.BaseDirectory))
                File.Copy(file, Path.Combine(root, Path.GetFileName(file)));
            File.Copy(Path.Combine(AppContext.BaseDirectory, "LauncherChecks.exe"), executable, true);
            var settings = new LauncherSettings { ClassicExecutable = "Main.exe" };
            Environment.SetEnvironmentVariable("PARIS_MULTICLIENT_PROBE", "1");
            ClientProcess.Start(root, settings, true, "vulkan");
            await WaitForCount(executable, 1);
            if (!ClientProcess.IsRunning(root, settings)) throw new Exception("First client was not detected");
            ClientProcess.Start(root, settings, true, "vulkan");
            await WaitForCount(executable, 2);
            Console.WriteLine("OK Second Main starts while the first remains running");
        }
        finally
        {
            Environment.SetEnvironmentVariable("PARIS_MULTICLIENT_PROBE", previous);
            foreach (var process in Matching(executable))
            {
                using (process) { process.Kill(); await process.WaitForExitAsync(); }
            }
            Directory.Delete(root, true);
        }
    }

    private static Process[] Matching(string path) => Process.GetProcessesByName("Main").Where(process =>
    {
        try { return string.Equals(process.MainModule?.FileName, path, StringComparison.OrdinalIgnoreCase); }
        catch { process.Dispose(); return false; }
    }).ToArray();

    private static async Task WaitForCount(string path, int expected)
    {
        const int Attempts = 40;
        for (int attempt = 0; attempt < Attempts; attempt++)
        {
            var matches = Matching(path);
            int count = matches.Length;
            foreach (var process in matches) process.Dispose();
            if (count == expected) return;
            await Task.Delay(100);
        }
        throw new Exception("Expected " + expected + " client processes");
    }
}
