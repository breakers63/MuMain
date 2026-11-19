using ParisLauncher.Core;

internal static class InstallationPromptChecks
{
    public static void Run(string root, Action<bool, string> check)
    {
        string fresh = Path.Combine(root, "fresh-install");
        Directory.CreateDirectory(fresh);
        int offers = 0;
        InstallationPrompts.OfferDefenderOnce(fresh, new LauncherSettings(), () => offers++);
        var saved = LauncherSettings.Load(fresh);
        check(saved.DefenderPromptShown && offers == 1, "Fresh installation records the optional exclusion question");
        InstallationPrompts.OfferDefenderOnce(fresh, saved, () => offers++);
        check(offers == 1, "Declined or accepted exclusion is not offered again after reloading settings");

        string interrupted = Path.Combine(root, "interrupted-install");
        Directory.CreateDirectory(interrupted);
        try { InstallationPrompts.OfferDefenderOnce(interrupted, new LauncherSettings(), () => throw new IOException("Cancelled")); }
        catch (IOException) { }
        InstallationPrompts.OfferDefenderOnce(interrupted, LauncherSettings.Load(interrupted), () => offers++);
        check(offers == 1, "Cancelled exclusion is not offered again when installation resumes");

        foreach (bool classic in new[] { false, true })
        {
            string installed = Path.Combine(root, classic ? "installed-classic" : "installed-modern");
            var settings = new LauncherSettings();
            string executable = settings.Executable(installed, classic);
            Directory.CreateDirectory(Path.GetDirectoryName(executable)!);
            File.WriteAllText(executable, "installed");
            InstallationPrompts.OfferDefenderOnce(installed, settings, () => offers++);
            check(offers == 1, "Existing installation never gets an exclusion question: classic=" + classic);
        }
    }
}
