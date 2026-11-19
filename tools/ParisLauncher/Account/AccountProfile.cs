namespace ParisLauncher.Account;
public sealed record AccountProfile(string LoginName,string Email,int VipTier,long VipExpires)
{
    public string Status => VipTier is >=1 and <=3 ? "VIP "+VipTier : "Cuenta normal";
    public string Expiry => VipTier==0?"Sin VIP activo":VipExpires==long.MaxValue?"VIP permanente":DateTimeOffset.FromUnixTimeSeconds(VipExpires).ToLocalTime().ToString("dd/MM/yyyy HH:mm");
}
