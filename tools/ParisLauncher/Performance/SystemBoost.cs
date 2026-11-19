using System.IO;
using System.Text.Json;
using System.Text.RegularExpressions;
using Microsoft.Win32;

namespace ParisLauncher.Performance;

public static class SystemBoost
{
    public const string GpuRegistryPath = @"Software\Microsoft\DirectX\UserGpuPreferences";
    private static string StateFile => Path.Combine(BoostPreferences.DirectoryPath, "system-backup.json");
    public static BoostState Load()
    {
        if (!File.Exists(StateFile)) return new();
        var state = JsonSerializer.Deserialize<BoostState>(File.ReadAllText(StateFile)) ?? throw new IOException("Respaldo del sistema no válido.");
        foreach (var entry in state.Originals) entry.Validate();
        if (state.LastAccess is < 0 or > 3 || state.KeyboardSpeed is > 31 || state.KeyboardDelay is > 3 || state.ServiceRunning.Keys.Any(key => key is not ("SysMain" or "WSearch"))) throw new IOException("Respaldo del sistema no válido.");
        return state;
    }
    private static void Save(BoostState state)
    {
        Directory.CreateDirectory(BoostPreferences.DirectoryPath);
        string temporary = StateFile + ".tmp";
        File.WriteAllText(temporary, JsonSerializer.Serialize(state));
        File.Move(temporary, StateFile, overwrite: true);
    }

    public static List<RegistryChange> RegistryTargets(IEnumerable<string> executables)
    {
        var rows = new List<RegistryChange>
        {
            new("animations", false, @"Control Panel\Desktop\WindowMetrics", "MinAnimate", RegistryValueKind.String, "0"),
            new("transparency", false, @"Software\Microsoft\Windows\CurrentVersion\Themes\Personalize", "EnableTransparency", RegistryValueKind.DWord, "0"),
            new("gamemode", false, @"Software\Microsoft\GameBar", "AutoGameModeEnabled", RegistryValueKind.DWord, "1"),
            new("capture", false, @"System\GameConfigStore", "GameDVR_Enabled", RegistryValueKind.DWord, "0"),
            new("capture", false, @"Software\Microsoft\Windows\CurrentVersion\GameDVR", "AppCaptureEnabled", RegistryValueKind.DWord, "0"),
            new("sysmain", true, @"SYSTEM\CurrentControlSet\Services\SysMain", "Start", RegistryValueKind.DWord, "4"),
            new("search", true, @"SYSTEM\CurrentControlSet\Services\WSearch", "Start", RegistryValueKind.DWord, "4"),
            new("counters", true, @"SYSTEM\CurrentControlSet\Services\PerfOS\Performance", "Disable Performance Counters", RegistryValueKind.DWord, "1"),
            new("counters", true, @"SYSTEM\CurrentControlSet\Services\PerfProc\Performance", "Disable Performance Counters", RegistryValueKind.DWord, "1"),
        };
        rows.AddRange(executables.Distinct(StringComparer.OrdinalIgnoreCase).Select(path =>
            new RegistryChange("gpu", false, GpuRegistryPath, Path.GetFullPath(path), RegistryValueKind.String, ReadGpuPreference(Path.GetFullPath(path)))));
        return rows;
    }
    private static string ReadGpuPreference(string path) { using var key = Registry.CurrentUser.OpenSubKey(GpuRegistryPath); return MergeGpuPreference(key?.GetValue(path) as string); }
    public static string MergeGpuPreference(string? existing) => string.Join(";", (existing ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries).Where(value => !value.StartsWith("GpuPreference=", StringComparison.OrdinalIgnoreCase)).Append("GpuPreference=2")) + ";";

    public static async Task Apply(IReadOnlySet<string> selected, IEnumerable<string> executables)
    {
        var known = BoostOption.Create().Select(option => option.Id).ToHashSet();
        if (selected.Any(option => !known.Contains(option))) throw new ArgumentException("Opción desconocida.");
        var state = Load();
        var previous = state.Active.ToHashSet();
        var enabling = selected.Except(previous).ToHashSet();
        var disabling = previous.Union(state.Originals.Select(row => row.Option)).Except(selected).ToHashSet();
        if (state.KeyboardSpeed.HasValue && !selected.Contains("keyboard")) disabling.Add("keyboard");
        if (state.LastAccess.HasValue && !selected.Contains("lastaccess")) disabling.Add("lastaccess");
        var operations = RegistryTargets(executables).Where(row => enabling.Contains(row.Option)).ToList();
        foreach (var target in operations)
            if (!state.Originals.Any(row => row.Option == target.Option && row.Path == target.Path && row.Name == target.Name)) state.Originals.Add(target.Current());
        operations.AddRange(state.Originals.Where(row => disabling.Contains(row.Option)));
        var before = operations.Select(row => row.Current()).ToList();
        var machine = operations.Where(row => row.Machine).Select(row => row.Script()).ToList();
        var rollback = before.Where(row => row.Machine).Select(row => row.Script()).ToList();
        await PrepareServices(state, enabling, disabling, machine, rollback);
        if (enabling.Contains("keyboard") && !state.KeyboardSpeed.HasValue) { var original = KeyboardRepeat.Read(); state.KeyboardSpeed = original.Speed; state.KeyboardDelay = original.Delay; }
        if (enabling.Contains("lastaccess"))
        {
            state.LastAccess ??= await ReadLastAccess();
            machine.Add(LastAccessScript(1)); rollback.Add(LastAccessScript(state.LastAccess.Value));
        }
        if (disabling.Contains("lastaccess") && state.LastAccess.HasValue) { int current = await ReadLastAccess(); machine.Add(LastAccessScript(state.LastAccess.Value)); rollback.Add(LastAccessScript(current)); }
        var power = await BoostPowerPlan.Prepare(selected.Contains("power"), selected.Contains("cores"));
        if (power.Apply.Length > 0) { machine.Add(power.Apply); rollback.Add(power.Rollback); }
        Save(state);
        if (machine.Count > 0) await WindowsCommand.ApplyAsAdministrator(string.Join(";", machine), string.Join(";", rollback));
        foreach (var change in operations.Where(row => !row.Machine)) change.WriteUser();
        if (enabling.Contains("keyboard")) KeyboardRepeat.Apply(31, 0);
        if (disabling.Contains("keyboard") && state.KeyboardSpeed.HasValue) KeyboardRepeat.Apply(state.KeyboardSpeed.Value, state.KeyboardDelay!.Value);
        BoostPowerPlan.Completed(power);
        state.Active = selected.ToList();
        state.Originals.RemoveAll(row => disabling.Contains(row.Option));
        if (disabling.Contains("keyboard")) { state.KeyboardSpeed = null; state.KeyboardDelay = null; }
        if (disabling.Contains("lastaccess")) state.LastAccess = null;
        if (disabling.Contains("sysmain")) state.ServiceRunning.Remove("SysMain");
        if (disabling.Contains("search")) state.ServiceRunning.Remove("WSearch");
        Save(state);
    }

    private static async Task PrepareServices(BoostState state, IReadOnlySet<string> enabling, IReadOnlySet<string> disabling, List<string> commands, List<string> rollback)
    {
        foreach (var (option, service) in new[] { ("sysmain", "SysMain"), ("search", "WSearch") })
        {
            if (!enabling.Contains(option) && !disabling.Contains(option)) continue;
            bool running = (await WindowsCommand.ReadPowerShell($"(Get-Service -Name '{service}' -ErrorAction Stop).Status.ToString()")) == "Running";
            if (enabling.Contains(option)) { state.ServiceRunning.TryAdd(service, running); commands.Add($"Stop-Service -Name '{service}' -ErrorAction Stop"); }
            else if (state.ServiceRunning.TryGetValue(service, out bool original)) commands.Add(ServiceState(service, original));
            rollback.Add(ServiceState(service, running));
        }
    }
    private static string ServiceState(string name, bool running) => $"{(running ? "Start" : "Stop")}-Service -Name '{name}' -ErrorAction Stop";
    private static async Task<int> ReadLastAccess()
    {
        string output = await WindowsCommand.ReadPowerShell("& ($env:SystemRoot+'\\System32\\fsutil.exe') behavior query disablelastaccess");
        var match = Regex.Match(output, @"=\s*([0-3])\b");
        if (!match.Success) throw new IOException("Windows no permite leer la política de último acceso.");
        return int.Parse(match.Groups[1].Value);
    }
    private static string LastAccessScript(int value)
    {
        if (value is < 0 or > 3) throw new ArgumentException("Valor NTFS inválido.");
        return $"& ($env:SystemRoot+'\\System32\\fsutil.exe') behavior set disablelastaccess {value};if($LASTEXITCODE -ne 0){{throw 'NTFS'}}";
    }
}
