using System.Diagnostics;
using System.Net;
using System.Net.Http;
using System.Net.NetworkInformation;
using System.Text;
namespace ParisLauncher.Performance;
public static class NetworkTests
{
    public static async Task<double?> Ping(string host,CancellationToken token)
    {
        using var ping=new Ping();try{var reply=await ping.SendPingAsync(host,TimeSpan.FromMilliseconds(1500),new byte[32],null,token);return reply.Status==IPStatus.Success?reply.RoundtripTime:null;}catch(PingException){return null;}
    }
    public static async Task<string> Trace(string host,IProgress<string> progress,CancellationToken token)
    {
        var addresses=await Dns.GetHostAddressesAsync(host,token);var target=addresses.FirstOrDefault(a=>a.AddressFamily==System.Net.Sockets.AddressFamily.InterNetwork)??throw new InvalidOperationException("El host no tiene dirección IPv4.");
        var result=new StringBuilder("MTR breve (3 sondas por salto, hasta 20 saltos)\nSaltos sin respuesta pueden filtrar ICMP; no prueban pérdida en el destino.\n\n");
        for(int ttl=1;ttl<=20;ttl++)
        {
            token.ThrowIfCancellationRequested();var times=new List<long>();IPAddress? address=null;bool reached=false;
            for(int n=0;n<3;n++){using var ping=new Ping();try{var hopWatch=Stopwatch.StartNew();var reply=await ping.SendPingAsync(target,TimeSpan.FromMilliseconds(900),new byte[32],new PingOptions(ttl,true),token);if(reply.Status is IPStatus.Success or IPStatus.TtlExpired){times.Add((long)hopWatch.Elapsed.TotalMilliseconds);address=reply.Address;}reached|=reply.Status==IPStatus.Success;}catch(PingException){}}
            result.AppendLine($"{ttl,2}   {address?.ToString()??"*",-18}   {times.Count}/3   {(times.Count>0?times.Average().ToString("0.0")+" ms":"Sin respuesta")}");progress.Report(result.ToString());if(reached)break;
        }return result.ToString();
    }
    public static async Task<(double Down,double Up)> Speed(IProgress<string> progress,CancellationToken token,IProgress<double>? downloaded=null)
    {
        const int downloadBytes=10_000_000,uploadBytes=2_000_000;
        using var http=new HttpClient(new HttpClientHandler{AllowAutoRedirect=false}){Timeout=TimeSpan.FromSeconds(40)};
        progress.Report("Midiendo descarga (10 MB)…");var watch=Stopwatch.StartNew();
        using var response=await http.GetAsync("https://speed.cloudflare.com/__down?bytes="+downloadBytes,HttpCompletionOption.ResponseHeadersRead,token);response.EnsureSuccessStatusCode();
        await using var stream=await response.Content.ReadAsStreamAsync(token);byte[] buffer=new byte[65536];long received=0;int count;
        while((count=await stream.ReadAsync(buffer,token))>0){received+=count;if(received>downloadBytes)throw new InvalidOperationException("Respuesta mayor al límite de prueba.");}
        if(received!=downloadBytes)throw new InvalidOperationException("Descarga incompleta.");double down=received*8/watch.Elapsed.TotalSeconds/1_000_000;
        downloaded?.Report(down);progress.Report("Midiendo subida (2 MB)…");watch.Restart();using var body=new ByteArrayContent(new byte[uploadBytes]);using var upload=await http.PostAsync("https://speed.cloudflare.com/__up",body,token);upload.EnsureSuccessStatusCode();double up=uploadBytes*8/watch.Elapsed.TotalSeconds/1_000_000;
        return(down,up);
    }
}
