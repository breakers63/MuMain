using System.IO;
using System.Security.Cryptography;
using System.Text.Json;

namespace ParisLauncher.Account;

public static class SessionStorage
{
    private static readonly string FilePath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ParisProject", "Launcher", "session.bin");
    public static void Save(LauncherSession session)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
        byte[] data = JsonSerializer.SerializeToUtf8Bytes(session);
        File.WriteAllBytes(FilePath, ProtectedData.Protect(data, null, DataProtectionScope.CurrentUser));
    }
    public static LauncherSession? Load()
    {
        try { return JsonSerializer.Deserialize<LauncherSession>(ProtectedData.Unprotect(File.ReadAllBytes(FilePath), null, DataProtectionScope.CurrentUser)); }
        catch (Exception error) when (error is IOException or CryptographicException or JsonException) { return null; }
    }
    public static void Clear() { if (File.Exists(FilePath)) File.Delete(FilePath); }
}
