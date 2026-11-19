using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Win32;
using ParisLauncher.Performance;
namespace ParisLauncher.Views;
public sealed partial class NetworkAnalyzerView
{
    StabilityTest.Report? stabilityReport;
    void BuildStability()
    {
        var panel=new StackPanel();panel.Children.Add(Text("Estabilidad de Internet y MU · 30 segundos",19));
        panel.Children.Add(Text("Compara Google y Cloudflare por ICMP y el puerto de MU por TCP, sin saturar la conexión."));
        var actions=new StackPanel{Orientation=Orientation.Horizontal};
        var run=Button("Iniciar prueba de 30 segundos");var cancel=Button("Cancelar");var save=Button("Guardar evidencia");
        save.IsEnabled=stabilityReport is not null;cancel.IsEnabled=false;
        actions.Children.Add(run);actions.Children.Add(cancel);actions.Children.Add(save);panel.Children.Add(actions);
        var output=Text(stabilityReport is null?"Sin prueba todavía.":StabilityTest.Describe(stabilityReport));panel.Children.Add(output);
        CancellationTokenSource? cancellation=null;
        cancel.Click+=(_,_)=>cancellation?.Cancel();
        run.Click+=async(_,_)=>
        {
            run.IsEnabled=false;cancel.IsEnabled=true;save.IsEnabled=false;
            cancellation=CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token);
            try
            {
                stabilityReport=await StabilityTest.Run(ServerHost,ServerPort,new Progress<StabilityTest.Report>(report=>output.Text=StabilityTest.Describe(report)),cancellation.Token);
                output.Text="Prueba completada\n"+StabilityTest.Describe(stabilityReport);save.IsEnabled=true;AddEvent("Prueba de estabilidad de 30 segundos completada");
            }
            catch(OperationCanceledException){output.Text="Prueba cancelada; no se certifica una medición completa de 30 segundos.";save.IsEnabled=stabilityReport is not null;}
            catch(Exception e){output.Text="No se completó: "+e.Message;save.IsEnabled=stabilityReport is not null;}
            finally{cancellation.Dispose();cancellation=null;run.IsEnabled=true;cancel.IsEnabled=false;}
        };
        save.Click+=(_,_)=>SaveStability();games.Children.Add(Card(panel));
    }
    void SaveStability()
    {
        if(stabilityReport is null)return;
        var dialog=new SaveFileDialog{Filter="Evidencia JSON|*.json",FileName=$"Paris-conexion-{DateTime.Now:yyyyMMdd-HHmmss}.json"};
        if(dialog.ShowDialog(Window.GetWindow(this))!=true)return;
        try{File.WriteAllText(dialog.FileName,JsonSerializer.Serialize(stabilityReport,new JsonSerializerOptions{WriteIndented=true}));}
        catch(Exception e) when(e is IOException or UnauthorizedAccessException){MessageBox.Show("No se pudo guardar: "+e.Message,"Paris Project");}
    }
}
