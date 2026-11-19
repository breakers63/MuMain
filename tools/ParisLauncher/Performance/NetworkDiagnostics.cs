using System.Net;
using System.Net.NetworkInformation;

namespace ParisLauncher.Performance;

public static class NetworkDiagnostics
{
    public static async Task<string> Describe(string host)
    {
        var addresses = await Dns.GetHostAddressesAsync(host);
        var adapters = NetworkInterface.GetAllNetworkInterfaces()
            .Where(adapter => adapter.OperationalStatus == OperationalStatus.Up && adapter.NetworkInterfaceType != NetworkInterfaceType.Loopback)
            .Select(adapter =>
            {
                var properties = adapter.GetIPProperties();
                string dns = string.Join(", ", properties.DnsAddresses.Select(address => address.ToString()));
                return $"{adapter.Name} · {adapter.NetworkInterfaceType}\nDNS: {dns}";
            });
        return $"Servidor: {host}\nResolución DNS: {string.Join(", ", addresses.Select(address => address.ToString()))}\n\n{string.Join("\n\n", adapters)}";
    }
}
