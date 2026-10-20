#include "stdafx.h"
#include "UI/NewUI/HUD/ParisHudStatus.h"
#include "UI/NewUI/Dialogs/ParisCustomMenuWindow.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/Character/NewUICharacterInfoWindow.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/HUD/CustomFeaturesWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "Audio/DSPlaySound.h"
#include <SDL3/SDL_misc.h>
using namespace SEASON3B;
namespace
{
constexpr int PanelX = 446, PanelY = 20, PanelWidth = 190, PanelHeight = 390;
constexpr int CardWidth = 76, CardHeight = 35, ColumnStep = 80, RowStep = 38;
constexpr int CardX = PanelX + 17, CardY = PanelY + 42, CardCount = 16;
constexpr int ToggleX = 27, ToggleY = 432, ToggleWidth = 50, ToggleHeight = 12;
constexpr const wchar_t* Labels[] = {L"Add Stats", L"Jewel Bank", L"Reset", L"Eventos", L"Info PJ", L"Market",
    L"Rank User", L"VIP", L"Battle Pass", L"Guias", L"Kill Log", L"Post Items", L"Mentoria", L"Panel GM", L"Soul System", L"Subastas"};
// Negative tiles select the dedicated custom-system artwork.
constexpr int IconTiles[] = {0, 11, 8, -1, -2, 4, 7, 5, 6, -3, -4, -5, -6, -7, -8, -9};
constexpr int CardActions[] = {0, 10, 7, 14, 15, 3, 6, 4, 5, 16, 17, 18, 19, 20, 21, 22};
constexpr int AdditionalActionBase = 14;
constexpr const char* GuideUrl = "https://muparis.com/guia5/index.html";
CParisCustomMenuWindow* Menu = nullptr;
void Text(int x, int y, int width, const wchar_t* text, bool gold = false, bool bold = true)
{
    g_pRenderText->SetFont(gold && bold ? g_hFontBold : g_hFont);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(gold ? 255 : 210, gold ? 215 : 210, gold ? 100 : 210, 255);
    g_pRenderText->RenderText(x, y, text, width, 0, RT3_SORT_CENTER);
}
void CardBorder(int x, int y, bool hover)
{
    UI::Theme::Bevel(x, y, CardWidth, CardHeight, L"", hover);
}
} // namespace
CParisCustomMenuWindow::~CParisCustomMenuWindow()
{
    Release();
}
bool CParisCustomMenuWindow::Create(CNewUIManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    UI::Paris::LoadGoldTheme();
    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    manager->AddUIObj(INTERFACE_PARIS_CUSTOM_MENU, this);
    m_icons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_CustomIconsV2.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_guidesIcon = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MenuGuides61.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_infoIcon = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MenuInfo61.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_extraIcons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MenuExtraV36.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_featureIcons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MenuFeatures50.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_eventsIcon = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MenuEvents61.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_toggle = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MenuToggle63.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    if (!m_questWindow.Create(manager, 416, 65) || !m_jewelBankWindow.Create(manager) || !m_vipWindow.Create(manager) ||
        !m_infoWindow.Create(manager) || !m_resetWindow.Create(manager) || !m_allocationWindow.Create(manager) ||
        !m_eventsWindow.Create(manager) || !m_rankingWindow.Create(manager, g_pNewUI3DRenderMng) ||
        !m_marketWindow.Create(manager) || !m_battlePassWindow.Create(manager))
    {
        Release();
        return false;
    }
    m_featuresWindow = std::make_unique<CParisFeatureWindow>();
    if (!m_featuresWindow->Create(manager, g_pNewUI3DRenderMng))
    {
        Release();
        return false;
    }
    Menu = this;
    Show(false);
    return true;
}
void CParisCustomMenuWindow::Release()
{
    if (!m_manager)
        return;
    m_featuresWindow.reset();
    m_battlePassWindow.Release();
    m_marketWindow.Release();
    m_rankingWindow.Release();
    m_eventsWindow.Release();
    m_allocationWindow.Release();
    m_resetWindow.Release();
    m_infoWindow.Release();
    m_vipWindow.Release();
    m_jewelBankWindow.Release();
    m_questWindow.Release();
    if (m_icons != BITMAP_UNKNOWN)
        DeleteBitmap(m_icons);
    if (m_toggle != BITMAP_UNKNOWN)
        DeleteBitmap(m_toggle);
    if (m_guidesIcon != BITMAP_UNKNOWN) DeleteBitmap(m_guidesIcon);
    if (m_eventsIcon != BITMAP_UNKNOWN) DeleteBitmap(m_eventsIcon);
    for (auto image : {m_infoIcon, m_extraIcons, m_featureIcons}) if (image != BITMAP_UNKNOWN) DeleteBitmap(image);
    m_infoIcon = m_extraIcons = m_featureIcons = BITMAP_UNKNOWN;
    m_guidesIcon = m_eventsIcon = BITMAP_UNKNOWN;
    m_icons = m_toggle = BITMAP_UNKNOWN;
    UI::Paris::UnloadGoldTheme();
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
    if (Menu == this)
        Menu = nullptr;
}
void CParisCustomMenuWindow::TogglePanel()
{
    if (Menu)
        Menu->Show(!Menu->CNewUIObj::IsVisible());
}
void CParisCustomMenuWindow::CloseAllOverlays(CNewUIObj* keep)
{
    if (Menu) Menu->CloseOverlays(keep);
}
void CParisCustomMenuWindow::CloseOverlays(CNewUIObj* keep)
{
    if (m_featuresWindow && keep != m_featuresWindow.get()) m_featuresWindow->Close();
    if (keep != &m_rankingWindow) m_rankingWindow.CloseWindow(false);
    if (keep != &m_marketWindow) m_marketWindow.CloseWindow(false);
    if (keep != &m_battlePassWindow) m_battlePassWindow.CloseWindow();
    if (keep != &m_questWindow) m_questWindow.CloseWindow();
    CNewUIObj* panels[] = {&m_jewelBankWindow,  &m_vipWindow,
                           &m_resetWindow, &m_allocationWindow, &m_eventsWindow};
    for (auto* panel : panels)
        if (panel != keep)
            panel->Show(false);
    if (keep != &m_infoWindow)
        m_infoWindow.CloseWindow(false);
}
void CParisCustomMenuWindow::Activate(int index)
{
    if (index < 0 || index >= CardCount) return;
    index = CardActions[index];
    if (index >= AdditionalActionBase) { ActivateAdditional(index - AdditionalActionBase); return; }
    CNewUIObj* keep = index == 4    ? static_cast<CNewUIObj*>(&m_vipWindow)
                      : index == 7  ? static_cast<CNewUIObj*>(&m_resetWindow)
                      : index == 10 ? static_cast<CNewUIObj*>(&m_jewelBankWindow)
                      : index == 9 ? static_cast<CNewUIObj*>(&m_questWindow)
                                    : index == 0 ? static_cast<CNewUIObj*>(&m_allocationWindow)
                      : index == 8 ? static_cast<CNewUIObj*>(&m_eventsWindow)
                                    : nullptr;
    CloseOverlays(keep);
    Show(false);
    switch (index)
    {
    case 0:
        m_allocationWindow.ToggleWindow();
        break;
    case 1:
        SDL_OpenURL(GuideUrl);
        break;
    case 2:
        g_pNewUISystem->Show(INTERFACE_INVENTORY);
        break;
    case 3:
        m_marketWindow.OpenWindow();
        break;
    case 5:
        m_battlePassWindow.ToggleWindow();
        break;
    case 6:
        m_rankingWindow.OpenWindow();
        break;
    case 4:
        m_vipWindow.ToggleWindow();
        break;
    case 7:
        m_resetWindow.ToggleWindow();
        break;
    case 9:
        m_questWindow.OpenWindow();
        break;
    case 10:
        m_vipWindow.Show(false);
        m_resetWindow.Show(false);
        m_infoWindow.CloseWindow(false);
        g_pNewUISystem->Hide(INTERFACE_CHARACTER);
        g_pNewUISystem->Show(INTERFACE_INVENTORY);
        m_jewelBankWindow.Show(!m_jewelBankWindow.IsVisible());
        break;
    case 8:
        m_eventsWindow.ToggleWindow();
        break;
    }
}
void CParisCustomMenuWindow::ActivateAdditional(int index)
{
    Show(false);
    if (index == 0)
    {
        CloseOverlays(&m_eventsWindow);
        m_eventsWindow.ToggleWindow();
    }
    else if (index == 1)
    {
        CloseOverlays(&m_infoWindow);
        g_pNewUISystem->Show(INTERFACE_CHARACTER);
        m_infoWindow.OpenWindow();
    }
    else if (index == 2) SDL_OpenURL(GuideUrl);
    else if (m_featuresWindow)
    {
        CloseOverlays(m_featuresWindow.get());
        m_featuresWindow->Open(10 + index - 3);
    }
}
void CParisCustomMenuWindow::RenderAdditionalIcon(int index, int x, int y) const
{
    constexpr int IconSize = 19;
    if (index < 3)
    {
        const GLuint images[]{m_eventsIcon, m_infoIcon, m_guidesIcon};
        if (images[index] != BITMAP_UNKNOWN) RenderBitmap(images[index], x, y, IconSize, IconSize, .20f, .12f, .60f, .51f);
        return;
    }
    if (index < 7)
    {
        constexpr float Cell = 627.f / 2048.f;
        const int tile = index - 3;
        if (m_extraIcons != BITMAP_UNKNOWN)
            RenderBitmap(m_extraIcons, x, y, IconSize, IconSize, (tile % 2 + .20f) * Cell, (tile / 2 + .12f) * Cell, .60f * Cell, .49f * Cell);
        return;
    }
    if (m_featureIcons != BITMAP_UNKNOWN)
        RenderBitmap(m_featureIcons, x, y, IconSize, IconSize, (index == 7 ? 0.f : .5f) + .10f, .10f, .22f, .47f);
}
bool CParisCustomMenuWindow::UpdateMouseEvent()
{
    const bool toggle = UI::Hud::Status::MenuHit();
    const bool panel = CNewUIObj::IsVisible() && CheckMouseIn(PanelX, PanelY, PanelWidth, PanelHeight);
    if (!toggle && !panel)
        return true;
    if (IsPress(VK_LBUTTON))
    {
        if (toggle)
            TogglePanel();
        else if (CheckMouseIn(PanelX + 169, PanelY + 7, 13, 12))
            Show(false);
        else if (CheckMouseIn(PanelX + 10, PanelY + 360, 58, 20))
        {
            Show(false);
            CloseOverlays(&m_allocationWindow);
            m_allocationWindow.ToggleWindow();
        }
        else if (CheckMouseIn(PanelX + 68, PanelY + 360, 55, 20))
        {
            Show(false);
            CloseOverlays(&m_eventsWindow);
            m_eventsWindow.ToggleWindow();
        }
        else if (CheckMouseIn(PanelX + 123, PanelY + 360, 57, 20))
            SDL_OpenURL(GuideUrl);
        else
            for (int index = 0; index < CardCount; ++index)
                if (CheckMouseIn(CardX + index % 2 * ColumnStep, CardY + index / 2 * RowStep, CardWidth, CardHeight))
                {
                    Activate(index);
                    break;
                }
        PlayBuffer(SOUND_CLICK01);
    }
    UI::Paris::ConsumeMouse();
    return false;
}
bool CParisCustomMenuWindow::UpdateKeyEvent()
{
    if (SceneFlag != MAIN_SCENE || CUITextInputBox::IsAnyInputBoxFocused())
        return true;
    if (IsPress('H'))
    {
        CloseOverlays(&m_eventsWindow);
        m_eventsWindow.ToggleWindow();
        return false;
    }
    if (IsPress('L'))
    {
        CloseOverlays(&m_allocationWindow);
        m_allocationWindow.ToggleWindow();
        return false;
    }
    if (IsPress(VK_F1))
    {
        SDL_OpenURL(GuideUrl);
        return false;
    }
    if (IsPress('I') && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && g_pNewUISystem->IsVisible(INTERFACE_CHARACTER))
    {
        CloseOverlays(&m_infoWindow);
        if (m_infoWindow.IsWindowOpen())
            m_infoWindow.CloseWindow();
        else
            m_infoWindow.OpenWindow();
        return false;
    }
    if (!CNewUIObj::IsVisible() || !IsPress(VK_ESCAPE))
        return true;
    Show(false);
    return false;
}
bool CParisCustomMenuWindow::Update()
{
    return true;
}
void CParisCustomMenuWindow::RenderCard(int index) const
{
    const int slot = index;
    const int x = CardX + slot % 2 * ColumnStep, y = CardY + slot / 2 * RowStep;
    CardBorder(x, y, CheckMouseIn(x, y, CardWidth, CardHeight));
    constexpr int IconSize = 19, IconTop = 2, LabelTop = 23, LabelInset = 4;
    UI::Paris::Color(1, 1, 1, 1);
    const int tile = IconTiles[index];
    if (tile < 0) RenderAdditionalIcon(-tile - 1, x + (CardWidth - IconSize) / 2, y + IconTop);
    else if (m_icons != BITMAP_UNKNOWN)
        RenderBitmap(m_icons, x + (CardWidth - IconSize) / 2, y + IconTop, IconSize, IconSize, (tile % 2) * .5f,
            (tile / 2) / 8.f, .5f, 1.f / 8.f);
    Text(x + LabelInset, y + LabelTop, CardWidth - LabelInset * 2, Labels[index], true, false);
}
bool CParisCustomMenuWindow::Render()
{
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    UI::Hud::Status::RenderMenu();
    if (CNewUIObj::IsVisible())
    {
        UI::Paris::Frame(PanelX, PanelY, PanelWidth, PanelHeight);
        Text(PanelX, PanelY + 20, PanelWidth, L"CUSTOM", true);
        Text(PanelX, PanelY + 31, PanelWidth, L"SISTEMAS PARIS");
        for (int index = 0; index < CardCount; ++index)
            RenderCard(index);
        Text(PanelX, PanelY + 347, PanelWidth, L"Selecciona un sistema");
        Text(PanelX, PanelY + 364, PanelWidth, L"L: Add Stats   H: Eventos   F1: Guias");
        UI::Theme::HoverTip(CardX, CardY, CardWidth, CardHeight, L"Stats / L: Add Stats / Ctrl+I: Info Character");
    }
    DisableAlphaBlend();
    return true;
}
bool CParisCustomMenuWindow::IsVisible() const
{
    return true;
}
float CParisCustomMenuWindow::GetLayerDepth()
{
    return 10.5f;
}
float CParisCustomMenuWindow::GetKeyEventOrder()
{
    return 2.0f;
}
