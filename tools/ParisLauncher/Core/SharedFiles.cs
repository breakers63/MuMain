using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;

namespace ParisLauncher.Core;

public static class SharedFiles
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CreateHardLink(string fileName, string existingFileName, IntPtr securityAttributes);

    public static void LinkOrCopy(string source, string destination)
    {
        if (CreateHardLink(destination, source, IntPtr.Zero)) return;
        // Non-NTFS destinations may not support links. Copying keeps the install usable.
        File.Copy(source, destination);
    }
}
