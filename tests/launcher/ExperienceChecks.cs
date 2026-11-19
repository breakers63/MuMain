using ParisLauncher.Api;
using ParisLauncher.Performance;

internal static class ExperienceChecks
{
    public static void Run(Action<bool, string> check)
    {
        check(AccountAccess.CanLogin(0), "normal game accounts may authenticate");
        check(AccountAccess.CanLogin(1), "spectator game accounts may authenticate");
        check(AccountAccess.CanLogin(2) && AccountAccess.CanLogin(3), "both Game Master account states may authenticate");
        check(!AccountAccess.CanLogin(4) && !AccountAccess.CanLogin(5), "banned and temporarily banned accounts cannot authenticate");
        check(!AccountAccess.CanLogin(-1) && !AccountAccess.CanLogin(99), "unknown account states cannot authenticate");
        check(new AccountRequest("Player1", "abc123", "player@example.com").IsValid(true), "registration accepts game-compatible credentials");
        check(!new AccountRequest("TooLongName1", "abc123", "player@example.com").IsValid(true), "registration rejects names outside the game packet limit");
        check(!new AccountRequest("Player1", "abcé123", "player@example.com").IsValid(true), "registration rejects passwords the game packet cannot encode");
        check(!new AccountRequest("Player1", "abc123", "invalid").IsValid(true), "registration validates email");
        check(new AccountRequest("Player1", "abc123", null).IsValid(false), "existing players can log in without providing email");
        string directory = Path.Combine(Path.GetTempPath(), "ParisProfileChecks-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(directory);
        try { CheckProfiles(directory, check); }
        finally { Directory.Delete(directory, recursive: true); }
    }

    private static void CheckProfiles(string directory, Action<bool, string> check)
    {
        string file = Path.Combine(directory, "config.ini");
        File.WriteAllText(file, "[LOGIN]\nEncryptedUsername=UNCHANGED\n[Render]\nDisableBodyShadows=0\n");
        var profile = new IniProfile(directory);
        profile.Apply(false, "Rendimiento", false, false, false, true, false, false);
        check(File.ReadAllText(file).Contains("EncryptedUsername=UNCHANGED"), "graphics profiles preserve unrelated login settings");
        check(File.ReadAllText(file).Contains("DisableBodyShadows=1"), "performance profile writes a supported render setting");
        check(File.ReadAllText(file).Contains("VSync=0"), "graphics profile saves the chosen vertical synchronization setting");
        check(File.ReadAllText(file).Contains("DisableBoids=1"), "graphics profile saves the chosen environment setting");
        profile.Apply(false, "Calidad", true, true, true, false);
        profile.Restore();
        check(File.ReadAllText(file).Contains("DisableBodyShadows=0"), "restore keeps the original baseline across multiple profiles");
        check(!File.Exists(Path.Combine(directory, "paris-launcher-profile-backup.json")), "successful profile restoration removes the backup marker");
    }
}
