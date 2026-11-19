using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
namespace ParisLauncher.Performance;
public static class TrafficLimiter
{
    private static string Journal=>Path.Combine(BoostPreferences.DirectoryPath,"traffic-limits.txt");
    public static string PolicyName(string path)=>"ParisProject-"+Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Path.GetFullPath(path).ToUpperInvariant())))[..20];
    public static string CreateScript(string path,decimal mbps)
    {
        if(!Path.IsPathFullyQualified(path)||!path.EndsWith(".exe",StringComparison.OrdinalIgnoreCase)||mbps is <0.1m or >10000m)throw new ArgumentException("Aplicación o límite inválido.");
        if(Path.GetFileNameWithoutExtension(path).Equals("Main",StringComparison.OrdinalIgnoreCase)||Path.GetFileNameWithoutExtension(path).Equals("Jugar",StringComparison.OrdinalIgnoreCase)||Path.GetFileNameWithoutExtension(path).Equals("Launcher",StringComparison.OrdinalIgnoreCase))throw new ArgumentException("MU y el launcher quedan excluidos.");
        if(path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.Windows),StringComparison.OrdinalIgnoreCase))throw new ArgumentException("Los procesos de Windows quedan excluidos.");
        string name=PolicyName(path);long rate=(long)(mbps*1_000_000m);
        return $"$parisCreated=$false;Get-Command New-NetQosPolicy -ErrorAction Stop|Out-Null;if(Get-NetQosPolicy -Name '{name}' -PolicyStore ActiveStore -ErrorAction SilentlyContinue){{throw 'La aplicación ya tiene un límite Paris. Quítalo antes de cambiarlo.'}};New-NetQosPolicy -Name '{name}' -AppPathNameMatchCondition {WindowsCommand.Quote(path)} -ThrottleRateActionBitsPerSecond {rate} -PolicyStore ActiveStore -NetworkProfile All -ErrorAction Stop|Out-Null;$parisCreated=$true;if(!(Get-NetQosPolicy -Name '{name}' -PolicyStore ActiveStore -ErrorAction Stop)){{throw 'No se pudo verificar la política'}}";
    }
    public static string RemoveScript(string name)
    {
        if(!Regex.IsMatch(name,"^ParisProject-[A-F0-9]{20}$"))throw new ArgumentException("Nombre de política inválido.");
        return $"if(Get-NetQosPolicy -Name '{name}' -PolicyStore ActiveStore -ErrorAction SilentlyContinue){{Remove-NetQosPolicy -Name '{name}' -PolicyStore ActiveStore -Confirm:$false -ErrorAction Stop}}";
    }
    public static async Task Apply(string path,decimal mbps)
    {
        string script=CreateScript(path,mbps),name=PolicyName(path);Directory.CreateDirectory(BoostPreferences.DirectoryPath);var names=Names().Append(name).Distinct().ToArray();File.WriteAllLines(Journal,names);
        await WindowsCommand.ApplyAsAdministrator(script,"if($parisCreated){"+RemoveScript(name)+"}");
    }
    private static string[] Names()=>File.Exists(Journal)?File.ReadAllLines(Journal).Where(s=>Regex.IsMatch(s,"^ParisProject-[A-F0-9]{20}$")).ToArray():[];
    public static async Task RemoveAll(){var names=Names();if(names.Length==0)return;await WindowsCommand.ApplyAsAdministrator(string.Join(";",names.Select(RemoveScript)),"");File.Delete(Journal);}
}
