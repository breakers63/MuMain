using System.Diagnostics;
using System.Net;
using System.Net.Http;
using System.Text.Json;
namespace ParisLauncher.Performance;
public static class LiveSpeedTest
{
    public sealed record Update(string Phase, double Mbps = 0, string? Connection = null);
    public static async Task<(double Down, double Up)> Run(IProgress<Update> progress, CancellationToken token)
    {
        using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(45) };
        progress.Report(new("Detectando proveedor…"));
        string identity = await Identity(http, token);
        progress.Report(new("Preparando descarga…", Connection: identity));
        double down = await Download(http, progress, token);
        double up = await Upload(http, progress, token);
        progress.Report(new("Completado", Connection: identity));
        return (down, up);
    }
    private static async Task<string> Identity(HttpClient http, CancellationToken token)
    {
        string provider = "Proveedor no disponible", ip = "IP no disponible", node = "Automático";
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(token);
        timeout.CancelAfter(TimeSpan.FromSeconds(8));
        try
        {
            using var json = JsonDocument.Parse(await http.GetStringAsync("https://ipwho.is/?fields=success,ip,connection", timeout.Token));
            var root = json.RootElement;
            if (root.GetProperty("success").GetBoolean())
            {
                ip = root.GetProperty("ip").GetString() ?? ip;
                provider = root.GetProperty("connection").GetProperty("isp").GetString() ?? provider;
            }
        }
        catch (Exception e) when (e is HttpRequestException or JsonException or OperationCanceledException or KeyNotFoundException) { token.ThrowIfCancellationRequested(); }
        try
        {
            string trace = await http.GetStringAsync("https://speed.cloudflare.com/cdn-cgi/trace", timeout.Token);
            node = trace.Split('\n').FirstOrDefault(s => s.StartsWith("colo="))?[5..].Trim() ?? node;
        }
        catch (Exception e) when (e is HttpRequestException or OperationCanceledException) { token.ThrowIfCancellationRequested(); }
        return $"{provider}\n{ip} · Servidor Cloudflare {node} · selección automática";
    }
    private static async Task<double> Download(HttpClient http, IProgress<Update> progress, CancellationToken token)
    {
        const int chunk = 10_000_000, maximum = 190_000_000;
        var watch = Stopwatch.StartNew(); long total = 0; double last = 0;
        byte[] buffer = new byte[65536];
        do
        {
            using var response = await http.GetAsync($"https://speed.cloudflare.com/__down?bytes={chunk}", HttpCompletionOption.ResponseHeadersRead, token);
            response.EnsureSuccessStatusCode();
            await using var stream = await response.Content.ReadAsStreamAsync(token);
            int count; long received = 0;
            while ((count = await stream.ReadAsync(buffer, token)) > 0)
            {
                received += count; total += count;
                if (received > chunk) throw new InvalidOperationException("La descarga supera el límite solicitado.");
                if (watch.Elapsed.TotalSeconds - last < 0.1) continue;
                last = watch.Elapsed.TotalSeconds;
                progress.Report(new("Descarga", total * 8 / watch.Elapsed.TotalSeconds / 1_000_000));
            }
            if (received != chunk) throw new InvalidOperationException("Descarga incompleta.");
        } while (watch.Elapsed.TotalSeconds < 6 && total < maximum);
        double result = total * 8 / watch.Elapsed.TotalSeconds / 1_000_000;
        progress.Report(new("Descarga", result)); return result;
    }
    private static async Task<double> Upload(HttpClient http, IProgress<Update> progress, CancellationToken token)
    {
        const int chunk = 2_000_000, maximum = 48_000_000;
        var watch = Stopwatch.StartNew(); long total = 0;
        do
        {
            long previous = total;
            using var body = new MeasuredUpload(chunk, bytes => progress.Report(new("Subida", (previous + bytes) * 8 / watch.Elapsed.TotalSeconds / 1_000_000)));
            using var response = await http.PostAsync("https://speed.cloudflare.com/__up", body, token);
            response.EnsureSuccessStatusCode(); total += chunk;
        } while (watch.Elapsed.TotalSeconds < 6 && total < maximum);
        double result = total * 8 / watch.Elapsed.TotalSeconds / 1_000_000;
        progress.Report(new("Subida", result)); return result;
    }
}

