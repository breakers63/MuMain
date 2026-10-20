#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/HUD/ParisOverlayInput.h"
#include "GmEventsPanel.h"
#include "Network/Paris/GmEventsProtocol.h"
#include "Network/Paris/EventDeviceProtocol.h"
#include "UI/NewUI/Character/PanelAspectScale.h"
#include "UI/NewUI/Widgets/ParisTintOverlay.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Legacy/UIControls.h"
#include "Render/Textures/ZzzTexture.h"
#include "World/MapInfra/MapManager.h"
#include <memory>
#include <cwctype>

namespace UI::GmEvents
{
namespace
{
constexpr int DesignWidth = 1536, DesignHeight = 1024, RowTop = 282, RowHeight = 55;
constexpr DWORD RefreshInterval = 1000, StaleInterval = 5000;
GLuint texture = BITMAP_UNKNOWN;
std::unique_ptr<CUITextInputBox> search, announcement;
HFONT normal = nullptr, bold = nullptr;
bool requested = false, visible = false;
int selected = 1, fontSize = 0, ox = 0, oy = 0;
float scale = 1;
DWORD lastRequest = 0;
std::wstring result;
const wchar_t *PendingNames[] = {L"Nemesis Semanal", L"Tiempo Frances", L"TvT Event"};
wchar_t permissionCharacter[11]{};
void ProbePermissions()
{
    if (SceneFlag != MAIN_SCENE || !Hero || !Hero->ID[0] || !SocketClient || !SocketClient->IsConnected()) return;
    if (std::wcscmp(permissionCharacter, Hero->ID) == 0) return;
    wcscpy_s(permissionCharacter, Hero->ID);
    Network::Paris::GmEvents::Authorized = false;
    Network::Paris::GmEvents::Received = 0;
    Network::Paris::GmEvents::Request(0, 0);
}
bool IsGm() { return Hero && (Hero->CtlCode == CTLCODE_08OPERATOR || Hero->CtlCode == CTLCODE_20OPERATOR); }
int W(int v) { return UI::Character::PanelAspect::Axis(v, scale, g_fScreenRate_x); }
int H(int v) { return UI::Character::PanelAspect::Axis(v, scale, g_fScreenRate_y); }
int X(int v) { return ox + W(v); }
int Y(int v) { return oy + H(v); }
bool Hit(int x, int y, int w, int h) { return SEASON3B::CheckMouseIn(X(x), Y(y), W(w), H(h)); }
void Text(int x, int y, int width, const wchar_t *text, bool gold = false, bool strong = false)
{
    EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
    g_pRenderText->SetFont(strong ? bold : normal); g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(gold ? 240 : 255, gold ? 211 : 255, gold ? 138 : 255, 255);
    g_pRenderText->RenderText(X(x), Y(y), text, W(width), 0, RT3_SORT_LEFT);
}
void Glow(int x, int y, int w, int h)
{
    UI::Theme::TintOverlay(X(x), Y(y), W(w), H(h), 1.f, .8f, .25f, .14f);
}
bool MatchesSearch(int index)
{
    wchar_t query[41]{}; search->GetText(query, 41);
    std::wstring filter(query);
    std::wstring name = index < 7 ? Network::Paris::GmEvents::Events[index].Name : PendingNames[index - 7];
    for (auto &character : filter) character = static_cast<wchar_t>(std::towlower(character));
    for (auto &character : name) character = static_cast<wchar_t>(std::towlower(character));
    return filter.empty() || name.find(filter) != std::wstring::npos;
}
void Layout()
{
    scale = UI::Character::PanelAspect::Fit(WindowWidth, WindowHeight * .94f, DesignWidth, DesignHeight);
    ox = static_cast<int>((WindowWidth - DesignWidth * scale) / (2 * g_fScreenRate_x)); oy = 0;
    int pixels = std::max(11, static_cast<int>(22 * scale));
    if (pixels != fontSize)
    {
        g_pRenderText->SetFont(g_hFont);
        if (normal) DeleteObject(normal); if (bold) DeleteObject(bold);
        normal = CreateFontW(pixels, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
        bold = CreateFontW(pixels, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
        fontSize = pixels; search->SetFont(normal); announcement->SetFont(normal);
    }
    search->SetPosition(X(250), Y(245)); search->SetSize(W(390), H(25));
    announcement->SetPosition(X(771), Y(711)); announcement->SetSize(W(358), H(27));
}
void Send(BYTE action)
{
    if ((selected >= Network::Paris::GmEvents::Count && action != 4) || !Network::Paris::GmEvents::Authorized) return;
    wchar_t message[121]{}; announcement->GetText(message, 121);
    Network::Paris::GmEvents::Request(action, static_cast<BYTE>(selected < 7 ? selected : 0), message);
    lastRequest = timeGetTime(); result = action == 4 ? L"Enviando anuncio..." : L"Solicitud enviada...";
}
void Details()
{
    if (selected >= Network::Paris::GmEvents::Count)
    {
        Text(864, 213, 440, PendingNames[selected - 7], true, true);
        Text(923, 259, 250, L"PENDIENTE DE CONECTAR", true);
        Text(790, 802, 490, L"Se requiere el codigo del evento.");
        return;
    }
    const auto &event = Network::Paris::GmEvents::Events[selected];
    const wchar_t *phases[] = {L"INACTIVO", L"INSCRIPCIONES ABIERTAS", L"PREPARACION", L"EN CURSO"};
    Text(865, 214, 440, event.Name.c_str(), true, true); Text(925, 259, 258, phases[event.Phase], true);
    wchar_t value[128]{};
    swprintf_s(value, L"%d / %d", event.Participants, event.Maximum); Text(891, 337, 87, value, true, true);
    int seconds = std::max(0, event.Seconds - static_cast<int>((timeGetTime() - Network::Paris::GmEvents::Received) / 1000));
    swprintf_s(value, L"%02d:%02d", seconds / 60, seconds % 60); Text(1030, 337, 100, value, true, true);
    swprintf_s(value, L"%02d:%02d", event.Duration / 60, event.Duration % 60); Text(1166, 337, 125, value, true, true);
    swprintf_s(value, L"Mapa: %s", gMapManager.GetMapName(event.Map)); Text(770, 394, 215, value);
    Text(1055, 394, 242, event.Requirement.c_str());
    Text(790, 802, 500, result.empty() ? L"Estado recibido del servidor" : result.c_str());
    swprintf_s(value, L"%d participantes registrados", event.Participants); Text(790, 832, 490, value);
}
} // namespace
void Load()
{
    texture = Bitmaps.LoadImage(L"Data\\Interface\\Paris_GmPanelV31.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    search = std::make_unique<CUITextInputBox>(); announcement = std::make_unique<CUITextInputBox>();
    search->Init(g_hWnd, 180, 18, 40); announcement->Init(g_hWnd, 220, 18, 120);
    for (auto *input : {search.get(), announcement.get()})
    {
        input->SetTextColor(255, 255, 255, 255); input->SetBackColor(0, 0, 0, 0); input->SetState(UISTATE_HIDE);
    }
    Layout();
}
void Close()
{
    visible = requested = false;
    if (search) search->SetState(UISTATE_HIDE); if (announcement) announcement->SetState(UISTATE_HIDE);
}
void Unload()
{
    Close(); search.reset(); announcement.reset();
    g_pRenderText->SetFont(g_hFont); if (normal) DeleteObject(normal); if (bold) DeleteObject(bold);
    normal = bold = nullptr; fontSize = 0;
    if (texture != BITMAP_UNKNOWN) DeleteBitmap(texture); texture = BITMAP_UNKNOWN;
    Network::Paris::GmEvents::Authorized = false;
    permissionCharacter[0] = 0;
}
void Toggle()
{
    if (visible || requested) { Close(); return; }
    if (!search) return;
    g_pNewUISystem->HideAll();
    Network::Paris::GmEvents::Authorized = false; requested = true; result.clear();
    Network::Paris::GmEvents::Request(0, 0); lastRequest = timeGetTime();
}
bool IsOpen() { return visible || requested; }
bool CanOpen() { return IsGm() || Network::Paris::GmEvents::Authorized; }
bool HasInputFocus() { return visible && (search->HaveFocus() || announcement->HaveFocus()); }
void Render()
{
    Network::Paris::EventDevice::Probe();
    ProbePermissions();
    if (!IsGm() && !requested && !Network::Paris::GmEvents::Authorized) { Close(); return; }
    if (requested && Network::Paris::GmEvents::Authorized)
    {
        visible = true; requested = false; search->SetState(UISTATE_NORMAL); announcement->SetState(UISTATE_NORMAL);
    }
    if (requested && timeGetTime() - lastRequest > StaleInterval) { Close(); return; }
    if (!visible) return;
    Layout();
    if (timeGetTime() - lastRequest >= RefreshInterval) { Network::Paris::GmEvents::Request(0, 0); lastRequest = timeGetTime(); }
    if (timeGetTime() - Network::Paris::GmEvents::Received >= StaleInterval) result = L"Sin respuesta del servidor";
    if (Network::Paris::GmEvents::Result) { result = Network::Paris::GmEvents::Result == 2 ? L"Anuncio enviado" : Network::Paris::GmEvents::Result == 3 ? L"Anuncio no valido" : L"Solicitud procesada; comprueba la fase"; Network::Paris::GmEvents::Result = 0; }
    EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
    RenderBitmap(texture, X(0), Y(0), W(DesignWidth), H(DesignHeight), 0, 0, .75f, 1.f);
    wchar_t query[41]{}; search->GetText(query, 41);
    for (int i = 0; i < 10; ++i)
    {
        std::wstring name = i < 7 ? Network::Paris::GmEvents::Events[i].Name : PendingNames[i - 7];
        if (!MatchesSearch(i)) continue;
        if (i == selected || Hit(238, RowTop + i * RowHeight, 415, 49)) Glow(238, RowTop + i * RowHeight, 415, 49);
        Text(310, RowTop + i * RowHeight + 14, 322, name.c_str(), i == selected, i == selected);
    }
    for (int y : {471, 533, 597}) if (Hit(795, y, 414, 54) && selected < 7) Glow(795, y, 414, 54);
    if (selected >= 7)
        for (int y : {471, 533, 597}) UI::Theme::TintOverlay(X(795), Y(y), W(414), H(54), 0, 0, 0, .6f);
    if (Hit(1155, 700, 146, 49)) Glow(1155, 700, 146, 49);
    Details(); search->Render(); announcement->Render();
    if (!*query && !search->HaveFocus()) Text(250, 245, 380, L"Buscar evento...");
    g_pRenderText->SetFont(g_hFont);
}
bool Mouse()
{
    if (!visible && !requested) return false;
    UI::OverlayInput::Scope mouseOwner;
    if (!visible) return true;
    Layout(); search->DoAction(); announcement->DoAction();
    if (SEASON3B::IsPress(VK_LBUTTON))
    {
        if (Hit(1274, 79, 40, 44) || Hit(1155, 872, 148, 41)) { Close(); return true; }
        if (Hit(241, 236, 411, 39)) search->GiveFocus();
        if (Hit(765, 699, 376, 48)) announcement->GiveFocus();
        for (int i = 0; i < 10; ++i) if (MatchesSearch(i) && Hit(238, RowTop + i * RowHeight, 415, 49)) { selected = i; result.clear(); }
        if (selected < 7 && timeGetTime() - Network::Paris::GmEvents::Received < StaleInterval)
        {
            const auto &event = Network::Paris::GmEvents::Events[selected];
            if (event.Enabled && event.Phase == 0 && Hit(795, 471, 414, 54)) Send(1);
            if (event.Enabled && event.Phase == 1 && Hit(795, 533, 414, 54)) Send(2);
            if (event.Phase != 0 && Hit(795, 597, 414, 54)) Send(3);
        }
        if (Hit(1155, 700, 146, 49)) Send(4);
    }
    return true;
}
} // namespace UI::GmEvents
