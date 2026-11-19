namespace ParisLauncher.Api;

public static class AccountAccess
{
    // OpenMU AccountState: Normal=0, Spectator=1, GameMaster=2,
    // GameMasterInvisible=3, Banned=4, TemporarilyBanned=5.
    public const int FirstBlockedState = 4;
    public static bool CanLogin(int state) => state >= 0 && state < FirstBlockedState;
}
