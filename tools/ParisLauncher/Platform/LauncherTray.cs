using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
namespace ParisLauncher.Core;
public sealed class LauncherTray : IDisposable
{
    [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] private struct NotifyData
    {
        public int Size;public IntPtr Window;public uint Id,Flags,Message;public IntPtr Icon;
        [MarshalAs(UnmanagedType.ByValTStr,SizeConst=128)]public string Tip;
        public uint State,StateMask;
        [MarshalAs(UnmanagedType.ByValTStr,SizeConst=256)]public string Info;
        public uint Timeout;
        [MarshalAs(UnmanagedType.ByValTStr,SizeConst=64)]public string Title;
        public uint InfoFlags;public Guid Guid;public IntPtr Balloon;
    }
    [DllImport("shell32.dll",CharSet=CharSet.Unicode)]private static extern bool Shell_NotifyIcon(uint message,ref NotifyData data);
    [DllImport("shell32.dll",CharSet=CharSet.Unicode)]private static extern IntPtr ExtractIcon(IntPtr instance,string file,uint index);
    [DllImport("user32.dll")]private static extern bool DestroyIcon(IntPtr icon);
    private const uint Callback=0x8001;
    readonly Window window;readonly HwndSource source;NotifyData data;
    public LauncherTray(Window window)
    {
        this.window=window;IntPtr handle=new WindowInteropHelper(window).EnsureHandle();source=HwndSource.FromHwnd(handle);source.AddHook(Message);
        data=new NotifyData{Size=Marshal.SizeOf<NotifyData>(),Window=handle,Id=1,Flags=7,Message=Callback,Icon=ExtractIcon(IntPtr.Zero,Environment.ProcessPath!,0),Tip="PARIS PROJECT · Haz clic para abrir",Info="",Title=""};
        if(!Shell_NotifyIcon(0,ref data))throw new InvalidOperationException("No se pudo crear el icono de bandeja.");
    }
    IntPtr Message(IntPtr hwnd,int msg,IntPtr wParam,IntPtr lParam,ref bool handled)
    {
        if(msg==Callback&&lParam.ToInt32() is 0x0202 or 0x0203 or 0x0205){window.Show();window.WindowState=WindowState.Normal;window.Activate();handled=true;}return IntPtr.Zero;
    }
    public void Notify(string title,string message){data.Flags=16;data.Title=title;data.Info=message;data.InfoFlags=1;Shell_NotifyIcon(1,ref data);}
    public void Dispose(){Shell_NotifyIcon(2,ref data);source.RemoveHook(Message);if(data.Icon!=IntPtr.Zero)DestroyIcon(data.Icon);}
}
