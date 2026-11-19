using System.Net;
using System.Net.Http;
using System.IO;
namespace ParisLauncher.Performance;
internal sealed class MeasuredUpload(int size, Action<long> report) : HttpContent
{
    protected override bool TryComputeLength(out long length) { length = size; return true; }
    protected override Task SerializeToStreamAsync(Stream stream, TransportContext? context) => Write(stream, CancellationToken.None);
    protected override Task SerializeToStreamAsync(Stream stream, TransportContext? context, CancellationToken token) => Write(stream, token);
    private async Task Write(Stream stream, CancellationToken token)
    {
        byte[] buffer = new byte[65536]; long sent = 0; var watch = System.Diagnostics.Stopwatch.StartNew(); double last = 0;
        while (sent < size)
        {
            int count = (int)Math.Min(buffer.Length, size - sent);
            await stream.WriteAsync(buffer.AsMemory(0, count), token); sent += count; if(watch.Elapsed.TotalSeconds-last >= 0.1 || sent == size) { last=watch.Elapsed.TotalSeconds; report(sent); }
        }
    }
}
