namespace ParisLauncher.Account;

public sealed record LauncherSession(string Token, string LoginName, DateTimeOffset ExpiresAt);
