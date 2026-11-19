namespace ParisLauncher.Api;
public sealed record AccountProfile(string LoginName,string Email,int VipTier,long VipExpires)
{
    public static AccountProfile From(string name,string email,string? tier,string? expires,float legacy)
    {
        long now=DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        if(long.TryParse(tier,out long level)){long.TryParse(expires,out long end);return new(name,email,level is >=1 and <=3&&end>now?(int)level:0,end);}
        if(legacy>0&&legacy<=1000)return new(name,email,(int)Math.Clamp(legacy,1,3),long.MaxValue);
        long expiry=(long)Math.Min(long.MaxValue,legacy*3600d);return new(name,email,expiry>now?1:0,expiry);
    }
}
