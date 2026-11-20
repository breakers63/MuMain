using System.IO;
using System.IO.Compression;

namespace ParisLauncher.Core;

public static class BundleUpdates
{
    private const long MaximumBundleBytes = 32L * 1024 * 1024 * 1024;
    private const int ManySmallFiles = 128;

    public static void Validate(UpdateManifest manifest, string root)
    {
        if (manifest.Bundles == null || manifest.Bundles.Count > 64) throw new InvalidDataException("Lista de ZIP no válida.");
        var known = manifest.Files.ToDictionary(f => f.Path, StringComparer.OrdinalIgnoreCase);
        foreach (var bundle in manifest.Bundles)
        {
            if (bundle.Size <= 0 || bundle.Size > MaximumBundleBytes || bundle.Sha256.Length != 64 || !bundle.Sha256.All(Uri.IsHexDigit) || !bundle.Url.EndsWith(".zip", StringComparison.OrdinalIgnoreCase) || bundle.Files == null || bundle.Files.Count == 0 || bundle.Files.Count > known.Count)
                throw new InvalidDataException("ZIP de actualización no válido.");
            if (bundle.EntryMap != null)
            {
                if (bundle.EntryMap.Count != bundle.Files.Count || !bundle.EntryMap.Values.ToHashSet(StringComparer.OrdinalIgnoreCase).SetEquals(bundle.Files)) throw new InvalidDataException("Invalid ZIP entry map.");
                foreach (var source in bundle.EntryMap.Keys) UpdatePaths.Resolve(root, source);
            }
            var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string file in bundle.Files)
            {
                UpdatePaths.Resolve(root, file);
                if (!known.ContainsKey(file) || !seen.Add(file)) throw new InvalidDataException("Archivo del ZIP no declarado o repetido: " + file);
            }
        }
    }

    public static List<UpdateBundle> Select(IReadOnlyList<UpdateBundle> bundles, List<UpdateFile> changed)
    {
        var remaining = changed.ToDictionary(f => f.Path, StringComparer.OrdinalIgnoreCase);
        if (changed.Count >= ManySmallFiles)
        {
            long total = changed.Sum(f => f.Size);
            var complete = bundles.Where(b => b.Size <= total * 1.05 && Covers(b, remaining.Keys)).OrderBy(b => b.Size).FirstOrDefault();
            if (complete != null) return [complete];
        }
        var selected = new List<UpdateBundle>();
        foreach (var bundle in bundles.OrderBy(b => b.Size))
        {
            var useful = bundle.Files.Where(remaining.ContainsKey).Select(p => remaining[p]).ToList();
            long bytes = useful.Sum(f => f.Size);
            if (useful.Count == 0 || changed.Count < ManySmallFiles && bundle.Size > bytes * 1.05 && (useful.Count < ManySmallFiles || bundle.Size > bytes * 2)) continue;
            selected.Add(bundle);
            foreach (var file in useful) remaining.Remove(file.Path);
        }
        return selected;
    }

    private static bool Covers(UpdateBundle bundle, IEnumerable<string> paths)
    {
        var files = bundle.Files.ToHashSet(StringComparer.OrdinalIgnoreCase);
        return paths.All(files.Contains);
    }

    public static async Task<List<(UpdateFile File, string Staged)>> Extract(string archive, UpdateBundle bundle, IReadOnlyList<UpdateFile> manifest, IReadOnlySet<string> wanted, string workspace, string root, IProgress<UpdateProgress> progress, CancellationToken cancellation)
    {
        var declared = manifest.ToDictionary(f => f.Path, StringComparer.OrdinalIgnoreCase);
        var allowed = bundle.Files.ToHashSet(StringComparer.OrdinalIgnoreCase);
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var staged = new List<(UpdateFile File, string Staged)>();
        using var zip = ZipFile.OpenRead(archive);
        if (zip.Entries.Count > (bundle.EntryMap == null ? allowed.Count : 100000)) throw new InvalidDataException("El ZIP contiene entradas no declaradas.");
        foreach (var entry in zip.Entries)
        {
            cancellation.ThrowIfCancellationRequested();
            string source = entry.FullName.Replace('\\', '/');
            if (source.StartsWith('/') || source.Contains(':') || source.Split('/').Any(p => p is "" or "." or "..")) throw new InvalidDataException("Unsafe ZIP entry path.");
            if (((entry.ExternalAttributes >> 16) & 0xF000) == 0xA000) throw new InvalidDataException("ZIP symbolic link rejected.");
            string path = source;
            if (bundle.EntryMap != null) { if (!bundle.EntryMap.TryGetValue(source, out var mapped)) continue; path = mapped; }
            UpdatePaths.Resolve(root, path);
            if (((entry.ExternalAttributes >> 16) & 0xF000) == 0xA000 || !allowed.Contains(path) || !seen.Add(path) || !declared.TryGetValue(path, out var file) || entry.Length != file.Size)
                throw new InvalidDataException("Entrada del ZIP no válida: " + path);
            if (!wanted.Contains(path)) continue;
            string temp = Path.Combine(workspace, Guid.NewGuid().ToString("N") + ".zip-part");
            progress.Report(new("Extrayendo y verificando " + path, 90));
            await using (var input = entry.Open())
            await using (var output = new FileStream(temp, FileMode.CreateNew, FileAccess.Write, FileShare.None, 131072, true))
                await CopyBounded(input, output, file.Size, cancellation);
            if (!await UpdateService.Matches(temp, file, cancellation)) throw new InvalidDataException("Archivo del ZIP dañado: " + path);
            staged.Add((file, temp));
        }
        if (!allowed.SetEquals(seen)) throw new InvalidDataException("Faltan archivos declarados en el ZIP.");
        return staged;
    }

    private static async Task CopyBounded(Stream input, Stream output, long maximum, CancellationToken cancellation)
    {
        byte[] buffer = new byte[131072];
        long total = 0;
        int count;
        while ((count = await input.ReadAsync(buffer, cancellation)) != 0)
        {
            total += count;
            if (total > maximum) throw new InvalidDataException("El ZIP supera el tamaño declarado.");
            await output.WriteAsync(buffer.AsMemory(0, count), cancellation);
        }
        if (total != maximum) throw new InvalidDataException("Archivo ZIP incompleto.");
    }
}
