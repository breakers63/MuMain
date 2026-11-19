using System.IO.Compression;
using System.Net;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using ParisLauncher.Core;

internal static class BundleChecks
{
    private static string Hash(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes));
    public static async Task Run()
    {
        string root = Path.Combine(Path.GetTempPath(), "ParisZip-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            byte[] content = Encoding.UTF8.GetBytes(new string('z', 8192));
            var file = new UpdateFile { Path = "Data/zip-test.bin", Size = content.Length, Sha256 = Hash(content) };
            byte[] zip = Archive(file.Path, content);
            var bundle = new UpdateBundle { Url = "releases/test/client.zip", Size = zip.Length, Sha256 = Hash(zip), Files = [file.Path] };
            var manifest = new UpdateManifest { Version = "zip-test", Files = [file], Bundles = [bundle] };
            using var handler = new Downloads(manifest, zip);
            using var http = new HttpClient(handler);
            await new UpdateService(http, root, () => { }).RunAsync("https://patch.test/windows/manifest.json", false, new Progress<UpdateProgress>(), default);
            if (handler.ZipRequests != 2 || !handler.Resumed || !await UpdateService.Matches(Path.Combine(root, file.Path), file, default)) throw new Exception("ZIP resumed installation failed");
            Console.WriteLine("OK ZIP resumes interrupted data and verifies extracted content before committing");
            string archive = Path.Combine(root, "unsafe.zip"), workspace = Path.Combine(root, "staging"); Directory.CreateDirectory(workspace);
            await File.WriteAllBytesAsync(archive, Archive("../escape.bin", content));
            try { await BundleUpdates.Extract(archive, bundle, [file], new HashSet<string> { file.Path }, workspace, root, new Progress<UpdateProgress>(), default); throw new Exception("Unsafe ZIP accepted"); }
            catch (InvalidDataException) { Console.WriteLine("OK ZIP rejects paths outside installation and undeclared entries"); }
            await File.WriteAllBytesAsync(archive, Archive(file.Path, Encoding.UTF8.GetBytes(new string('x', content.Length))));
            try { await BundleUpdates.Extract(archive, bundle, [file], new HashSet<string> { file.Path }, workspace, root, new Progress<UpdateProgress>(), default); throw new Exception("Corrupt ZIP content accepted"); }
            catch (InvalidDataException) { if (!await UpdateService.Matches(Path.Combine(root, file.Path), file, default)) throw new Exception("Corruption changed installed file"); Console.WriteLine("OK ZIP rejects bad per-file hash and preserves installed files"); }
        }
        finally { Directory.Delete(root, true); }
    }
    private static byte[] Archive(string path, byte[] data)
    {
        using var memory = new MemoryStream();
        using (var zip = new ZipArchive(memory, ZipArchiveMode.Create, true))
        using (var output = zip.CreateEntry(path).Open()) output.Write(data);
        return memory.ToArray();
    }
    private sealed class Downloads(UpdateManifest manifest, byte[] zip) : HttpMessageHandler
    {
        public int ZipRequests;
        public bool Resumed;
        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            if (request.RequestUri!.AbsolutePath.EndsWith("manifest.json")) return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK) { Content = new ByteArrayContent(JsonSerializer.SerializeToUtf8Bytes(manifest)) });
            if (!request.RequestUri.AbsolutePath.EndsWith("client.zip")) throw new Exception("Individual download used instead of ZIP");
            ZipRequests++;
            int offset = (int)(request.Headers.Range?.Ranges.First().From ?? 0);
            if (ZipRequests == 1)
            {
                var response = new HttpResponseMessage(HttpStatusCode.OK) { Content = new ByteArrayContent(zip[..(zip.Length / 2)]) };
                response.Content.Headers.ContentLength = zip.Length;
                return Task.FromResult(response);
            }
            Resumed = offset == zip.Length / 2;
            var resumed = new HttpResponseMessage(HttpStatusCode.PartialContent) { Content = new ByteArrayContent(zip[offset..]) };
            resumed.Content.Headers.ContentRange = new ContentRangeHeaderValue(offset, zip.Length - 1, zip.Length);
            return Task.FromResult(resumed);
        }
    }
}
