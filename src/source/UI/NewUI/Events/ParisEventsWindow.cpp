#include "stdafx.h"
#include "UI/NewUI/Events/ParisEventsWindow.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/NewUISystem.h"
#include "Network/Paris/EventTimerState.h"
namespace
{
enum S3EventTimerPage
{
    S3_EVENT_PAGE_CLASSIC = 0,
    S3_EVENT_PAGE_INVASIONS,
    S3_EVENT_PAGE_BONUS,
    S3_EVENT_PAGE_MINI_INVASIONS, // MUETRO_MINI_INVASIONS_V1
    S3_EVENT_PAGE_BOSSES,         // ETRO_EVENT_TIMER_V3
    S3_EVENT_PAGE_COUNT
};

// MUETRO_EVENT_TIMER_SYNC_V1
// The H-panel no longer owns event schedules. OpenMU is authoritative and
// sends hidden [#EVT:U|EventId|State|Seconds#] blue-message metadata.
struct S3EventScheduleEntry
{
    unsigned short EventId;
    const wchar_t* Name;
    int Page;
    const wchar_t* TooltipLine1;
    const wchar_t* TooltipLine2;
    const wchar_t* RewardLine1;
    const wchar_t* RewardLine2;
};

enum S3EventTimerId
{
    S3_EVENT_ID_CHAOS_CASTLE = 1,
    S3_EVENT_ID_DEVIL_SQUARE = 2,
    S3_EVENT_ID_BLOOD_CASTLE = 3,
    S3_EVENT_ID_RED_DRAGON = 4,
    S3_EVENT_ID_GOLDEN_INVASION = 5,
    S3_EVENT_ID_WHITE_WIZARD = 6,
    S3_EVENT_ID_EROHIM = 7,
    S3_EVENT_ID_KUNDUN = 8,
    S3_EVENT_ID_HAPPY_HOUR = 9,
    S3_EVENT_ID_MINI_INVASION_1 = 10, // MUETRO_MINI_INVASIONS_V1
    S3_EVENT_ID_MINI_INVASION_2 = 11,
    S3_EVENT_ID_MINI_INVASION_3 = 12,
    S3_EVENT_ID_MINI_INVASION_4 = 13, // MUETRO_MINI_INVASIONS_V2
    S3_EVENT_ID_MINI_INVASION_5 = 14, // Conservado, oculto en Event Timer
    S3_EVENT_ID_MEDUSA = 15,          // Reservado para futura sincronizacion servidor
    S3_EVENT_ID_GAION = 16,           // Reservado para futura sincronizacion servidor
    S3_EVENT_ID_BOX_OF_FORTUNE = 17,  // Reservado para futura sincronizacion servidor
    S3_EVENT_ID_COUNT = 18
};

static const S3EventScheduleEntry S3_EVENT_SCHEDULE[] = {
    {S3_EVENT_ID_CHAOS_CASTLE, L"Chaos Castle", S3_EVENT_PAGE_CLASSIC, L"Disponible cada hora.",
     L"Las explosiones mueven tu personaje, procura tener cuidado.", L"Ancient random items", L""},
    {S3_EVENT_ID_DEVIL_SQUARE, L"Devil Square", S3_EVENT_PAGE_CLASSIC, L"Golden monsters inside.", L"Duration: 12'",
     L"[ Box of Kundun +1 - +5 ]", L""},
    {S3_EVENT_ID_BLOOD_CASTLE, L"Blood Castle", S3_EVENT_PAGE_CLASSIC, L"Disponible cada hora.",
     L"Recompensa individual para cada participante.", L"[Jewels]", L"Soul, Bless, Chaos, Creation, Life"},

    {S3_EVENT_ID_GOLDEN_INVASION, L"Golden Invasion", S3_EVENT_PAGE_INVASIONS, L"Classic Golden Invasion",
     L"Random Locations.", L"[ Box Of Kundun +1 - +5 ]", L""},
    {S3_EVENT_ID_RED_DRAGON, L"Red Dragon Invasion", S3_EVENT_PAGE_INVASIONS, L"Classic Red Dragons",
     L"Lorencia, Noria: Random respawn.", L"Loch's Feather, Crest of Monarch", L"Bundle Jewel Packs"},
    {S3_EVENT_ID_WHITE_WIZARD, L"White Wizard", S3_EVENT_PAGE_INVASIONS, L"White Wizard and his Minions.",
     L"Lorencia, Noria: Random Respawn.", L"Soul, Bless, Chaos, Creation, Life", L"Wizard Ring"},

    {S3_EVENT_ID_HAPPY_HOUR, L"Happy Hour", S3_EVENT_PAGE_BONUS, L"15' experience and drop increases.",
     L"Avaible on all maps", L"", L""},
    {S3_EVENT_ID_BOX_OF_FORTUNE, L"Box of Fortune", S3_EVENT_PAGE_BONUS, L"Random Box with incredible rewards.",
     L"Lorencia, Noria, Devias: Random location.", L"Powerfull Equipment", L"Powerfull Weapons"},

    {S3_EVENT_ID_MINI_INVASION_1, L"Bali Invasion", S3_EVENT_PAGE_MINI_INVASIONS, L"Bali Monsters",
     L"Devias: Random Respawn.", L"Jewel of Soul, Bless, Chaos", L"Monsters count: 15"},
    {S3_EVENT_ID_MINI_INVASION_2, L"Scorpion Invasion", S3_EVENT_PAGE_MINI_INVASIONS, L"Bone Scorpions",
     L"Tarkan: Random Respawn.", L"Jewel of Creation, Life", L"Monsters count: 15"},
    {S3_EVENT_ID_MINI_INVASION_3, L"Strange Rabbit", S3_EVENT_PAGE_MINI_INVASIONS, L"Strange Rabbits",
     L"Lorencia, Noria: Random Respawn.", L"Jewel of Soul, Bless, Chaos", L"Monsters count: 30"},
    {S3_EVENT_ID_MINI_INVASION_4, L"Grizzly", S3_EVENT_PAGE_MINI_INVASIONS, L"Strong Grizzly", L"Aida: Random Respawn.",
     L"Loch's Feather, Crest of Monarch, Bundle Jewels", L"Monsters count: 5"},

    {S3_EVENT_ID_EROHIM, L"Erohim", S3_EVENT_PAGE_BOSSES, L"Erohim - Strong Boss", L"Aida: Random Respawn.",
     L"Boxs of Kundun [ +3 - +5 ]", L"Excellent Weapons"},
    {S3_EVENT_ID_KUNDUN, L"Lord of Kundun", S3_EVENT_PAGE_BOSSES, L"Lord of Kundun - Strong Boss",
     L"Lorencia - Noria: Random respawn", L"Boxs of Kundun [ +5 ]", L"Excellent Armours"},
    {S3_EVENT_ID_MEDUSA, L"Medusa", S3_EVENT_PAGE_BOSSES, L"Medusa - Ultra Strong Boss", L"Tarkan: Random respawn",
     L"Excellent Weapons", L"Flame of Condor, Powerfull Equipment"},
    {S3_EVENT_ID_GAION, L"Gaion the Gladiator", S3_EVENT_PAGE_BOSSES, L"Gaion the Gladiator - Strong Boss",
     L"Devias: Random respawn", L"Excellent Armours", L"Powerfull Weapons"},
};

static const wchar_t* GetEventTimerPageTitle(int page)
{
    switch (page)
    {
    case S3_EVENT_PAGE_CLASSIC:
        return L"Eventos Clasicos";
    case S3_EVENT_PAGE_INVASIONS:
        return L"Invasiones";
    case S3_EVENT_PAGE_BONUS:
        return L"Bonus";
    case S3_EVENT_PAGE_MINI_INVASIONS:
        return L"Mini Invasions"; // MUETRO_MINI_INVASIONS_V1
    case S3_EVENT_PAGE_BOSSES:
        return L"Bosses"; // ETRO_EVENT_TIMER_V3
    default:
        return L"Eventos";
    }
}
static void FormatCountdown(int totalSeconds, wchar_t* buffer, int bufferCount)
{
    if (buffer == NULL || bufferCount <= 0)
    {
        return;
    }

    if (totalSeconds < 0)
    {
        totalSeconds = 0;
    }

    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    swprintf_s(buffer, bufferCount, L"%02d:%02d:%02d", hours, minutes, seconds);
}

constexpr int PanelX = 450, PanelY = 0, PanelWidth = 190, PanelHeight = 429;
constexpr float ScaleY = 429.f / 512.f;
void Text(int x, int y, const wchar_t* text, int width = 0, int align = RT3_SORT_LEFT)
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(235, 220, 185, 255);
    g_pRenderText->RenderText(x, y, text, width, 0, align);
}
void Tooltip(const S3EventScheduleEntry& event, int rowY)
{
    constexpr int Width = 214, Margin = 7, LineHeight = 13;
    const int rewardCount = (*event.RewardLine1 ? 1 : 0) + (*event.RewardLine2 ? 1 : 0);
    const int height = rewardCount == 0 ? 58 : rewardCount == 1 ? 93 : 106;
    const int x = PanelX - Width - 6, y = std::clamp(rowY - 2, 4, 475 - height);
    RenderColorQuadARGB(x, y, Width, height, 0xEB050505u);
    constexpr unsigned int Border = 0x80565656u;
    RenderColorQuadARGB(x, y, Width, 1, Border);
    RenderColorQuadARGB(x, y + height - 1, Width, 1, Border);
    RenderColorQuadARGB(x, y, 1, height, Border);
    RenderColorQuadARGB(x + Width - 1, y, 1, height, Border);
    Text(x + Margin, y + 5, event.Name, Width - 2 * Margin, RT3_SORT_CENTER);
    Text(x + Margin, y + 26, event.TooltipLine1, Width - 2 * Margin, RT3_SORT_CENTER);
    Text(x + Margin, y + 40, event.TooltipLine2, Width - 2 * Margin, RT3_SORT_CENTER);
    if (!rewardCount)
        return;
    Text(x + Margin, y + 64, L"Posibles premios", Width - 2 * Margin, RT3_SORT_CENTER);
    int rewardY = y + 79;
    for (const auto* line : {event.RewardLine1, event.RewardLine2})
        if (*line)
        {
            Text(x + Margin, rewardY, line, Width - 2 * Margin, RT3_SORT_CENTER);
            rewardY += LineHeight;
        }
}
} // namespace
using namespace SEASON3B;
CParisEventsWindow::~CParisEventsWindow()
{
    Release();
}
bool CParisEventsWindow::Create(CNewUIManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    constexpr DWORD EventsInterface = 0x50455654;
    manager->AddUIObj(EventsInterface, this);
    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    Show(false);
    return true;
}
void CParisEventsWindow::Release()
{
    if (!m_manager)
        return;
    Network::Paris::EventTimers.Clear();
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisEventsWindow::ToggleWindow()
{
    const bool opening = !IsVisible();
    if (opening)
        g_pNewUISystem->HideAll();
    Show(opening);
}
bool CParisEventsWindow::UpdateMouseEvent()
{
    if (!IsVisible() || !CheckMouseIn(PanelX, PanelY, PanelWidth, PanelHeight))
        return true;
    if (IsPress(VK_LBUTTON))
    {
        if (CheckMouseIn(PanelX + 169, PanelY + 7, 13, 12))
            Show(false);
        if (CheckMouseIn(PanelX + 35, PanelY + 375, 22, 22))
            m_page = (m_page + S3_EVENT_PAGE_COUNT - 1) % S3_EVENT_PAGE_COUNT;
        if (CheckMouseIn(PanelX + 136, PanelY + 375, 22, 22))
            m_page = (m_page + 1) % S3_EVENT_PAGE_COUNT;
    }
    UI::Paris::ConsumeMouse();
    return false;
}
bool CParisEventsWindow::UpdateKeyEvent()
{
    if (!IsVisible() || !IsPress(VK_ESCAPE))
        return true;
    Show(false);
    return false;
}
bool CParisEventsWindow::Update()
{
    if (SceneFlag != MAIN_SCENE)
    {
        Show(false);
        Network::Paris::EventTimers.Clear();
    }
    return true;
}
bool CParisEventsWindow::Render()
{
    if (!IsVisible())
        return true;
    UI::Paris::Frame(PanelX, PanelY);
    Text(PanelX, PanelY + 43, GetEventTimerPageTitle(m_page), PanelWidth, RT3_SORT_CENTER);
    Text(PanelX + 16, PanelY + 68, L"Evento");
    Text(PanelX + 89, PanelY + 68, L"Restante");
    Text(PanelX + 139, PanelY + 68, L"Estado");
    int row = 0;
    for (const auto& event : S3_EVENT_SCHEDULE)
    {
        if (event.Page != m_page)
            continue;
        const int y = PanelY + static_cast<int>((112 + row * 32) * ScaleY);
        RenderColorQuadARGB(PanelX + 12, y, 165, 20, 0x40000000u);
        Text(PanelX + 17, y + 5, event.Name, 70);
        int state = 0, seconds = 0;
        const bool valid = Network::Paris::EventTimers.Get(event.EventId, timeGetTime(), state, seconds);
        wchar_t remaining[32] = L"--:--:--";
        if (valid)
            FormatCountdown(seconds, remaining, _countof(remaining));
        Text(PanelX + 89, y + 5, remaining, 48);
        Text(PanelX + 139, y + 5, valid ? (state ? L"Activo" : L"Espera") : L"Sin datos", 40);
        if (CheckMouseIn(PanelX + 17, y, 70, 20))
            Tooltip(event, y);
        ++row;
    }
    UI::Theme::Bevel(PanelX + 35, PanelY + 375, 22, 22, L"<");
    UI::Theme::Bevel(PanelX + 136, PanelY + 375, 22, 22, L">");
    wchar_t page[32];
    mu_swprintf(page, L"Pagina %d/%d", m_page + 1, S3_EVENT_PAGE_COUNT);
    Text(PanelX, PanelY + 381, page, PanelWidth, RT3_SORT_CENTER);
    Text(PanelX, PanelY + 410, L"Horarios del servidor", PanelWidth, RT3_SORT_CENTER);
    DisableAlphaBlend();
    return true;
}
float CParisEventsWindow::GetLayerDepth()
{
    return 8.5f;
}
float CParisEventsWindow::GetKeyEventOrder()
{
    return 2.0f;
}
