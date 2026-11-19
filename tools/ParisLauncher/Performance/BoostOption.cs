using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace ParisLauncher.Performance;

public sealed class BoostOption(string id, string title, string description, bool advanced = false) : INotifyPropertyChanged
{
    private bool selected;
    public string Id { get; } = id;
    public string Title { get; } = title;
    public string Description { get; } = description;
    public bool Advanced { get; } = advanced;
    public bool Selected { get => selected; set { selected = value; PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Selected))); } }
    public event PropertyChangedEventHandler? PropertyChanged;
    public static List<BoostOption> Create() =>
    [
        new("animations", "Reducir animaciones de ventanas", "Desactiva la animación al minimizar y maximizar. Windows puede requerir cerrar sesión para reflejarlo."),
        new("transparency", "Desactivar transparencias de Windows", "Desactiva transparencias del escritorio. Conserva el valor previo para restaurarlo."),
        new("gpu", "GPU de alto rendimiento", "Preferencia gráfica de Windows para los ejecutables de MU. Se conserva el valor anterior."),
        new("priority", "Prioridad de MU", "Prioridad AboveNormal solo para MU. No utiliza prioridad Realtime ni modifica procesos del sistema."),
        new("capture", "Desactivar captura en segundo plano", "Desactiva Game DVR del usuario. Afecta las capturas automáticas de Xbox Game Bar."),
        new("gamemode", "Modo Juego de Windows", "Activa la preferencia Modo Juego para el usuario actual."),
        new("keyboard", "Repetición del teclado", "Repetición más rápida y demora mínima al mantener una tecla. No acelera la red ni garantiza menor latencia del juego."),
        new("power", "Plan de alto rendimiento", "Crea un plan propio. Puede aumentar consumo, temperatura y uso de batería."),
        new("cores", "Mantener núcleos disponibles", "Desactiva core parking en el plan propio mientras está seleccionado. No cambia la configuración de arranque del procesador.", true),
        new("background", "Reducir prioridad de apps elegidas", "Solo las aplicaciones que marques en RAM; no toca servicios de Windows ni antivirus.", true),
        new("sysmain", "Pausar SysMain / SuperFetch", "Detiene SysMain y guarda su inicio original. Puede ralentizar cargas; necesita administrador.", true),
        new("search", "Pausar indexación de Windows", "Detiene Windows Search. La búsqueda puede funcionar peor; se guarda su estado para restaurarlo.", true),
        new("counters", "Desactivar contadores de diagnóstico", "Opción avanzada: desactiva contadores PerfOS y PerfProc. Afecta herramientas de diagnóstico; puede requerir reiniciar.", true),
        new("lastaccess", "Desactivar registro de último acceso", "Cambia la política NTFS de última lectura. Puede afectar herramientas de copia de seguridad; no garantiza más FPS.", true),
    ];
}
