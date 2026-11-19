using System.Diagnostics;
using System.Net.NetworkInformation;
using System.Windows;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using System.Windows.Threading;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed class HardwareMonitorView : UniformGrid
{
    private readonly CpuSampler cpu = new();
    private readonly GpuSampler gpu = new();
    private readonly ResourceChartCard cpuCard = new("CPU · uso total", Brushes.Turquoise);
    private readonly ResourceChartCard ramCard = new("MEMORIA RAM · uso físico", Brushes.CornflowerBlue);
    private readonly ResourceChartCard gpuCard = new("GPU · motores 3D combinados", Brushes.MediumOrchid);
    private readonly ResourceChartCard netCard = new("RED · descarga + subida (Mbps)", Brushes.Goldenrod, false);
    private readonly Dictionary<string, (long Down, long Up)> previous = [];
    private readonly Stopwatch elapsed = Stopwatch.StartNew();
    private readonly DispatcherTimer timer = new() { Interval = TimeSpan.FromSeconds(1) };
    public HardwareMonitorView()
    {
        Columns = 2; Children.Add(cpuCard); Children.Add(ramCard); Children.Add(gpuCard); Children.Add(netCard);
        timer.Tick += (_, _) => Update();
        Loaded += (_, _) => { Update(); timer.Start(); };
        Unloaded += (_, _) => { timer.Stop(); gpu.Dispose(); previous.Clear(); };
    }
    private void Update()
    {
        double? usage = cpu.Read(); cpuCard.Sample(usage, usage.HasValue ? $"{usage:0}%" : "Midiendo…");
        var memory = MemoryTools.Snapshot(); const double gib = 1073741824.0;
        ramCard.Sample(memory.Load, $"{memory.Load}% · {(memory.Total - memory.Available) / gib:0.0} / {memory.Total / gib:0.0} GB");
        double? graphics = gpu.Read(); gpuCard.Sample(graphics, graphics.HasValue ? $"{graphics:0}%" : "Midiendo / no disponible");
        UpdateNetwork();
    }
    private void UpdateNetwork()
    {
        double seconds = elapsed.Elapsed.TotalSeconds; elapsed.Restart(); long down = 0, up = 0;
        foreach (var adapter in NetworkInterface.GetAllNetworkInterfaces().Where(row => row.OperationalStatus == OperationalStatus.Up && row.NetworkInterfaceType != NetworkInterfaceType.Loopback))
        {
            try
            {
                var stats = adapter.GetIPv4Statistics();
                if (previous.TryGetValue(adapter.Id, out var old)) { down += Math.Max(0, stats.BytesReceived - old.Down); up += Math.Max(0, stats.BytesSent - old.Up); }
                previous[adapter.Id] = (stats.BytesReceived, stats.BytesSent);
            }
            catch (NetworkInformationException) { }
        }
        double download = down * 8.0 / Math.Max(seconds, 0.001) / 1000000;
        double upload = up * 8.0 / Math.Max(seconds, 0.001) / 1000000;
        netCard.Sample(download + upload, $"↓ {download:0.0}   ↑ {upload:0.0} Mbps");
    }
}
