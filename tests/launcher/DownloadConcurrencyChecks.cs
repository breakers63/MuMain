using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using ParisLauncher.Core;

internal static class DownloadConcurrencyChecks
{
    public static async Task Run()
    {
        const int FileCount = 36;
        string root = Path.Combine(Path.GetTempPath(), "ParisParallel-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        using var handler = new DelayedDownloads(FileCount);
        using var http = new HttpClient(handler);
        try
        {
            await new UpdateService(http, root, () => { }).RunAsync("https://patch.test/windows/manifest.json", false, new Progress<UpdateProgress>(), default);
            if (handler.Peak <= 4 || handler.Peak > UpdateService.MaximumConcurrentDownloads) throw new Exception("Download concurrency outside its bounded budget");
            foreach (var entry in handler.Manifest.Files)
                if (!await UpdateService.Matches(Path.Combine(root, entry.Path), entry, default)) throw new Exception("Parallel download content mismatch");
            Console.WriteLine("OK Parallel downloads exceed four, remain bounded and verify all content (peak " + handler.Peak + ")");
        }
        finally { Directory.Delete(root, true); }
    }

    private sealed class DelayedDownloads : HttpMessageHandler
    {
        private int active;
        public int Peak;
        private static readonly byte[] Content = Encoding.UTF8.GetBytes("verified parallel content");
        public UpdateManifest Manifest { get; }
        public DelayedDownloads(int count)
        {
            Manifest = new UpdateManifest { Version = "parallel", Files = Enumerable.Range(0, count).Select(i => new UpdateFile { Path = "Data/test-" + i + ".bin", Size = Content.Length, Sha256 = Convert.ToHexString(SHA256.HashData(Content)), Client = "common" }).ToList() };
        }
        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            if (request.RequestUri!.AbsolutePath.EndsWith("manifest.json")) return new(HttpStatusCode.OK) { Content = new ByteArrayContent(JsonSerializer.SerializeToUtf8Bytes(Manifest)) };
            int count = Interlocked.Increment(ref active);
            lock (this) { Peak = Math.Max(Peak, count); }
            try { await Task.Delay(30, cancellationToken); return new(HttpStatusCode.OK) { Content = new ByteArrayContent(Content) }; }
            finally { Interlocked.Decrement(ref active); }
        }
    }
}
