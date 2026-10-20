#include "stdafx.h"
#include "Character/CharacterManager.h"
#include "UI/NewUI/Character/ParisAllocationWindow.h"
#include "UI/NewUI/Character/ParisAllocationArt.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/NewUISystem.h"
#include "GameLogic/Character/StatAllocationState.h"
#include "Network/Paris/StatServerLimits.h"
#include "Network/Paris/ParisRequests.h"
#include "Network/Server/WSclient.h"
#include "Network/Server/SocketSystem.h"
namespace UI::Character::Allocation
{
GameLogic::Character::StatAllocationState g_ParisAllocation;
UI::Character::ParisAllocationArt g_AllocationArt47;
constexpr int ParisAllocationRowY = 112, ParisAllocationRowStep = 38;
constexpr DWORD ParisAllocationTimeout = 5000, ParisAllocationInterval = 50;
DWORD g_ParisLimitRequestAt = 0;
int g_ParisAutoBudget = 0;
void DrawAllocationSolid(int x, int y, int w, int h, float r, float g, float b)
{
    UI::Paris::Color(r, g, b, 1);
    RenderColor(x, y, w, h);
    EndRenderColor();
}
void ParisAllocationText(int x, int y, const wchar_t* text, int width = 158, bool disabled = false)
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const BYTE brightness = disabled ? 135 : 245;
    g_pRenderText->SetTextColor(brightness, brightness, brightness, 255);
    g_pRenderText->RenderText(x, y, text, width, 0, RT3_SORT_CENTER);
}

void RenderParisCheckbox(int x, int y, bool checked)
{
    constexpr int size = 14;
    g_AllocationArt47.Frame(x, y, size, size, UI::Character::ParisAllocationArt::Checkbox);
    if (!checked)
        return;
    for (int i = 0; i < 4; ++i)
        DrawAllocationSolid(x + 2 + i, y + 5 + i, 2, 2, 1.f, .8f, .2f);
    for (int i = 0; i < 6; ++i)
        DrawAllocationSolid(x + 5 + i, y + 8 - i, 2, 2, 1.f, .8f, .2f);
}

int ParisAllocationCount()
{
    return gCharacterManager.GetBaseClass(Hero->Class) == CLASS_DARK_LORD ? 5 : 4;
}

unsigned int ParisBaseAttribute(int index)
{
    switch (index)
    {
    case 0:
        return CharacterAttribute->Strength;
    case 1:
        return CharacterAttribute->Dexterity;
    case 2:
        return CharacterAttribute->Vitality;
    case 3:
        return CharacterAttribute->Energy;
    case 4:
        return CharacterAttribute->Charisma;
    default:
        return 0;
    }
}

void RequestParisStatLimits()
{
    using namespace Network::Paris::ServerLimits;
    if (!SocketClient || !SocketClient->IsConnected() || !Hero)
        return;
    ++Request;
    Ready = false;
    Pending = true;
    g_ParisLimitRequestAt = GetTickCount();
    wchar_t command[48];
    mu_swprintf(command, L"/parislimits %u", Request);
    Network::Paris::RequestJewelBank(Hero->ID, command);
}

std::array<int, 5> ParisStatRoom()
{
    std::array<int, 5> room{};
    if (!Network::Paris::ServerLimits::Ready)
        return room;
    for (int i = 0; i < ParisAllocationCount(); ++i)
        room[i] = std::max(0, Network::Paris::ServerLimits::Values[i] - static_cast<int>(ParisBaseAttribute(i)));
    return room;
}

void UpdateParisAutomaticAllocation()
{
    auto& state = g_ParisAllocation;
    static wchar_t owner[128]{};
    static unsigned int previousPoints = 0, previousLevel = 0, previousReset = 0;
    if (!Hero || !CharacterAttribute || SceneFlag != MAIN_SCENE || !SocketClient || !SocketClient->IsConnected())
    {
        state.automatic = false;
        Network::Paris::ServerLimits::Ready = Network::Paris::ServerLimits::Pending = false;
        owner[0] = L'\0';
        g_ParisAutoBudget = 0;
        return;
    }
    const auto points = CharacterAttribute->LevelUpPoint;
    const auto level = CharacterAttribute->Level;
    const auto resets = CharacterAttribute->Resets;
    if (wcscmp(owner, CharacterAttribute->Name) != 0 || resets != previousReset || level < previousLevel)
    {
        state = {};
        Network::Paris::ServerLimits::Ready = Network::Paris::ServerLimits::Pending = false;
        wcscpy_s(owner, CharacterAttribute->Name);
        g_ParisAutoBudget = 0;
    }
    else if (state.automatic && points > previousPoints)
    {
        g_ParisAutoBudget += static_cast<int>(points - previousPoints);
        if (!state.applying && !Network::Paris::ServerLimits::Pending)
            RequestParisStatLimits();
    }
    previousPoints = points;
    previousLevel = level;
    previousReset = resets;
    if (Network::Paris::ServerLimits::Pending && GetTickCount() - g_ParisLimitRequestAt >= ParisAllocationTimeout)
    {
        Network::Paris::ServerLimits::Pending = false;
        state.automatic = false;
        state.status = L"Sin limites del servidor. Revisa plugin.";
    }
    if (!state.automatic || state.applying || !Network::Paris::ServerLimits::Ready || g_ParisAutoBudget <= 0)
        return;
    state.PlanAutomatic(g_ParisAutoBudget, static_cast<int>(points), ParisStatRoom());
    g_ParisAutoBudget = 0;
    state.applying = state.Total() > 0;
    if (state.applying)
        state.status = L"Repartiendo automaticamente...";
}

void UpdateParisAllocationAcknowledgement(DWORD now)
{
    auto& state = g_ParisAllocation;
    const bool confirmed = ParisBaseAttribute(state.pendingStat) == state.previousValue + state.pendingAmount &&
                           CharacterAttribute->LevelUpPoint < state.previousPoints;
    if (confirmed)
    {
        state.amounts[state.pendingStat] = 0;
        state.waiting = false;
        if (state.stopRequested)
            state.Stop(L"Detenido. Se conserva lo confirmado.");
    }
    else if (ParisBaseAttribute(state.pendingStat) > state.previousValue &&
             CharacterAttribute->LevelUpPoint < state.previousPoints)
    {
        state.automatic = false;
        state.Stop(L"El servidor aplico un limite. Revisa stats.");
    }
    else if (now - state.sentAt >= ParisAllocationTimeout)
    {
        // Never retry an unconfirmed request: a late reply must not spend points twice.
        state.automatic = false;
        state.Stop(L"Sin confirmacion. Revisa tus puntos.");
    }
}

void UpdateParisPointAllocation(bool visible)
{
    auto& state = g_ParisAllocation;
    if (SceneFlag != MAIN_SCENE || Hero == nullptr || CharacterAttribute == nullptr || SocketClient == nullptr ||
        !SocketClient->IsConnected() || SocketClient->ToGameServer() == nullptr)
    {
        state.Stop(L"Asignacion cerrada.");
        state.open = false;
        return;
    }
    if (!visible && !state.automatic)
    {
        if (state.waiting)
        {
            state.stopRequested = true;
            UpdateParisAllocationAcknowledgement(GetTickCount());
        }
        else
            state.Stop(L"Asignacion cerrada.");
        state.open = false;
        return;
    }
    if (!state.applying)
        return;
    const DWORD now = GetTickCount();
    if (state.waiting)
        UpdateParisAllocationAcknowledgement(now);
    if (!state.applying || state.waiting || now - state.sentAt < ParisAllocationInterval)
        return;
    if (state.stopRequested)
    {
        state.Stop(L"Detenido. Se conserva lo confirmado.");
        return;
    }
    if (state.Total() == 0 || CharacterAttribute->LevelUpPoint == 0)
    {
        state.Stop(state.automatic ? L"Automatico activo. Esperando puntos." : L"Asignacion finalizada.");
        return;
    }
    if (state.Total() > static_cast<int>(CharacterAttribute->LevelUpPoint))
    {
        state.Stop(L"Los puntos disponibles cambiaron.");
        return;
    }
    for (int i = 0; i < ParisAllocationCount(); ++i)
    {
        if (state.amounts[i] == 0)
            continue;
        state.pendingStat = i;
        state.pendingAmount = state.amounts[i];
        state.previousValue = ParisBaseAttribute(i);
        state.previousPoints = CharacterAttribute->LevelUpPoint;
        state.sentAt = now;
        state.waiting = true;
        wchar_t command[48];
        mu_swprintf(command, L"/parisalloc %d %d", i, state.pendingAmount);
        Network::Paris::RequestJewelBank(Hero->ID, command);
        break;
    }
}

void RenderParisAllocationRows(int x, int y)
{
    const wchar_t* labels[] = {L"Fuerza", L"Agilidad", L"Vitalidad", L"Energia", L"Comando"};
    for (int i = 0; i < 5; ++i)
    {
        const int rowY = y + ParisAllocationRowY + i * ParisAllocationRowStep;
        wchar_t amount[24];
        mu_swprintf(amount, L"%d",
                    g_ParisAllocation.percentMode ? g_ParisAllocation.percentages[i] : g_ParisAllocation.amounts[i]);
        ParisAllocationText(x + 16, rowY + 6, labels[i], 48);
        g_AllocationArt47.Frame(x + 75, rowY, 55, 22, UI::Character::ParisAllocationArt::Input);
        if (i == g_ParisAllocation.selectedField && !g_ParisAllocation.applying)
            DrawAllocationSolid(x + 81, rowY + 18, 43, 1, 1.f, .63f, .1f);
        ParisAllocationText(x + 76, rowY + 6, i < ParisAllocationCount() ? amount : L"Solo DL", 53,
                            i >= ParisAllocationCount());
        if (g_ParisAllocation.percentMode)
            ParisAllocationText(x + 139, rowY + 5, L"%", 20);
        else
        {
            RenderParisCheckbox(x + 139, rowY + 3, g_ParisAllocation.full[i]);
            ParisAllocationText(x + 154, rowY + 6, L"Lleno", 24, i >= ParisAllocationCount());
        }
    }
}

void RenderParisAllocationHeading(int x, int y)
{
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(255, 221, 35, 255);
    g_pRenderText->RenderText(x + 12, y + 43, L"REPARTIR PUNTOS", 166, 0, RT3_SORT_CENTER);
    g_AllocationArt47.Frame(x + 14, y + 56, 162, 20, UI::Character::ParisAllocationArt::Available);
    wchar_t available[24];
    mu_swprintf(available, L"%u", CharacterAttribute->LevelUpPoint);
    g_pRenderText->SetFont(g_hFont);
    constexpr const wchar_t* label = L"Puntos disponibles: ";
    SIZE labelSize{}, valueSize{};
    labelSize = g_pRenderText->MeasureText(label, static_cast<int>(wcslen(label)));
    valueSize = g_pRenderText->MeasureText(available, static_cast<int>(wcslen(available)));
    const float scale = g_fScreenRate_x > 0 ? g_fScreenRate_x : 1.f;
    const int labelWidth = static_cast<int>(labelSize.cx / scale), valueWidth = static_cast<int>(valueSize.cx / scale);
    const int left = x + (190 - labelWidth - valueWidth) / 2;
    g_pRenderText->SetTextColor(245, 245, 245, 255);
    g_pRenderText->RenderText(left, y + 62, label, labelWidth, 0, RT3_SORT_LEFT);
    g_pRenderText->SetTextColor(255, 221, 35, 255);
    g_pRenderText->RenderText(left + labelWidth, y + 62, available, valueWidth, 0, RT3_SORT_LEFT);
}

void RenderParisPointAllocation(const POINT& position)
{
    if (!g_ParisAllocation.open)
        return;
    const int x = position.x, y = position.y;
    UI::Paris::Frame(x, y, 190, 429);
    RenderParisAllocationHeading(x, y);
    RenderParisAllocationRows(x, y);
    g_AllocationArt47.Frame(x + 12, y + 310, 79, 22,
                            g_ParisAllocation.percentMode ? UI::Character::ParisAllocationArt::Percent
                                                          : UI::Character::ParisAllocationArt::Amounts);
    ParisAllocationText(x + 12, y + 316, L"Cantidades", 79);
    g_AllocationArt47.Frame(x + 99, y + 310, 79, 22,
                            g_ParisAllocation.percentMode ? UI::Character::ParisAllocationArt::Amounts
                                                          : UI::Character::ParisAllocationArt::Percent);
    ParisAllocationText(x + 99, y + 316, L"Porcentajes", 79);
    RenderParisCheckbox(x + 18, y + 343, g_ParisAllocation.automatic);
    ParisAllocationText(x + 35, y + 346, L"Anadir automaticamente", 138);
    ParisAllocationText(x + 12, y + 365, g_ParisAllocation.status, 166);
    g_AllocationArt47.Frame(x + 12, y + 384, 79, 24, UI::Character::ParisAllocationArt::Assign);
    g_AllocationArt47.Frame(x + 99, y + 384, 79, 24, UI::Character::ParisAllocationArt::Close);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 225, 105, 255);
    g_pRenderText->RenderText(x + 12, y + 391, L"ASIGNAR", 79, 0, RT3_SORT_CENTER);
    g_pRenderText->RenderText(x + 99, y + 391, g_ParisAllocation.applying ? L"Detener" : L"CERRAR", 79, 0,
                              RT3_SORT_CENTER);
}

void HandleParisAllocationControls(int x, int y)
{
    auto& state = g_ParisAllocation;
    if (state.automatic && SEASON3B::CheckMouseIn(x + 169, y + 7, 13, 12))
    {
        state.open = false;
        return;
    }
    if (SEASON3B::CheckMouseIn(x + 99, y + 384, 79, 24) || SEASON3B::CheckMouseIn(x + 169, y + 7, 13, 12))
    {
        if (state.applying)
        {
            state.stopRequested = true;
            state.automatic = false;
        }
        else
            state.open = false;
        return;
    }
    if (SEASON3B::CheckMouseIn(x + 18, y + 341, 157, 18))
    {
        if (state.automatic)
        {
            state.automatic = false;
            state.stopRequested = state.applying;
            return;
        }
        if (!state.StartAutomatic())
            return;
        g_ParisAutoBudget = CharacterAttribute->LevelUpPoint;
        RequestParisStatLimits();
        return;
    }
    if (state.applying || state.automatic)
        return;
    if (SEASON3B::CheckMouseIn(x + 12, y + 310, 79, 22))
        state.percentMode = false;
    if (SEASON3B::CheckMouseIn(x + 99, y + 310, 79, 22))
        state.percentMode = true;
    for (int i = 0; i < ParisAllocationCount(); ++i)
    {
        const int rowY = y + ParisAllocationRowY + i * ParisAllocationRowStep;
        if (SEASON3B::CheckMouseIn(x + 75, rowY, 55, 22))
        {
            state.selectedField = i;
            state.replaceField = true;
            state.full[i] = false;
        }
        if (!state.percentMode && SEASON3B::CheckMouseIn(x + 138, rowY, 40, 22))
        {
            if (!Network::Paris::ServerLimits::Ready)
            {
                state.status = L"Esperando limites del servidor.";
                RequestParisStatLimits();
                return;
            }
            state.full[i] = !state.full[i];
            if (!state.full[i])
                state.amounts[i] = 0;
            state.PlanFull(CharacterAttribute->LevelUpPoint, ParisStatRoom());
        }
    }
    if (SEASON3B::CheckMouseIn(x + 12, y + 384, 79, 24))
    {
        if (!Network::Paris::ServerLimits::Ready)
        {
            state.status = L"Esperando limites del servidor.";
            RequestParisStatLimits();
            return;
        }
        if (state.percentMode)
        {
            if (state.PercentTotal() <= 0 || state.PercentTotal() > 100)
            {
                state.status = L"La suma debe ser de 1 a 100%.";
                return;
            }
            state.remainders.fill(0);
            state.PlanPercent(CharacterAttribute->LevelUpPoint, ParisStatRoom());
        }
        else
            state.PlanFull(CharacterAttribute->LevelUpPoint, ParisStatRoom());
        if (state.Total() == 0)
        {
            state.status = L"Sin puntos para asignar.";
            return;
        }
        if (state.Total() > static_cast<int>(CharacterAttribute->LevelUpPoint))
        {
            state.Stop(L"Los puntos disponibles cambiaron.");
            return;
        }
        state.applying = true;
        state.status = L"Esperando al servidor...";
    }
}

void ProcessParisAllocationKeyboard()
{
    auto& state = g_ParisAllocation;
    if (state.applying || state.automatic)
        return;
    auto& inputs = state.percentMode ? state.percentages : state.amounts;
    if (SEASON3B::IsPress(VK_TAB))
    {
        state.selectedField = (state.selectedField + 1) % ParisAllocationCount();
        state.replaceField = true;
    }
    if (SEASON3B::IsPress(VK_BACK))
    {
        inputs[state.selectedField] /= 10;
        state.replaceField = false;
    }
    if (SEASON3B::IsPress(VK_DELETE))
        inputs[state.selectedField] = 0;
    for (int digit = 0; digit <= 9; ++digit)
        if (SEASON3B::IsPress('0' + digit) || SEASON3B::IsPress(VK_NUMPAD0 + digit))
            state.EnterDigit(digit);
}
} // namespace UI::Character::Allocation

using namespace SEASON3B;
namespace Allocation = UI::Character::Allocation;
CParisAllocationWindow::~CParisAllocationWindow()
{
    Release();
}
bool CParisAllocationWindow::Create(CNewUIManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    constexpr DWORD AllocationInterface = 0x50414C4C;
    manager->AddUIObj(AllocationInterface, this);
    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    Allocation::g_AllocationArt47.Load();
    Show(false);
    return true;
}
void CParisAllocationWindow::Release()
{
    if (!m_manager)
        return;
    Allocation::g_ParisAllocation = {};
    Network::Paris::ServerLimits::Ready = Network::Paris::ServerLimits::Pending = false;
    Allocation::g_AllocationArt47.Unload();
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisAllocationWindow::ToggleWindow()
{
    auto& state = Allocation::g_ParisAllocation;
    state.open = !IsVisible();
    if (state.open)
    {
        g_pNewUISystem->HideAll();
        g_pNewUISystem->Show(INTERFACE_CHARACTER);
        Allocation::RequestParisStatLimits();
    }
    Show(state.open);
}
bool CParisAllocationWindow::UpdateMouseEvent()
{
    if (!IsVisible())
        return true;
    constexpr int PanelX = 450, PanelY = 0;
    if (!CheckMouseIn(PanelX, PanelY, 190, 429))
        return true;
    if (IsPress(VK_LBUTTON))
        Allocation::HandleParisAllocationControls(PanelX, PanelY);
    UI::Paris::ConsumeMouse();
    Show(Allocation::g_ParisAllocation.open);
    return false;
}
bool CParisAllocationWindow::UpdateKeyEvent()
{
    if (!IsVisible())
        return true;
    if (IsPress(VK_ESCAPE))
    {
        Allocation::g_ParisAllocation.open = false;
        Show(false);
    }
    else
        Allocation::ProcessParisAllocationKeyboard();
    return false;
}
bool CParisAllocationWindow::Update()
{
    Allocation::UpdateParisAutomaticAllocation();
    Allocation::UpdateParisPointAllocation(IsVisible());
    Allocation::g_ParisAllocation.open = IsVisible() && Allocation::g_ParisAllocation.open;
    Show(Allocation::g_ParisAllocation.open);
    return true;
}
bool CParisAllocationWindow::Render()
{
    if (IsVisible())
        Allocation::RenderParisPointAllocation(POINT{450, 0});
    DisableAlphaBlend();
    return true;
}
float CParisAllocationWindow::GetLayerDepth()
{
    return 9.0f;
}
float CParisAllocationWindow::GetKeyEventOrder()
{
    return 2.0f;
}
