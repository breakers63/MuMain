using System.IO;
using System.Buffers;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.Json;
using System.Collections.Concurrent;

namespace ParisLauncher.Core;

public sealed class UpdateService(HttpClient http, string root, Action ensureClientClosed)
{
    private const int MaximumManifestBytes = 8 * 1024 * 1024;
    private const int BufferBytes = 128 * 1024;
    private const int MaximumFiles = 100000;

    public async Task<string> RunAsync(string manifestUrl, bool classic, IProgress<UpdateProgress> progress, CancellationToken cancellation, bool both = false)
    {
        ensureClientClosed();
        var transaction = new UpdateTransaction(root);
        await Task.Run(transaction.Recover, cancellation);
        if (string.IsNullOrWhiteSpace(manifestUrl)) return "Actualizaciones sin configurar · Puedes jugar";
        if (!Uri.TryCreate(manifestUrl, UriKind.Absolute, out var uri) || uri.Scheme != "https" || !string.IsNullOrEmpty(uri.UserInfo))
            throw new InvalidDataException("Configura una dirección HTTPS válida para las actualizaciones.");
        progress.Report(new("Buscando actualizaciones…", 0, true));
        UpdateManifest manifest = await FetchManifest(uri, cancellation);
        var files = ValidateManifest(manifest, classic, both);
        string workspace = await Task.Run(transaction.Prepare, cancellation);
        var changed = await FindChanges(files, progress, cancellation);
        var downloads = new ConcurrentBag<(UpdateFile File, string Staged)>();
        long total = changed.Sum(f => f.Size), completed = 0;
        const long FreeSpaceMargin = 64L * 1024 * 1024;
        var drive = new DriveInfo(Path.GetPathRoot(Path.GetFullPath(root))!);
        long backups = changed.Where(f => System.IO.File.Exists(UpdatePaths.Resolve(root, f.Path))).Sum(f => new FileInfo(UpdatePaths.Resolve(root, f.Path)).Length);
        if (drive.AvailableFreeSpace < total + backups + FreeSpaceMargin)
            throw new IOException("No hay espacio suficiente en la unidad para completar la instalación.");
        await Parallel.ForEachAsync(changed, new ParallelOptions { MaxDegreeOfParallelism = Environment.ProcessorCount <= 4 ? 2 : 4, CancellationToken = cancellation }, async (file, token) =>
        {
            string temp = Path.Combine(workspace, Guid.NewGuid().ToString("N") + ".part");
            long previous = 0;
            Uri download = UpdatePaths.DownloadUri(uri, file.Url.Length == 0 ? file.Path.Replace('\\', '/') : file.Url);
            await Download(file, UpdatePaths.WithVersion(download, file.Sha256), temp,
                bytes => {
                    long sum = Interlocked.Add(ref completed, bytes - previous);
                    previous = bytes;
                    progress.Report(new($"Descargando {file.Path}", total == 0 ? 90 : 20 + 70.0 * sum / total));
                }, token);
            downloads.Add((file, temp));
        });
        var staged = downloads.ToList();
        if (classic || both)
            await Task.Run(() => StageClassicAliases(files, staged, workspace, cancellation), cancellation);
        cancellation.ThrowIfCancellationRequested();
        ensureClientClosed();
        progress.Report(new("Instalando actualización…", 95));
        await Task.Run(() => CommitWithProgress(transaction, staged, progress));
        return changed.Count == 0 ? $"Cliente al día · Versión {manifest.Version}" : $"Actualización completa · Versión {manifest.Version}";
    }

    private static void CommitWithProgress(UpdateTransaction transaction, List<(UpdateFile File, string Staged)> staged, IProgress<UpdateProgress> progress)
    {
        const int ProgressIntervalMilliseconds = 100;
        const double InstallStartPercent = 95;
        const double InstallPercentRange = 4;
        long previousReport = 0;
        transaction.Commit(staged, (completed, total, path) =>
        {
            long now = Environment.TickCount64;
            if (completed != total && now - previousReport < ProgressIntervalMilliseconds) return;
            previousReport = now;
            double percent = InstallStartPercent + InstallPercentRange * completed / total;
            progress.Report(new($"Instalando {completed:N0}/{total:N0}: {path}", percent));
        });
    }

    private async Task<UpdateManifest> FetchManifest(Uri uri, CancellationToken cancellation)
    {
        Uri fresh = UpdatePaths.WithVersion(uri, DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(System.Globalization.CultureInfo.InvariantCulture));
        using var response = await http.GetAsync(fresh, HttpCompletionOption.ResponseHeadersRead, cancellation);
        response.EnsureSuccessStatusCode();
        if (response.Content.Headers.ContentLength > MaximumManifestBytes) throw new InvalidDataException("El manifiesto es demasiado grande.");
        await using var input = await response.Content.ReadAsStreamAsync(cancellation);
        using var output = new MemoryStream();
        byte[] buffer = new byte[BufferBytes];
        int count;
        while ((count = await input.ReadAsync(buffer, cancellation)) > 0)
        {
            if (output.Length + count > MaximumManifestBytes) throw new InvalidDataException("El manifiesto es demasiado grande.");
            output.Write(buffer, 0, count);
        }
        return JsonSerializer.Deserialize<UpdateManifest>(output.ToArray(), LauncherSettings.JsonOptions)
            ?? throw new InvalidDataException("El manifiesto está vacío.");
    }

    public List<UpdateFile> ValidateManifest(UpdateManifest manifest, bool classic, bool both = false)
    {
        if (string.IsNullOrWhiteSpace(manifest.Version) || manifest.Files == null || manifest.Files.Count > MaximumFiles)
            throw new InvalidDataException("El manifiesto no tiene una versión o lista de archivos válida.");
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var file in manifest.Files)
        {
            UpdatePaths.Resolve(root, file.Path);
            if (!names.Add(file.Path.Replace('\\', '/')) || file.Size < 0 || file.Size > 32L * 1024 * 1024 * 1024 ||
                file.Sha256.Length != 64 || !file.Sha256.All(Uri.IsHexDigit) || file.Client is not ("common" or "modern" or "classic"))
                throw new InvalidDataException("Entrada de actualización no válida: " + file.Path);
            if (file.ClassicAliases == null || file.ClassicAliases.Count > 1 || (file.ClassicAliases.Count != 0 && file.Client != "common"))
                throw new InvalidDataException("Recurso compartido no válido: " + file.Path);
            foreach (string alias in file.ClassicAliases)
            {
                UpdatePaths.Resolve(root, alias);
                if (!names.Add(alias.Replace('\\', '/'))) throw new InvalidDataException("Destino repetido: " + alias);
            }
        }
        return manifest.Files.Where(f => both || f.Client == "common" || f.Client == (classic ? "classic" : "modern")).ToList();
    }

    private async Task StageClassicAliases(List<UpdateFile> files, List<(UpdateFile File, string Staged)> staged, string workspace, CancellationToken cancellation)
    {
        var sources = staged.ToDictionary(s => s.File.Path, s => s.Staged, StringComparer.OrdinalIgnoreCase);
        foreach (var file in files)
        {
            foreach (string alias in file.ClassicAliases)
            {
                // Re-link after every common-file update: atomic replacement changes the original link.
                bool changed = sources.TryGetValue(file.Path, out string? source);
                if (!changed && await Matches(UpdatePaths.Resolve(root, alias), file, cancellation)) continue;
                source ??= UpdatePaths.Resolve(root, file.Path);
                string temp = Path.Combine(workspace, Guid.NewGuid().ToString("N") + ".alias");
                SharedFiles.LinkOrCopy(source, temp);
                staged.Add((new UpdateFile { Path = alias, Size = file.Size, Sha256 = file.Sha256 }, temp));
            }
        }
    }

    private async Task<List<UpdateFile>> FindChanges(List<UpdateFile> files, IProgress<UpdateProgress> progress, CancellationToken cancellation)
    {
        var changed = new List<UpdateFile>();
        for (int i = 0; i < files.Count; ++i)
        {
            cancellation.ThrowIfCancellationRequested();
            var file = files[i];
            progress.Report(new($"Verificando {file.Path}", 20.0 * i / Math.Max(1, files.Count)));
            string path = UpdatePaths.Resolve(root, file.Path);
            if (!await Matches(path, file, cancellation)) changed.Add(file);
        }
        return changed;
    }

    public static async Task<bool> Matches(string path, UpdateFile file, CancellationToken cancellation)
    {
        if (!File.Exists(path) || new FileInfo(path).Length != file.Size) return false;
        await using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, BufferBytes, useAsync: true);
        return Convert.ToHexString(await SHA256.HashDataAsync(stream, cancellation)).Equals(file.Sha256, StringComparison.OrdinalIgnoreCase);
    }

    private async Task Download(UpdateFile file, Uri uri, string destination, Action<long> report, CancellationToken cancellation)
    {
        const int MaximumAttempts = 3;
        for (int attempt = 0; ; ++attempt)
        {
            try { await DownloadOnce(file, uri, destination, report, cancellation); return; }
            catch (HttpRequestException error) when (attempt + 1 < MaximumAttempts &&
                (error.StatusCode == null || error.StatusCode is System.Net.HttpStatusCode.RequestTimeout or System.Net.HttpStatusCode.TooManyRequests || (int)error.StatusCode >= 500))
            {
                if (System.IO.File.Exists(destination)) System.IO.File.Delete(destination);
                report(0);
                await Task.Delay(TimeSpan.FromSeconds(attempt + 1), cancellation);
            }
        }
    }

    private async Task DownloadOnce(UpdateFile file, Uri uri, string destination, Action<long> report, CancellationToken cancellation)
    {
        using var response = await http.GetAsync(uri, HttpCompletionOption.ResponseHeadersRead, cancellation);
        response.EnsureSuccessStatusCode();
        if (response.Content.Headers.ContentLength is long length && length != file.Size) throw new InvalidDataException("Tamaño de descarga incorrecto: " + file.Path);
        await using var input = await response.Content.ReadAsStreamAsync(cancellation);
        using var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
        byte[] buffer = ArrayPool<byte>.Shared.Rent(BufferBytes);
        try
        {
            await using var output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None, BufferBytes, useAsync: true);
            await CopyDownload(input, output, file, buffer, hash, report, cancellation);
        }
        finally { ArrayPool<byte>.Shared.Return(buffer); }
        if (!Convert.ToHexString(hash.GetHashAndReset()).Equals(file.Sha256, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("La descarga no pasó la verificación SHA-256: " + file.Path);
        report(file.Size);
    }

    private static async Task CopyDownload(Stream input, Stream output, UpdateFile file, byte[] buffer, IncrementalHash hash, Action<long> report, CancellationToken cancellation)
    {
        const int ReportIntervalMilliseconds = 150;
        long received = 0;
        long lastReport = 0;
        int count;
        while ((count = await input.ReadAsync(buffer.AsMemory(0, BufferBytes), cancellation)) > 0)
        {
            received += count;
            if (received > file.Size) throw new InvalidDataException("La descarga supera el tamaño esperado: " + file.Path);
            hash.AppendData(buffer, 0, count);
            await output.WriteAsync(buffer.AsMemory(0, count), cancellation);
            if (Environment.TickCount64 - lastReport < ReportIntervalMilliseconds) continue;
            report(received);
            lastReport = Environment.TickCount64;
        }
        if (received != file.Size) throw new InvalidDataException("Descarga incompleta: " + file.Path);
    }
}
