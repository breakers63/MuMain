using System.Runtime.InteropServices;

namespace ParisLauncher.Core;

public static class GpuDetector
{
    private const int EnumAdapters1Slot = 12;
    private const int GetDesc1Slot = 10;
    private const uint SoftwareAdapterFlag = 2;
    private const int AdapterNotFound = unchecked((int)0x887A0002);

    [DllImport("dxgi.dll")]
    private static extern int CreateDXGIFactory1(ref Guid iid, out IntPtr factory);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int EnumAdapters1(IntPtr self, uint index, out IntPtr adapter);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int GetDesc1(IntPtr self, out AdapterDescription description);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct AdapterDescription
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)] public string Description;
        public uint VendorId, DeviceId, SubSysId, Revision;
        public nuint DedicatedVideoMemory, DedicatedSystemMemory, SharedSystemMemory;
        public long Luid;
        public uint Flags;
    }

    private static T Method<T>(IntPtr instance, int slot) where T : Delegate =>
        Marshal.GetDelegateForFunctionPointer<T>(Marshal.ReadIntPtr(Marshal.ReadIntPtr(instance), slot * IntPtr.Size));

    public static IReadOnlyList<GpuInfo> Detect()
    {
        var cards = new List<GpuInfo>();
        var iid = new Guid("770aae78-f26f-4dba-a829-253c83d1b387");
        Marshal.ThrowExceptionForHR(CreateDXGIFactory1(ref iid, out IntPtr factory));
        try
        {
            var enumerate = Method<EnumAdapters1>(factory, EnumAdapters1Slot);
            for (uint index = 0; ; ++index)
            {
                int result = enumerate(factory, index, out IntPtr adapter);
                if (result == AdapterNotFound) break;
                Marshal.ThrowExceptionForHR(result);
                try
                {
                    Marshal.ThrowExceptionForHR(Method<GetDesc1>(adapter, GetDesc1Slot)(adapter, out var description));
                    if ((description.Flags & SoftwareAdapterFlag) == 0)
                        cards.Add(new(description.Description.Trim(), (ulong)description.DedicatedVideoMemory, (ulong)description.SharedSystemMemory));
                }
                finally { Marshal.Release(adapter); }
            }
        }
        finally { Marshal.Release(factory); }
        return cards.OrderByDescending(c => c.DedicatedBytes).ToArray();
    }
}
