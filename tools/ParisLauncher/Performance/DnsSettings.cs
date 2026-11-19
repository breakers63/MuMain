using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text.Json;
using Microsoft.Win32;

namespace ParisLauncher.Performance;

public static class DnsSettings
{
    private sealed record Backup(Guid Adapter, string[] StaticServers);
    private static string BackupPath(Guid id) => Path.Combine(BoostPreferences.DirectoryPath, "dns-" + id.ToString("N") + ".json");
    public static string Script(int index, IReadOnlyList<string> servers)
    {
        if (index < 1 || servers.Count > 8 || servers.Any(value => !IPAddress.TryParse(value, out var ip) || ip.AddressFamily != AddressFamily.InterNetwork)) throw new ArgumentException("Configuración DNS IPv4 no válida.");
        string netsh = "& ($env:SystemRoot+'\\System32\\netsh.exe') interface ipv4 ";
        if (servers.Count == 0) return netsh + $"set dnsservers name={index} source=dhcp; if($LASTEXITCODE -ne 0){{throw 'DNS'}}; Clear-DnsClientCache";
        var commands = new List<string> { netsh + $"set dnsservers name={index} source=static address={servers[0]} validate=no; if($LASTEXITCODE -ne 0){{throw 'DNS'}}" };
        for (int i = 1; i < servers.Count; i++) commands.Add(netsh + $"add dnsservers name={index} address={servers[i]} index={i + 1} validate=no; if($LASTEXITCODE -ne 0){{throw 'DNS'}}");
        commands.Add("Clear-DnsClientCache");
        return string.Join(";", commands);
    }

    public static async Task Apply(DnsAdapter adapter, DnsProvider provider)
    {
        var current = DnsAdapter.Active().SingleOrDefault(value => value.Id == adapter.Id) ?? throw new IOException("El adaptador ya no está activo.");
        Directory.CreateDirectory(BoostPreferences.DirectoryPath);
        string file = BackupPath(adapter.Id);
        if (!File.Exists(file))
        {
            using var key = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\" + adapter.Id.ToString("B"));
            string original = key?.GetValue("NameServer") as string ?? "";
            string[] addresses = original.Split([',', ';', ' '], StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
            Script(current.Index, addresses);
            File.WriteAllText(file, JsonSerializer.Serialize(new Backup(adapter.Id, addresses)));
        }
        var backup = JsonSerializer.Deserialize<Backup>(File.ReadAllText(file)) ?? throw new IOException("Respaldo DNS inválido.");
        if (backup.Adapter != adapter.Id) throw new IOException("El respaldo pertenece a otro adaptador.");
        await WindowsCommand.ApplyAsAdministrator(Script(current.Index, provider.Addresses), Script(current.Index, backup.StaticServers));
    }

    public static async Task Restore(DnsAdapter adapter)
    {
        string file = BackupPath(adapter.Id);
        if (!File.Exists(file)) throw new IOException("Este adaptador no tiene cambios DNS guardados por PARIS PROJECT.");
        var backup = JsonSerializer.Deserialize<Backup>(File.ReadAllText(file)) ?? throw new IOException("Respaldo DNS inválido.");
        if (backup.Adapter != adapter.Id) throw new IOException("Respaldo DNS no válido.");
        var current = DnsAdapter.Active().SingleOrDefault(value => value.Id == adapter.Id) ?? throw new IOException("El adaptador ya no está activo.");
        await WindowsCommand.ApplyAsAdministrator(Script(current.Index, backup.StaticServers), "");
        File.Delete(file);
    }
}
