using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace ParisVoice.Client;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        ApplicationConfiguration.Initialize();
        try
        {
            if (args.Length != 3 || !long.TryParse(args[0], out var mapping) || !long.TryParse(args[1], out var parent) || !int.TryParse(args[2], out var pid)) return;
            using var form = new VoiceForm((nint)mapping, (nint)parent, pid);
            Application.Run(form);
        }
        catch (Exception exception)
        {
            var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Voice");
            Directory.CreateDirectory(directory);
            // Log exception type only: credentials must never appear in files.
            File.WriteAllText(Path.Combine(directory, "last-error.txt"), DateTime.UtcNow.ToString("O") + " " + exception.GetType().Name);
        }
    }
}

internal sealed class VoiceForm : Form
{
    private const string Origin = "https://paris-voice.local";
    private readonly nint parent;
    private readonly int parentPid;
    private readonly nint view;
    private readonly WebView2 browser = new();
    private readonly System.Windows.Forms.Timer timer = new() { Interval = 100 };
    private bool ready, minimized = true, closing, dragging;
    private int generation = -1, lastFlags = -1, lastPtt = -1, lastStatus = -1;
    private int lastRestore;
    private uint lastMetrics;
    private Point offset = new(15, 90);
    private string endpoint = "wss://ws.hispanogaming.com/voice/ws";

    public VoiceForm(nint mapping, nint parent, int parentPid)
    {
        this.parent = parent;
        this.parentPid = parentPid;
        view = Native.MapViewOfFile(mapping, 0x000F001F, 0, 0, 128);
        Native.CloseHandle(mapping);
        if (view == 0 || Marshal.ReadInt32(view) != 0x3156504D) throw new InvalidOperationException("Invalid voice control");
        FormBorderStyle = FormBorderStyle.None;
        ShowInTaskbar = false;
        // Owned WinForms windows can be sent behind the SDL/OpenGL window when
        // the game raises itself. Only show this overlay while its game is active.
        TopMost = true;
        Text = "Paris Project · Chat de voz";
        BackColor = Color.Black;
        StartPosition = FormStartPosition.Manual;
        browser.Dock = DockStyle.Fill;
        browser.DefaultBackgroundColor = Color.Black;
        Controls.Add(browser);
        Load += async (_, _) => await InitializeAsync();
        FormClosed += (_, _) => Cleanup();
        timer.Tick += (_, _) => Tick();
        timer.Start();
    }

    protected override bool ShowWithoutActivation => true;

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        Native.SetWindowLongPtr(Handle, -8, parent);
    }

    private async Task InitializeAsync()
    {
        try
        {
            var config = Path.Combine(AppContext.BaseDirectory, "voice-settings.json");
            if (File.Exists(config))
            {
                using var json = JsonDocument.Parse(await File.ReadAllTextAsync(config));
                var value = json.RootElement.GetProperty("endpoint").GetString();
                if (Uri.TryCreate(value, UriKind.Absolute, out var uri) && (uri.Scheme == "wss" && uri.Host == "ws.hispanogaming.com" || uri.Scheme == "ws" && uri.IsLoopback)) endpoint = uri.ToString();
            }

            var profile = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Voice", "WebView2");
            var environment = await CoreWebView2Environment.CreateAsync(null, profile, new CoreWebView2EnvironmentOptions("--autoplay-policy=no-user-gesture-required"));
            await browser.EnsureCoreWebView2Async(environment);
            browser.CoreWebView2.SetVirtualHostNameToFolderMapping("paris-voice.local", Path.Combine(AppContext.BaseDirectory, "UI"), CoreWebView2HostResourceAccessKind.DenyCors);
            browser.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
            browser.CoreWebView2.Settings.AreDevToolsEnabled = false;
            browser.CoreWebView2.Settings.IsStatusBarEnabled = false;
            browser.CoreWebView2.Settings.IsZoomControlEnabled = false;
            browser.CoreWebView2.Settings.AreBrowserAcceleratorKeysEnabled = false;
            browser.CoreWebView2.NavigationStarting += (_, e) => { if (!e.Uri.StartsWith(Origin + "/", StringComparison.Ordinal)) e.Cancel = true; };
            browser.CoreWebView2.NewWindowRequested += (_, e) => e.Handled = true;
            browser.CoreWebView2.PermissionRequested += (_, e) =>
            {
                e.State = CoreWebView2PermissionState.Deny;
                e.SavesInProfile = false;
                if (e.PermissionKind == CoreWebView2PermissionKind.Microphone && e.Uri.StartsWith(Origin + "/", StringComparison.Ordinal))
                {
                    e.State = MessageBox.Show(this, "¿Permitir que Paris Project use el micrófono para el chat de voz?", "Paris Project · Micrófono", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes ? CoreWebView2PermissionState.Allow : CoreWebView2PermissionState.Deny;
                    e.SavesInProfile = e.State == CoreWebView2PermissionState.Allow;
                }
            };
            browser.CoreWebView2.WebMessageReceived += OnMessage;
            browser.CoreWebView2.NavigationCompleted += async (_, e) =>
            {
                if (!e.IsSuccess) return;
                ready = true;
                Post(new { type = "config", endpoint, minimized });
                await ScaleAsync();
            };
            browser.Source = new Uri(Origin + "/index.html");
        }
        catch (Exception exception)
        {
            MessageBox.Show(this, "No se pudo iniciar la interfaz de voz (" + exception.GetType().Name + ").", "Paris Project", MessageBoxButtons.OK, MessageBoxIcon.Information);
            Close();
        }
    }

    private void OnMessage(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
    {
        if (!e.Source.StartsWith(Origin + "/", StringComparison.Ordinal)) return;
        try
        {
            using var json = JsonDocument.Parse(e.WebMessageAsJson);
            var message = json.RootElement;
            switch (message.GetProperty("type").GetString())
            {
                case "microphoneState": Marshal.WriteInt32(view, 104, message.GetProperty("enabled").GetBoolean() ? 1 : 0); break;
                case "requestTicket": Marshal.WriteInt32(view, 20, 1); break;
                case "revoke": Marshal.WriteInt32(view, 20, 2); break;
                case "diagnostics":
                    var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Voice");
                    Directory.CreateDirectory(directory);
                    // Only counters and connection states; never audio, names or tickets.
                    File.WriteAllText(Path.Combine(directory, "ipc-" + parentPid + ".json"), JsonSerializer.Serialize(new { status = Marshal.ReadInt32(view, 28), command = Marshal.ReadInt32(view, 20), generation = Marshal.ReadInt32(view, 24) }));
                    var metrics = message.GetProperty("metrics").GetRawText();
                    if (metrics.Length < 16384) File.WriteAllText(Path.Combine(directory, "metrics-" + parentPid + ".json"), metrics);
                    break;
                case "minimize": minimized = message.GetProperty("value").GetBoolean(); Position(); _ = ScaleAsync(); break;
                case "drag":
                    dragging = true;
                    Native.ReleaseCapture();
                    Native.SendMessage(Handle, 0xA1, 2, 0);
                    dragging = false;
                    if (Native.GetClientRect(parent, out var rect))
                    {
                        var origin = new Native.Point(); Native.ClientToScreen(parent, ref origin);
                        offset = new Point(Left - origin.X, Top - origin.Y);
                    }
                    Position();
                    break;
            }
        }
        catch (JsonException) { }
    }

    private void Tick()
    {
        if (closing) return;
        Native.GetWindowThreadProcessId(parent, out var pid);
        if (!Native.IsWindow(parent) || pid != parentPid || Marshal.ReadInt32(view, 4) == 0 || unchecked((uint)Environment.TickCount - (uint)Marshal.ReadInt32(view, 8)) > 5000)
        {
            Close(); return;
        }

        var flags = Marshal.ReadInt32(view, 16);
        var ptt = Marshal.ReadInt32(view, 12);
        var foreground = Native.GetForegroundWindow();
        Native.GetWindowThreadProcessId(foreground, out var foregroundPid);
        var visible = !minimized && (flags & 2) == 0 && !Native.IsIconic(parent) && (foreground == parent || foregroundPid == Environment.ProcessId);
        if (Visible != visible) { if (visible) Show(); else Hide(); }
        Position();
        if (!ready) return;
        if (unchecked((uint)Environment.TickCount - lastMetrics) > 5000)
        {
            lastMetrics = unchecked((uint)Environment.TickCount);
            Post(new { type = "diagnostics" });
        }
        var restore = Marshal.ReadInt32(view, 100);
        if (restore != lastRestore)
        {
            lastRestore = restore;
            minimized = false;
            Post(new { type = "restore" });
            Position();
        }
        var focused = (flags & 1) != 0 || foreground == Handle;
        var computed = focused ? 1 : 0;
        if (computed != lastFlags || ptt != lastPtt)
        {
            Post(new { type = "control", focused, ptt = ptt != 0 }); lastFlags = computed; lastPtt = ptt;
        }

        var status = Marshal.ReadInt32(view, 28);
        if (status == 3 && lastStatus != 3) Post(new { type = "unavailable" });
        lastStatus = status;
        var current = Marshal.ReadInt32(view, 24);
        if (current == generation || (current & 1) != 0) return;
        var bytes = new byte[64]; Marshal.Copy(view + 32, bytes, 0, bytes.Length);
        Thread.MemoryBarrier();
        if (Marshal.ReadInt32(view, 24) != current) return;
        generation = current;
        var ticket = Encoding.ASCII.GetString(bytes);
        if (ticket.All(c => c is >= '0' and <= '9' or >= 'a' and <= 'f')) Post(new { type = "ticket", ticket });
    }

    private void Position()
    {
        if (dragging) return;
        if (!Native.GetClientRect(parent, out var rect)) return;
        var origin = new Native.Point(); Native.ClientToScreen(parent, ref origin);
        var available = Math.Max(100, (int)(rect.Bottom * .84));
        var scale = Math.Min(1.0, Math.Min((rect.Right - 16) / 360.0, (available - 16) / 475.0));
        var width = minimized ? 64 : Math.Max(100, (int)Math.Floor(360 * scale));
        var height = minimized ? 64 : Math.Max(100, (int)Math.Floor(475 * scale));
        var sizeChanged = width != Width || height != Height;
        var x = Math.Clamp(offset.X, 0, Math.Max(0, rect.Right - width));
        var y = Math.Clamp(offset.Y, 0, Math.Max(0, available - height));
        Native.SetWindowPos(Handle, (nint)(-1), origin.X + x, origin.Y + y, width, height, 0x10);
        if (sizeChanged)
        {
            ApplyShape();
            if (ready) _ = ScaleAsync();
        }
    }

    private void ApplyShape()
    {
        if (minimized) return;
        using var image = new Bitmap(Path.Combine(AppContext.BaseDirectory, "UI", "assets", "panel.png"));
        using var path = new System.Drawing.Drawing2D.GraphicsPath();
        for (var y = 0; y < Height; y++)
        {
            var start = -1;
            for (var x = 0; x <= Width; x++)
            {
                var opaque = x < Width && image.GetPixel(Math.Min(image.Width - 1, x * image.Width / Width), Math.Min(image.Height - 1, y * image.Height / Height)).A > 32;
                if (opaque && start < 0) start = x;
                if (!opaque && start >= 0) { path.AddRectangle(new Rectangle(start, y, x - start, 1)); start = -1; }
            }
        }
        var previous = Region;
        Region = new Region(path);
        previous?.Dispose();
    }

    private Task ScaleAsync()
    {
        if (!ready) return Task.CompletedTask;
        // CSS pixels account for WebView DPI; one uniform scale prevents stretched frames.
        return browser.ExecuteScriptAsync("document.body.style.zoom=" + (minimized ? "'1'" : "String(Math.min(innerWidth/360,innerHeight/475))") + ";");
    }

    private void Post(object value) => browser.CoreWebView2?.PostWebMessageAsJson(JsonSerializer.Serialize(value));

    private void Cleanup()
    {
        if (closing) return;
        closing = true;
        timer.Stop(); timer.Dispose();
        Marshal.WriteInt32(view, 20, 2);
        browser.Dispose();
        Native.UnmapViewOfFile(view);
    }
}

internal static partial class Native
{
    [StructLayout(LayoutKind.Sequential)] internal struct Rect { internal int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] internal struct Point { internal int X, Y; }
    [DllImport("kernel32.dll")] internal static extern nint MapViewOfFile(nint mapping, uint access, uint high, uint low, nuint size);
    [DllImport("kernel32.dll")] internal static extern bool UnmapViewOfFile(nint view);
    [DllImport("kernel32.dll")] internal static extern bool CloseHandle(nint handle);
    [DllImport("user32.dll")] internal static extern bool GetClientRect(nint window, out Rect rect);
    [DllImport("user32.dll")] internal static extern bool ClientToScreen(nint window, ref Point point);
    [DllImport("user32.dll")] internal static extern bool IsWindow(nint window);
    [DllImport("user32.dll")] internal static extern bool IsIconic(nint window);
    [DllImport("user32.dll")] internal static extern nint GetForegroundWindow();
    [DllImport("user32.dll")] internal static extern uint GetWindowThreadProcessId(nint window, out int pid);
    [DllImport("user32.dll", EntryPoint="SetWindowLongPtrW")] internal static extern nint SetWindowLongPtr(nint window, int index, nint value);
    [DllImport("user32.dll")] internal static extern bool SetWindowPos(nint window, nint after, int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")] internal static extern bool ReleaseCapture();
    [DllImport("user32.dll", EntryPoint="SendMessageW")] internal static extern nint SendMessage(nint window, uint message, nint wparam, nint lparam);
}
