using Microsoft.Win32;

namespace ParisLauncher.Performance;

public sealed record RegistryChange(string Option, bool Machine, string Path, string Name, RegistryValueKind Kind, string? Value)
{
    public RegistryChange Current()
    {
        using var key = (Machine ? Registry.LocalMachine : Registry.CurrentUser).OpenSubKey(Path);
        object? value = key?.GetValue(Name, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
        if (value is null) return this with { Value = null };
        var kind = key!.GetValueKind(Name);
        if (kind is not (RegistryValueKind.DWord or RegistryValueKind.String)) throw new InvalidOperationException("Tipo de configuración no compatible: " + Name);
        return this with { Kind = kind, Value = Convert.ToString(value, System.Globalization.CultureInfo.InvariantCulture) };
    }
    public void WriteUser()
    {
        Validate();
        if (Machine) throw new InvalidOperationException("Este cambio requiere administrador.");
        using var key = Registry.CurrentUser.CreateSubKey(Path);
        if (Value is null) key.DeleteValue(Name, throwOnMissingValue: false);
        else key.SetValue(Name, Kind == RegistryValueKind.DWord ? int.Parse(Value, System.Globalization.CultureInfo.InvariantCulture) : Value, Kind);
    }
    public string Script()
    {
        Validate();
        string path = WindowsCommand.Quote((Machine ? "HKLM:\\" : "HKCU:\\") + Path);
        string name = WindowsCommand.Quote(Name);
        if (Value is null) return $"if(Test-Path {path}){{Remove-ItemProperty -LiteralPath {path} -Name {name} -ErrorAction SilentlyContinue}}";
        string value = Kind == RegistryValueKind.DWord ? int.Parse(Value, System.Globalization.CultureInfo.InvariantCulture).ToString(System.Globalization.CultureInfo.InvariantCulture) : WindowsCommand.Quote(Value);
        return $"New-Item -Path {path} -Force|Out-Null; New-ItemProperty -LiteralPath {path} -Name {name} -PropertyType {Kind} -Value {value} -Force|Out-Null";
    }
    public void Validate()
    {
        bool known = SystemBoost.RegistryTargets([]).Any(target => target.Option == Option && target.Machine == Machine && target.Path == Path && target.Name == Name);
        bool gpu = Option == "gpu" && !Machine && Path == SystemBoost.GpuRegistryPath && System.IO.Path.IsPathFullyQualified(Name) && Name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase);
        if (!known && !gpu) throw new InvalidOperationException("El respaldo contiene una opción no permitida.");
        if (Kind is not (RegistryValueKind.DWord or RegistryValueKind.String) || Value?.Length > 4096) throw new InvalidOperationException("Valor de respaldo no válido.");
        if (Kind == RegistryValueKind.DWord && Value is not null && !int.TryParse(Value, out _)) throw new InvalidOperationException("Valor numérico no válido.");
    }
}
