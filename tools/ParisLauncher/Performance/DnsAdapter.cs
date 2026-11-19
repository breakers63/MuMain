using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;

namespace ParisLauncher.Performance;

public sealed record DnsAdapter(Guid Id, string Name, int Index, string LocalAddress, string[] CurrentServers)
{
    public string Label => $"{Name} · {LocalAddress}";
    public static List<DnsAdapter> Active()
    {
        var result = new List<DnsAdapter>();
        foreach (var adapter in NetworkInterface.GetAllNetworkInterfaces())
        {
            if (adapter.OperationalStatus != OperationalStatus.Up || adapter.NetworkInterfaceType == NetworkInterfaceType.Loopback || !Guid.TryParse(adapter.Id, out var id)) continue;
            var properties = adapter.GetIPProperties();
            var address = properties.UnicastAddresses.FirstOrDefault(value => value.Address.AddressFamily == AddressFamily.InterNetwork)?.Address;
            if (address is null || properties.GetIPv4Properties() is not { } ipv4) continue;
            result.Add(new(id, adapter.Name, ipv4.Index, address.ToString(), properties.DnsAddresses.Where(value => value.AddressFamily == AddressFamily.InterNetwork).Select(value => value.ToString()).ToArray()));
        }
        return result;
    }
}
