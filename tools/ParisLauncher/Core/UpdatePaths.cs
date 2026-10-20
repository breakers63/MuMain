using System.IO;

namespace ParisLauncher.Core;

public static class UpdatePaths
{
    private static readonly string[] ProtectedNames = ["config.ini", "launcher.json", "launcher.exe", "preferences.json"];

    public static string Resolve(string root, string relative)
    {
        string normalized = relative.Replace('\\', '/');
        if (string.IsNullOrWhiteSpace(normalized) || normalized.StartsWith('/') || normalized.Contains(':') ||
            normalized.Split('/').Any(p => p is "" or "." or ".." || IsDeviceName(p) || p.EndsWith('.') || p.EndsWith(' ') || p.IndexOfAny(['<', '>', '"', '|', '?', '*']) >= 0))
            throw new InvalidDataException($"Ruta de actualización no válida: {relative}");
        if (normalized.Split('/').Any(p => p.StartsWith(".launcher", StringComparison.OrdinalIgnoreCase)) ||
            ProtectedNames.Contains(Path.GetFileName(normalized), StringComparer.OrdinalIgnoreCase))
            throw new InvalidDataException($"El actualizador no debe reemplazar {relative}.");
        string fullRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
        string target = Path.GetFullPath(Path.Combine(fullRoot, normalized));
        if (!target.StartsWith(fullRoot, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("El archivo está fuera de la carpeta del cliente.");
        RejectLinks(root, target);
        return target;
    }

    private static bool IsDeviceName(string part)
    {
        string stem = part.Split('.')[0].ToUpperInvariant();
        return stem is "CON" or "PRN" or "AUX" or "NUL" ||
            stem.Length == 4 && (stem.StartsWith("COM") || stem.StartsWith("LPT")) && stem[3] is >= '1' and <= '9';
    }

    public static void RejectLinks(string root, string target)
    {
        string? current = target;
        while (current != null)
        {
            if ((File.Exists(current) || Directory.Exists(current)) && (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                throw new InvalidDataException("La actualización no admite enlaces de carpetas o archivos.");
            if (string.Equals(current.TrimEnd(Path.DirectorySeparatorChar), Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar), StringComparison.OrdinalIgnoreCase))
                return;
            current = Path.GetDirectoryName(current);
        }
    }

    public static Uri DownloadUri(Uri manifest, string relative)
    {
        var uri = new Uri(manifest, relative);
        if (uri.Scheme != Uri.UriSchemeHttps || uri.Authority != manifest.Authority || !string.IsNullOrEmpty(uri.UserInfo))
            throw new InvalidDataException("Las descargas deben usar HTTPS en el mismo dominio que el manifiesto.");
        return uri;
    }

    public static Uri WithVersion(Uri uri, string version)
    {
        var builder = new UriBuilder(uri);
        builder.Query = (builder.Query.Length > 1 ? builder.Query[1..] + "&" : "") + "paris_version=" + Uri.EscapeDataString(version);
        return builder.Uri;
    }
}
