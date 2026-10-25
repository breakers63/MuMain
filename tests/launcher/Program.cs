using System.IO;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using ParisLauncher.Core;

if (args.Length == 1 && args[0] == "--verify-r2")
{
    const string ManifestAddress = "https://patch.hispanogaming.com/windows/manifest.json";
    using var fetch = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false });
    var published = JsonSerializer.Deserialize<UpdateManifest>(await fetch.GetStringAsync(ManifestAddress), LauncherSettings.JsonOptions)!;
    var selected = published.Files.Where(f => new[] { "Main.exe", "Classic/Main.exe", "Jugar.exe", "Classic/Jugar.exe", "msvcp140.dll", "Classic/msvcp140.dll", "Data/Interface/RmlUi/themes/paris-gold/char_sel_main.rml" }.Contains(f.Path)).ToList();
    selected.Add(published.Files.First(f => f.Client == "common" && f.Size is > 0 and < 1024 * 1024));
    selected.Add(published.Files.First(f => f.Path.EndsWith(".dxil")));
    selected.Add(published.Files.First(f => f.Path.EndsWith(".spv")));
    string temporary = Path.Combine(Path.GetTempPath(), "ParisR2Smoke-" + Guid.NewGuid().ToString("N"));
    Directory.CreateDirectory(temporary);
    try
    {
        using var transport = new R2Downloads(new UpdateManifest { Version = published.Version, Files = selected });
        using var client = new HttpClient(transport);
        string status = await new UpdateService(client, temporary + Path.DirectorySeparatorChar, () => { }).RunAsync(ManifestAddress, false, new Progress<UpdateProgress>(), default, both: true);
        Console.WriteLine(status + $" · {selected.Count} descargas reales verificadas desde R2, incluidos ambos CRT y shaders.");
    }
    finally { Directory.Delete(temporary, recursive: true); }
    return;
}

if (args.Length == 2 && args[0] == "--create-shortcut")
{
    ParisLauncher.DesktopShortcut.Create(Path.GetFullPath(args[1]));
    Console.WriteLine("Acceso directo Paris Project creado con el icono del launcher.");
    return;
}

if (args.Length == 3 && args[0] == "--install-release")
{
    string releaseRoot = Path.GetFullPath(args[1]);
    string installRoot = Path.GetFullPath(args[2]);
    if (Directory.Exists(installRoot)) throw new IOException("Fresh-install test requires a new directory.");
    Directory.CreateDirectory(installRoot);
    using var transport = new LocalRelease(releaseRoot);
    using var client = new HttpClient(transport);
    var watch = System.Diagnostics.Stopwatch.StartNew();
    var reporter = new Progress<UpdateProgress>(p => { if (p.Percent >= 95) Console.WriteLine(p.Message); });
    string result = await new UpdateService(client, installRoot, () => { }).RunAsync("https://release.test/windows/manifest.json", false, reporter, default, both: true);
    var manifest = JsonSerializer.Deserialize<UpdateManifest>(File.ReadAllText(Path.Combine(releaseRoot, "manifest.json")), LauncherSettings.JsonOptions)!;
    foreach (var file in manifest.Files)
    {
        if (!await UpdateService.Matches(Path.Combine(installRoot, file.Path), file, default)) throw new Exception("Installed hash mismatch: " + file.Path);
        foreach (var alias in file.ClassicAliases)
            if (!await UpdateService.Matches(Path.Combine(installRoot, alias), file, default)) throw new Exception("Installed alias mismatch: " + alias);
    }
    Console.WriteLine($"FRESH_INSTALL_PASS: {manifest.Files.Count} files and all Classic aliases verified in {watch.Elapsed.TotalSeconds:0.0}s. {result}");
    return;
}

if (args.Length == 2 && args[0] == "--verify-release")
{
    string releaseRoot = Path.GetFullPath(args[1]);
    using var transport = new LocalRelease(releaseRoot);
    using var client = new HttpClient(transport);
    string result = await new UpdateService(client, releaseRoot, () => { }).RunAsync("https://release.test/windows/manifest.json", false, new Progress<UpdateProgress>(), default, both: true);
    if (transport.FileDownloads != 0) throw new Exception("The prepared release has missing or different files.");
    Console.WriteLine(result + " · Todos los archivos y alias verificados.");
    return;
}

string root = Path.Combine(Path.GetTempPath(), "ParisLauncherChecks-" + Guid.NewGuid().ToString("N"));
Directory.CreateDirectory(root);
int checks = 0;
void Check(bool value, string label) { if (!value) throw new Exception("FAILED: " + label); checks++; Console.WriteLine("OK " + label); }
void Reject(Action action, string label) { try { action(); } catch (InvalidDataException) { Check(true, label); return; } throw new Exception("Accepted: " + label); }
string Hash(string value) => Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(value)));
UpdateFile FileEntry(string path, string data, string client = "common") => new() { Path = path, Size = Encoding.UTF8.GetByteCount(data), Sha256 = Hash(data), Client = client };
var progress = new Progress<UpdateProgress>();
try
{
    InstallationPromptChecks.Run(root, Check);
    string expectedPath = Path.Combine(root, "Data", "inside.txt");
    foreach (string ending in new[] { "", "\\", "/", "\\\\" })
        Check(UpdatePaths.Resolve(root + ending, "Data/inside.txt") == expectedPath, "Accept installation root ending " + JsonSerializer.Serialize(ending));
    foreach (string path in new[] { "../escape", "C:/escape", "/escape", "Data/../../escape", "Data\\..\\escape", "Data/config.ini", "Launcher.exe", ".launcher-update/secret", "Data/file:stream", "Data/CON.txt", "Data/LPT1" })
        Reject(() => UpdatePaths.Resolve(root, path), "Reject path " + path);
    Reject(() => UpdatePaths.DownloadUri(new("https://patch.test/windows/manifest.json"), "https://other.test/file"), "Reject another download origin");
    Reject(() => UpdatePaths.DownloadUri(new("https://patch.test/windows/manifest.json"), "http://patch.test/file"), "Reject HTTP download");
    var entry = FileEntry("Data/shared.txt", "new-content");
    entry.ClassicAliases.Add("Classic/Data/shared.txt");
    var modern = FileEntry("modern.dll", "modern", "modern");
    var classic = FileEntry("Classic/classic.dll", "classic", "classic");
    var manifest = new UpdateManifest { Version = "test", Files = [entry, modern, classic] };
    var handler = new FakeDownloads(manifest, new() { ["Data/shared.txt"] = "new-content", ["modern.dll"] = "modern", ["Classic/classic.dll"] = "classic" });
    using var http = new HttpClient(handler);
    var service = new UpdateService(http, root + Path.DirectorySeparatorChar, () => { });
    Check(service.ValidateManifest(manifest, false).Count == 2, "Modern installs only common and modern");
    Check(service.ValidateManifest(manifest, true).Count == 2, "Classic installs only common and classic");
    Check(service.ValidateManifest(manifest, false, true).Count == 3, "Both installs all groups");
    await service.RunAsync("https://patch.test/windows/manifest.json", false, progress, default);
    Check(File.ReadAllText(Path.Combine(root, entry.Path)) == "new-content", "Verified modern files installed");
    Check(!File.Exists(Path.Combine(root, classic.Path)), "Classic files not downloaded with Modern");
    Check(!Directory.Exists(Path.Combine(root, "Classic/Data")), "Modern-only does not create classic aliases");
    int before = handler.FileDownloads;
    await service.RunAsync("https://patch.test/windows/manifest.json", false, progress, default);
    Check(handler.FileDownloads == before, "Unchanged files do not download twice");
    await service.RunAsync("https://patch.test/windows/manifest.json", true, progress, default);
    Check(handler.FileDownloads == before + 1, "Adding Classic downloads only missing classic files");
    Check(File.ReadAllText(Path.Combine(root, entry.ClassicAliases[0])) == "new-content", "Classic receives shared resources");
    File.WriteAllText(Path.Combine(root, entry.Path), "linked-change");
    Check(File.ReadAllText(Path.Combine(root, entry.ClassicAliases[0])) == "linked-change", "Shared resources are hard-linked on NTFS");
    await service.RunAsync("https://patch.test/windows/manifest.json", true, progress, default);
    Check(File.ReadAllText(Path.Combine(root, entry.ClassicAliases[0])) == "new-content", "Updating shared content refreshes both clients");
    handler.FailNext = true;
    File.WriteAllText(Path.Combine(root, entry.Path), "retry-original");
    await service.RunAsync("https://patch.test/windows/manifest.json", false, progress, default);
    Check(File.ReadAllText(Path.Combine(root, entry.Path)) == "new-content", "Transient server failure retries before installing");

    handler.Corrupt = true;
    File.WriteAllText(Path.Combine(root, entry.Path), "keep-original");
    bool badHash = false;
    try { await service.RunAsync("https://patch.test/windows/manifest.json", false, progress, default); } catch (InvalidDataException) { badHash = true; }
    Check(badHash && File.ReadAllText(Path.Combine(root, entry.Path)) == "keep-original", "Corrupt download does not replace installed files");
    handler.Corrupt = false;
    using (var cancel = new CancellationTokenSource())
    {
        cancel.Cancel();
        bool cancelled = false;
        try { await service.RunAsync("https://patch.test/windows/manifest.json", false, progress, cancel.Token); } catch (OperationCanceledException) { cancelled = true; }
        Check(cancelled && File.ReadAllText(Path.Combine(root, entry.Path)) == "keep-original", "Cancellation leaves installed content intact");
    }

    var transaction = new UpdateTransaction(root);
    string workspace = transaction.Prepare();
    string stage = Path.Combine(workspace, "one.part");
    File.WriteAllText(stage, "replacement");
    bool rolledBack = false;
    try { transaction.Commit([(entry, stage), (modern, Path.Combine(workspace, "missing.part"))]); } catch (IOException) { rolledBack = true; }
    Check(rolledBack && File.ReadAllText(Path.Combine(root, entry.Path)) == "keep-original", "Commit failure rolls back already replaced files");
    workspace = transaction.Prepare();
    File.WriteAllText(Path.Combine(workspace, "recover.bak"), "recovered");
    File.WriteAllText(Path.Combine(workspace, "journal.json"), JsonSerializer.Serialize(new[] { new UpdateJournalEntry(entry.Path, "recover.bak", true) }));
    transaction.Recover();
    Check(File.ReadAllText(Path.Combine(root, entry.Path)) == "recovered", "Restart recovers an interrupted commit");
    var duplicate = new UpdateManifest { Version = "x", Files = [entry, FileEntry("CLASSIC/Data/shared.txt", "x")] };
    Reject(() => service.ValidateManifest(duplicate, true), "Reject duplicate aliases across manifest entries");
    workspace = transaction.Prepare();
    var atomicFiles = new List<(UpdateFile File, string Staged)>();
    for (int index = 0; index < 4; index++)
    {
        string pending = Path.Combine(workspace, $"atomic-{index}.part");
        File.WriteAllText(pending, "new");
        atomicFiles.Add((FileEntry($"batch/{index}.txt", "new"), pending));
    }
    int applied = 0;
    bool fullPlanPersisted = true;
    transaction.Commit(atomicFiles, (done, total, path) =>
    {
        applied = done;
        var pendingPlan = JsonSerializer.Deserialize<List<UpdateJournalEntry>>(File.ReadAllText(Path.Combine(workspace, "journal.json")))!;
        fullPlanPersisted &= pendingPlan.Count == atomicFiles.Count;
    });
    Check(applied == atomicFiles.Count && fullPlanPersisted, "Commit persists the complete recovery plan once and reports each installed file");
    Check(!File.Exists(Path.Combine(workspace, "journal.json")), "Successful commit removes the recovery plan");

    workspace = transaction.Prepare();
    string rollbackStage = Path.Combine(workspace, "rollback.part");
    File.WriteAllText(rollbackStage, "changed");
    bool callbackFailed = false;
    try { transaction.Commit([(entry, rollbackStage)], (_, _, _) => throw new IOException("Simulated interruption after replacement")); }
    catch (IOException) { callbackFailed = true; }
    Check(callbackFailed && File.ReadAllText(Path.Combine(root, entry.Path)) == "recovered", "Published recovery plan restores an already replaced original after failure");
    var boundedProgress = new LatestProgress();
    const int ProgressBurst = 100000;
    Parallel.For(0, ProgressBurst, index => boundedProgress.Report(new("working", index)));
    boundedProgress.Report(new("latest", 100));
    Check(boundedProgress.Take()?.Message == "latest" && boundedProgress.Take() == null, "Progress bursts retain only the latest update without queuing callbacks");
    var cards = GpuDetector.Detect();
    Check(cards.Count != 0 && cards.All(c => c.Name.Length > 0), "Native DXGI detects physical graphics adapters");
    foreach (var card in cards) Console.WriteLine("GPU: " + card.Name + " · " + card.MemoryLabel);
    Console.WriteLine($"PASS: {checks} checks");
}
finally { Directory.Delete(root, recursive: true); }

sealed class FakeDownloads(UpdateManifest manifest, Dictionary<string, string> files) : HttpMessageHandler
{
    public int FileDownloads;
    public bool Corrupt;
    public bool FailNext;
    protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        string path = Uri.UnescapeDataString(request.RequestUri!.AbsolutePath.Replace("/windows/", ""));
        if (path != "manifest.json" && FailNext) { FailNext = false; return Task.FromResult(new HttpResponseMessage(HttpStatusCode.ServiceUnavailable)); }
        string content;
        if (path == "manifest.json") content = JsonSerializer.Serialize(manifest);
        else { Interlocked.Increment(ref FileDownloads); content = Corrupt ? new string('x', files[path].Length) : files[path]; }
        return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK) { Content = new ByteArrayContent(Encoding.UTF8.GetBytes(content)) });
    }
}

sealed class LocalRelease(string root) : HttpMessageHandler
{
    private readonly UpdateManifest releaseManifest = JsonSerializer.Deserialize<UpdateManifest>(File.ReadAllText(Path.Combine(root, "manifest.json")), LauncherSettings.JsonOptions)!;
    public int FileDownloads;
    protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
    {
        string path = Uri.UnescapeDataString(request.RequestUri!.AbsolutePath.Replace("/windows/", ""));
        if (path != "manifest.json") Interlocked.Increment(ref FileDownloads);
        string localPath = path;
        if (path != "manifest.json")
        {
            localPath = releaseManifest.Files.First(f => Uri.UnescapeDataString(f.Url.Length == 0 ? f.Path : f.Url) == path).Path;
        }
        return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK) { Content = new StreamContent(System.IO.File.OpenRead(Path.Combine(root, localPath))) });
    }
}

sealed class R2Downloads(UpdateManifest sample) : HttpMessageHandler
{
    private readonly HttpMessageInvoker network = new(new HttpClientHandler { AllowAutoRedirect = false });
    protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
    {
        if (request.RequestUri!.AbsolutePath.EndsWith("/manifest.json"))
            return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK) { Content = new ByteArrayContent(JsonSerializer.SerializeToUtf8Bytes(sample)) });
        return network.SendAsync(request, cancellationToken);
    }
    protected override void Dispose(bool disposing) { if (disposing) network.Dispose(); base.Dispose(disposing); }
}
