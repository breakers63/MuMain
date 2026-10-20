#include "stdafx.h"
#include "UI/NewUI/Character/ParisPanelReference.h"
#include "UI/NewUI/Character/ParisResetWindow.h"
#include "UI/NewUI/Character/ParisControls51.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/NewUICharacterInfoWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "Network/Paris/ParisRequests.h"
#include "Network/Server/WSclient.h"
#include "Network/Server/SocketSystem.h"
#include <vector>
#include <string>
#include <cstring>
namespace UI::Character::ResetPanel
{
UI::Theme::ParisFrameAssets Art;
UI::Theme::ParisPanelReference Reference;
UI::Theme::ParisControls51 Buttons;
constexpr int InfoRows = 5, RequirementRows = 4, RewardRows = 4, DetailStep = 14;
bool Open = false, CanReset = false, Pending = false, Dragging = false;
unsigned long long Token = 0;
int NextReset = 0, FirstLine = 0, InfoLine = 0, RewardLine = 0, DragOffset = 0;
DWORD RequestedAt = 0;
std::vector<std::wstring> Lines, Information, Requirements, Rewards;
std::wstring Result;
bool PendingReset = false;
constexpr int Width = 190, Height = 429, VisibleLines = 17, LineTop = 66, LineStep = 16;

int X()
{
    constexpr int FrameSeamOverlap = 10;
    return g_pCharacterInfoWindow
               ? (std::max)(4, static_cast<int>(g_pCharacterInfoWindow->GetPos().x) - Width + FrameSeamOverlap)
               : 270;
}
int Y()
{
    return g_pCharacterInfoWindow ? g_pCharacterInfoWindow->GetPos().y : 0;
}
int MaxScroll()
{
    return (std::max)(0, static_cast<int>(Lines.size()) - VisibleLines);
}
int ThumbHeight()
{
    return (std::max)(24, 264 * VisibleLines / (std::max)(VisibleLines, static_cast<int>(Lines.size())));
}
int ThumbY()
{
    const int maximum = MaxScroll();
    return Y() + LineTop + (maximum ? FirstLine * (264 - ThumbHeight()) / maximum : 0);
}

void ParseDetails()
{
    Information.clear();
    Requirements.clear();
    Rewards.clear();
    Result.clear();
    int section = 0;
    for (const auto& line : Lines)
    {
        if (line.rfind(L"!Reset personalizado", 0) == 0 || line.rfind(L"!Espera", 0) == 0)
        {
            Result = line.substr(1);
            continue;
        }
        if (line == L">Requisitos")
        {
            section = 1;
            continue;
        }
        if (line == L">Después del reset")
        {
            section = 2;
            continue;
        }
        if (line == L">Recompensas")
        {
            section = 3;
            continue;
        }
        if (!line.empty() && line[0] == L'>')
        {
            section = 0;
            continue;
        }
        if (section == 1)
            Requirements.push_back(line);
        else if (section == 2)
            Information.push_back(line);
        else if (section == 3)
            Rewards.push_back(line);
        else if (line.rfind(L"No hay", 0) == 0 || line.rfind(L"!", 0) == 0)
            Result = line.c_str() + (line[0] == L'!' ? 1 : 0);
    }
}
void Request(bool reset = false)
{
    if (Pending)
        return;
    if (!SocketClient || !SocketClient->IsConnected())
    {
        Result = L"Sin conexion con el servidor.";
        return;
    }
    if (reset && (!CanReset || Token == 0))
    {
        Result = L"Revisa los requisitos de reset.";
        Request(false);
        return;
    }
    if (!Network::Paris::RequestReset(reset, Token))
        return;
    RequestedAt = timeGetTime();
    Pending = true;
    PendingReset = reset;
}

void Toggle()
{
    const bool opening = !Open;
    if (g_pNewUISystem)
        g_pNewUISystem->HideAll();
    Open = opening;
    Pending = CanReset = Dragging = false;
    FirstLine = InfoLine = RewardLine = NextReset = 0;
    Token = 0;
    Lines.clear();
    Information.clear();
    Requirements.clear();
    Rewards.clear();
    Result.clear();
    if (Open && g_pNewUISystem)
    {
        g_pNewUISystem->Show(SEASON3B::INTERFACE_CHARACTER);
        Request();
    }
}

bool Receive(const BYTE* packet, int size)
{
    if (!packet || !Open || !Pending || size < 25 || size > 16024 || packet[0] != 0xC2 ||
        ((packet[1] << 8) | packet[2]) != size || packet[3] != 0xFA || packet[4] != 0xC7 || packet[5] != 1 ||
        packet[6] > 1 || packet[7] > 1)
        return false;
    const int count = packet[20] | (packet[21] << 8);
    if (count < 1 || count > 128)
        return false;
    std::vector<std::wstring> received;
    int offset = 24;
    for (int i = 0; i < count; ++i)
    {
        int end = offset;
        while (end < size && packet[end])
            ++end;
        if (end == size || end - offset > 1024)
            return false;
        const int length = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(packet + offset), end - offset, nullptr, 0);
        if (length <= 0 || length > 512)
            return false;
        std::wstring line(length, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(packet + offset), end - offset,
                            line.data(), length);
        received.push_back(std::move(line));
        offset = end + 1;
    }
    if (offset != size)
        return false;
    int next;
    std::memcpy(&next, packet + 16, sizeof(next));
    if (next < 0)
        return false;
    unsigned long long token;
    std::memcpy(&token, packet + 8, sizeof(token));
    if (packet[6] && token == 0)
        return false;
    NextReset = next;
    Token = token;
    CanReset = packet[6] && packet[7];
    Lines = std::move(received);
    ParseDetails();
    FirstLine = (std::min)(FirstLine, (std::max)(0, static_cast<int>(Requirements.size()) - RequirementRows));
    InfoLine = (std::min)(InfoLine, (std::max)(0, static_cast<int>(Information.size()) - InfoRows));
    RewardLine = (std::min)(RewardLine, (std::max)(0, static_cast<int>(Rewards.size()) - RewardRows));
    Pending = PendingReset = false;
    return true;
}

bool Mouse()
{
    if (!Open)
        return false;
    const int x = X(), y = Y();
    if (!SEASON3B::CheckMouseIn(x, y, Width, Height))
        return false;
    if (SEASON3B::IsPress(VK_LBUTTON))
    {
        if (SEASON3B::CheckMouseIn(x + 12, y + 382, 24, 24) || SEASON3B::CheckMouseIn(x + 169, y + (Reference.Ready() ? 24 : 7), 13, 12))
            Open = false;
        else if (SEASON3B::CheckMouseIn(x + 40, y + 379, 137, 30))
            Request(true);
        else if (Information.size() > InfoRows && SEASON3B::CheckMouseIn(x + 167, y + 100, 9, 18))
            InfoLine = (std::max)(0, InfoLine - 1);
        else if (Information.size() > InfoRows && SEASON3B::CheckMouseIn(x + 167, y + 147, 9, 18))
            InfoLine = (std::min)(static_cast<int>(Information.size()) - InfoRows, InfoLine + 1);
        else if (Requirements.size() > RequirementRows && SEASON3B::CheckMouseIn(x + 167, y + 208, 9, 18))
            FirstLine = (std::max)(0, FirstLine - 1);
        else if (Requirements.size() > RequirementRows && SEASON3B::CheckMouseIn(x + 167, y + 251, 9, 18))
            FirstLine = (std::min)(static_cast<int>(Requirements.size()) - RequirementRows, FirstLine + 1);
        else if (Rewards.size() > RewardRows && SEASON3B::CheckMouseIn(x + 167, y + 310, 9, 18))
            RewardLine = (std::max)(0, RewardLine - 1);
        else if (Rewards.size() > RewardRows && SEASON3B::CheckMouseIn(x + 167, y + 350, 9, 18))
            RewardLine = (std::min)(static_cast<int>(Rewards.size()) - RewardRows, RewardLine + 1);
    }
    UI::Paris::ConsumeMouse();
    return true;
}
void Text(int x, int y, int width, const wchar_t* text, bool gold = false, bool bold = false, int align = RT3_SORT_LEFT)
{
    g_pRenderText->SetFont(bold ? g_hFontBold : g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    if (gold)
        g_pRenderText->SetTextColor(255, 221, 35, 255);
    else
        g_pRenderText->SetTextColor(245, 245, 245, 255);
    g_pRenderText->RenderText(x, y, text, width, 0, align);
}
void Section(int x, int y, int height, const wchar_t* title)
{
    constexpr DWORD DividerGold = 0xFFA88B36u;
    constexpr int DividerHeight = 1;
    RenderColorQuadARGB(x, y, 164, DividerHeight, DividerGold);
    Text(x, y + 5, 164, title, true, true, RT3_SORT_CENTER);
}
void PageArrows(int x, int y, int height, int first, int count, int rows)
{
    if (count <= rows)
        return;
    UI::Theme::Bevel(x, y, 9, 18, L"^", false, first > 0);
    UI::Theme::Bevel(x, y + height - 18, 9, 18, L"v", false, first + rows < count);
}
void Detail(int x, int y, const std::wstring& line, int status = -1)
{
    Text(x + 27, y, 10, L"•", true);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(245, 245, 245, 255);
    if (status >= 0)
        g_pRenderText->SetTextColor(status ? 100 : 255, status ? 235 : 95, 100, 255);
    g_pRenderText->RenderText(x + 38, y, line.c_str(), 127, 0, RT3_SORT_LEFT);
    UI::Theme::HoverTip(x + 38, y, 125, 12, line.c_str());
}
void DrawInformation(int x, int y)
{
    Section(x + 13, y + 69, 102, L"INFORMACION");
    PageArrows(x + 167, y + 100, 65, InfoLine, static_cast<int>(Information.size()), InfoRows);
    for (int i = 0; i < InfoRows && InfoLine + i < static_cast<int>(Information.size()); ++i)
        Detail(x, y + 99 + i * DetailStep, Information[InfoLine + i]);
    if (Information.empty())
        Text(x + 23, y + 100, 140, Pending ? L"Consultando servidor..." : L"Sin informacion disponible.");
}
void DrawRequirements(int x, int y)
{
    Section(x + 13, y + 178, 96, L"REQUISITOS");
    PageArrows(x + 167, y + 208, 61, FirstLine, static_cast<int>(Requirements.size()), RequirementRows);
    for (int i = 0; i < RequirementRows && FirstLine + i < static_cast<int>(Requirements.size()); ++i)
    {
        const auto& line = Requirements[FirstLine + i];
        const bool marked = !line.empty() && (line[0] == L'+' || line[0] == L'!');
        // Keep the current / required values sent by the server intact.
        Detail(x, y + 210 + i * DetailStep, marked ? line.substr(1) : line, marked ? (line[0] == L'+' ? 1 : 0) : -1);
    }
}
void DrawRewards(int x, int y)
{
    Section(x + 13, y + 280, 89, L"RECOMPENSA");
    PageArrows(x + 167, y + 310, 58, RewardLine, static_cast<int>(Rewards.size()), RewardRows);
    for (int i = 0; i < RewardRows && RewardLine + i < static_cast<int>(Rewards.size()); ++i)
    {
        auto line = Rewards[RewardLine + i];
        const auto colon = line.find(L':');
        if (line.find(L"puntos:") != std::wstring::npos && colon != std::wstring::npos)
            line = L"Puntos LevelUP:" + line.substr(colon + 1);
        Detail(x, y + 311 + i * DetailStep, line);
    }
}
void Render()
{
    if (!Open)
        return;
    if (!g_pNewUISystem || !g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
    {
        Open = false;
        return;
    }
    if (Pending && timeGetTime() - RequestedAt > 6000)
    {
        const bool wasReset = PendingReset;
        Pending = false;
        PendingReset = false;
        CanReset = false;
        Result = L"Sin respuesta del servidor.";
        if (wasReset)
            Request(false);
    }
    const int x = X(), y = Y();
    Reference.Draw(x, y, Width, Height);
    Text(x + 10, y + 48, Width - 20, L"RESET SYSTEM", true, true, RT3_SORT_CENTER);
    wchar_t resets[48];
    swprintf_s(resets, L"Reset: %d", (std::max)(0, NextReset - 1));
    Text(x + 15, y + 60, 160, resets, true, false, RT3_SORT_CENTER);
    DrawInformation(x, y);
    DrawRequirements(x, y);
    DrawRewards(x, y);
    if (!Result.empty())
    {
        Text(x + 15, y + 370, 160, Result.c_str(), true);
        UI::Theme::HoverTip(x + 15, y + 369, 160, 10, Result.c_str());
    }
    Art.Exit(x + 12, y + 382);
    if (Buttons.Ready())
        Buttons.Reset(x + 40, y + 379, 137, 30);
    Text(x + 40, y + 389, 137, Pending ? L"Procesando..." : L"REINICIAR AHORA", true, true, RT3_SORT_CENTER);
}
} // namespace UI::Character::ResetPanel

using namespace SEASON3B;
namespace Reset = UI::Character::ResetPanel;
CParisResetWindow::~CParisResetWindow()
{
    Release();
}
bool CParisResetWindow::Create(CNewUIManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    constexpr DWORD ResetInterface = 0x50525354;
    manager->AddUIObj(ResetInterface, this);
    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    Reset::Buttons.Load();
    Reset::Art.Load();
    Reset::Reference.Load(L"Data\\Interface\\Paris_ResetStats45.tga");
    Show(false);
    return true;
}
void CParisResetWindow::Release()
{
    if (!m_manager)
        return;
    Reset::Open = Reset::Pending = false;
    Reset::Buttons.Unload();
    Reset::Art.Unload();
    Reset::Reference.Unload();
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisResetWindow::ToggleWindow()
{
    Reset::Toggle();
    Show(Reset::Open);
}
bool CParisResetWindow::ReceivePacket(const BYTE* packet, int size)
{
    return Reset::Receive(packet, size);
}
bool CParisResetWindow::UpdateMouseEvent()
{
    const bool handled = Reset::Mouse();
    Show(Reset::Open);
    return !handled;
}
bool CParisResetWindow::UpdateKeyEvent()
{
    if (!IsVisible() || !IsPress(VK_ESCAPE))
        return true;
    Reset::Open = false;
    Show(false);
    return false;
}
bool CParisResetWindow::Update()
{
    if (!IsVisible() || !g_pNewUISystem->IsVisible(INTERFACE_CHARACTER))
        Reset::Open = false;
    Show(Reset::Open);
    return true;
}
bool CParisResetWindow::Render()
{
    Reset::Render();
    DisableAlphaBlend();
    return true;
}
float CParisResetWindow::GetLayerDepth()
{
    return 8.5f;
}
float CParisResetWindow::GetKeyEventOrder()
{
    return 2.0f;
}
