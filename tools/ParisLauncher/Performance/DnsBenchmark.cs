using System.Buffers.Binary;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;

namespace ParisLauncher.Performance;

public static class DnsBenchmark
{
    private const string ProbeHost = "patch.hispanogaming.com";
    private static readonly TimeSpan Timeout = TimeSpan.FromMilliseconds(1800);

    public static async Task Measure(DnsProvider provider, IPAddress? localAddress = null, CancellationToken cancellation = default)
    {
        var values = new List<double>();
        for (int sample = 0; sample < 3; sample++)
        {
            double? value = await Query(IPAddress.Parse(provider.Addresses[0]), 53, ProbeHost, localAddress, cancellation);
            if (value.HasValue) values.Add(value.Value);
        }
        values.Sort(); provider.Replies = values.Count;
        provider.Milliseconds = values.Count >= 2 ? values[values.Count / 2] : null;
    }

    public static byte[] Question(string host, ushort id)
    {
        var packet = new List<byte>(80);
        var header = new byte[12]; BinaryPrimitives.WriteUInt16BigEndian(header, id);
        header[2] = 1; header[5] = 1; packet.AddRange(header);
        foreach (string part in host.Split('.'))
        {
            if (part.Length is < 1 or > 63 || part.Any(c => !char.IsAsciiLetterOrDigit(c) && c != '-')) throw new ArgumentException("Dominio DNS no válido.");
            packet.Add((byte)part.Length); packet.AddRange(Encoding.ASCII.GetBytes(part));
        }
        packet.AddRange([0, 0, 1, 0, 1]);
        if (packet.Count > 267) throw new ArgumentException("Dominio DNS demasiado largo.");
        return packet.ToArray();
    }

    public static bool ValidResponse(ReadOnlySpan<byte> response, ushort id) =>
        response.Length >= 12 && BinaryPrimitives.ReadUInt16BigEndian(response) == id &&
        (response[2] & 0x80) != 0 && (response[2] & 2) == 0 && (response[3] & 15) == 0 &&
        BinaryPrimitives.ReadUInt16BigEndian(response[4..]) == 1 && BinaryPrimitives.ReadUInt16BigEndian(response[6..]) > 0;

    public static async Task<double?> Query(IPAddress server, int port, string host, IPAddress? localAddress = null, CancellationToken cancellation = default)
    {
        ushort id = (ushort)RandomNumberGenerator.GetInt32(65536);
        byte[] question = Question(host, id);
        using var client = new UdpClient(new IPEndPoint(localAddress ?? IPAddress.Any, 0));
        using var limit = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
        limit.CancelAfter(Timeout);
        try
        {
            client.Connect(server, port);
            var watch = Stopwatch.StartNew();
            await client.SendAsync(question, limit.Token);
            var answer = await client.ReceiveAsync(limit.Token);
            return ValidResponse(answer.Buffer, id) ? watch.Elapsed.TotalMilliseconds : null;
        }
        catch (SocketException) { return null; }
        catch (OperationCanceledException) when (!cancellation.IsCancellationRequested) { return null; }
    }
}
