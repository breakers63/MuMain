using System.IO;

namespace ParisLauncher.Core;

public static class InstallationPrompts
{
    public static void OfferDefenderOnce(string root, LauncherSettings settings, Action offer)
    {
        if (settings.DefenderPromptShown || File.Exists(settings.Executable(root, false)) ||
            File.Exists(settings.Executable(root, true))) return;
        settings.DefenderPromptShown = true;
        settings.Save(root);
        offer();
    }
}
