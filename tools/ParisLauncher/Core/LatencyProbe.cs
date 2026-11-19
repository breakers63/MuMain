using System.Diagnostics;
using System.Net.Sockets;

namespace ParisLauncher.Core;

/// <summary>Result of a short series of TCP handshakes to the game server.</summary>
public sealed record LatencyResult(int Sent, int Received, double? MedianMs, double? MinMs, double? MaxMs, double? JitterMs)
{
    public IReadOnlyList<double> Samples { get; init; } = [];
    public double LossPercent => Sent == 0 ? 100 : 100.0 * (Sent - Received) / Sent;
}

/// <summary>
/// Measures the real round trip from this PC to the server by timing TCP handshakes.
/// No administrator rights and no game packets: one connect/close per sample.
/// </summary>
public static class LatencyProbe
{
    private static readonly TimeSpan ProbeTimeout = TimeSpan.FromSeconds(2);
    private static readonly TimeSpan Pause = TimeSpan.FromMilliseconds(250);

    public static async Task<LatencyResult> MeasureAsync(string host, int port, int samples, CancellationToken token)
    {
        var values = new List<double>();
        for (int i = 0; i < samples; i++)
        {
            token.ThrowIfCancellationRequested();
            double? rtt = await OnceAsync(host, port, token).ConfigureAwait(false);
            if (rtt.HasValue) values.Add(rtt.Value);
            if (i < samples - 1) await Task.Delay(Pause, token).ConfigureAwait(false);
        }

        if (values.Count == 0) return new LatencyResult(samples, 0, null, null, null, null);

        var sorted = values.OrderBy(v => v).ToList();
        int middle = sorted.Count / 2;
        double median = sorted.Count % 2 == 1 ? sorted[middle] : (sorted[middle - 1] + sorted[middle]) / 2;
        double jitter = values.Count > 1 ? values.Zip(values.Skip(1), (a, b) => Math.Abs(b - a)).Average() : 0;
        return new LatencyResult(samples, values.Count, median, sorted[0], sorted[sorted.Count - 1], jitter) { Samples = values.ToArray() };
    }

    private static async Task<double?> OnceAsync(string host, int port, CancellationToken token)
    {
        using var client = new TcpClient();
        using var limit = CancellationTokenSource.CreateLinkedTokenSource(token);
        limit.CancelAfter(ProbeTimeout);
        var watch = Stopwatch.StartNew();
        try
        {
            await client.ConnectAsync(host, port, limit.Token).ConfigureAwait(false);
            return watch.Elapsed.TotalMilliseconds;
        }
        catch (OperationCanceledException) when (!token.IsCancellationRequested)
        {
            return null;
        }
        catch (SocketException)
        {
            return null;
        }
    }
}
