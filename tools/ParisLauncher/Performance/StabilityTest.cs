using System.Diagnostics;
using ParisLauncher.Core;
namespace ParisLauncher.Performance;
public static class StabilityTest
{
    public sealed record Probe(DateTimeOffset Time, string Target, string Protocol, double? Milliseconds);
    public sealed record Report(DateTimeOffset Started, DateTimeOffset Finished, IReadOnlyList<Probe> Probes);
    public static async Task<Report> Run(string server, int port, IProgress<Report> progress, CancellationToken token)
    {
        var started=DateTimeOffset.Now;var watch=Stopwatch.StartNew();var probes=new List<Probe>();
        using var deadline=CancellationTokenSource.CreateLinkedTokenSource(token);
        deadline.CancelAfter(TimeSpan.FromSeconds(30));
        try
        {
            while(watch.Elapsed<TimeSpan.FromSeconds(30))
            {
                var round=watch.Elapsed;
                var samples=await Task.WhenAll(Internet("8.8.8.8",deadline.Token),Internet("1.1.1.1",deadline.Token),Game(server,port,deadline.Token));
                probes.AddRange(samples);progress.Report(new(started,DateTimeOffset.Now,probes.ToArray()));
                var wait=TimeSpan.FromSeconds(1)-(watch.Elapsed-round);
                if(wait>TimeSpan.Zero)await Task.Delay(wait,deadline.Token);
            }
        }
        catch(OperationCanceledException) when(!token.IsCancellationRequested) { }
        token.ThrowIfCancellationRequested();
        return new(started,DateTimeOffset.Now,probes.ToArray());
    }
    static async Task<Probe> Internet(string host,CancellationToken token) => new(DateTimeOffset.Now,host,"ICMP",await NetworkTests.Ping(host,token));
    static async Task<Probe> Game(string host,int port,CancellationToken token)
    {
        var time=DateTimeOffset.Now;
        var sample=await LatencyProbe.MeasureAsync(host,port,1,token);
        return new(time,$"{host}:{port}","TCP",sample.MedianMs);
    }
    public static string Describe(Report report)
    {
        var text=new System.Text.StringBuilder($"Duración: {(report.Finished-report.Started).TotalSeconds:0} / 30 segundos\n");
        foreach(var target in report.Probes.GroupBy(p=>(p.Target,p.Protocol)))
        {
            var values=target.Where(p=>p.Milliseconds.HasValue).Select(p=>p.Milliseconds!.Value).ToArray();
            int missing=target.Count()-values.Length,streak=0,longest=0;
            foreach(var sample in target){streak=sample.Milliseconds.HasValue?0:streak+1;longest=Math.Max(longest,streak);}
            text.AppendLine($"\n{target.Key.Target} · {target.Key.Protocol}\nRespuestas {values.Length}/{target.Count()} · Fallos {missing} ({100d*missing/target.Count():0.0}%)\nMáximo de fallos consecutivos: {longest}");
            if(values.Length>0)text.AppendLine($"Latencia mín./prom./máx.: {values.Min():0.0} / {values.Average():0.0} / {values.Max():0.0} ms\nVariación entre muestras: {(values.Length>1?values.Zip(values.Skip(1),(a,b)=>Math.Abs(a-b)).Average():0):0.0} ms");
        }
        text.AppendLine("\nICMP sin respuesta puede ser filtrado. TCP mide conexión al puerto de MU; no certifica pérdida de paquetes ni una desconexión dentro del juego.");
        return text.ToString();
    }
}
