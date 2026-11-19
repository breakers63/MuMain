namespace ParisLauncher.Performance;

public sealed record DnsProvider(string Name, string[] Addresses)
{
    public string AddressLabel => string.Join(" / ", Addresses);
    public double? Milliseconds { get; set; }
    public int Replies { get; set; }
    public bool Fastest { get; set; }
    public string Result => Milliseconds is { } value ? $"{value:0.0} ms · {Replies}/3 respuestas" : "Sin respuesta válida";
    public static List<DnsProvider> PublicResolvers() =>
    [new("Cloudflare", ["1.1.1.1", "1.0.0.1"]), new("Google", ["8.8.8.8", "8.8.4.4"]), new("Quad9", ["9.9.9.9", "149.112.112.112"]), new("OpenDNS", ["208.67.222.222", "208.67.220.220"]), new("Control D · sin filtro", ["76.76.2.0", "76.76.10.0"]), new("AdGuard · filtra anuncios", ["94.140.14.14", "94.140.15.15"]), new("Comodo Secure", ["8.26.56.26", "8.20.247.20"])];
}
