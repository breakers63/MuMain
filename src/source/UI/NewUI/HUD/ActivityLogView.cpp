#include "stdafx.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Character/RankingBorder.h"
#include "ActivityLogView.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "UI/NewUI/NewUISystem.h"
#include "GameLogic/Social/ActivityHistory.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/Character/PanelAspectScale.h"
#include "UI/NewUI/Widgets/NewUIChatInputBox.h"
#include "Engine/Object/ZzzInventory.h"
#include "World/MapInfra/MapManager.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/Widgets/ParisTintOverlay.h"
#include <cwctype>
#include <array>
namespace UI::ActivityLogs
{
namespace
{
constexpr int Width = 540, Height = 460, Rows = 4, RowHeight = 55;
constexpr int ScrollX = 508, ScrollTop = 164, ScrollHeight = 198;
int originX = 0, originY = 0;
float scale = 1;
ActivityLogView view;
int W(int v)
{
    return UI::Character::PanelAspect::Axis(v, scale, g_fScreenRate_x);
}
int H(int v)
{
    return UI::Character::PanelAspect::Axis(v, scale, g_fScreenRate_y);
}
int X(int v)
{
    return originX + W(v);
}
int Y(int v)
{
    return originY + H(v);
}
bool Hit(int x, int y, int w, int h)
{
    return SEASON3B::CheckMouseIn(X(x), Y(y), W(w), H(h));
}
auto &History(bool kills)
{
    return kills ? GameLogic::Social::ActivityHistory::Kills : GameLogic::Social::ActivityHistory::Posts;
}
std::wstring Lower(std::wstring text)
{
    for (auto &c : text)
        c = static_cast<wchar_t>(std::towlower(c));
    return text;
}
void Sprite(GLuint texture, int x, int y, int w, int h, float u = 0, float v = 0, float uw = 1, float vh = 1)
{
    if (texture == BITMAP_UNKNOWN)
        return;
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    RenderBitmap(texture, X(x), Y(y), W(w), H(h), u, v, uw, vh);
}
void Glow(int x, int y, int w, int h)
{
    UI::Theme::TintOverlay(X(x), Y(y), W(w), H(h), 1.f, .8f, .3f, .10f);
}
} // namespace
ActivityLogView &ActivityLog()
{
    return view;
}
void ActivityLogView::Load()
{
    m_Buttons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingColorsV14.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Background = Bitmaps.LoadImage(L"Data\\Interface\\Paris_KillLogV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_PostBackground = Bitmaps.LoadImage(L"Data\\Interface\\Paris_PostLogV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Frame = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingReferenceV5.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Widgets = Bitmaps.LoadImage(L"Data\\Interface\\Paris_KillAtlasV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Winner = Bitmaps.LoadImage(L"Data\\Interface\\Paris_WinnerBadgeV23.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Cutouts = Bitmaps.LoadImage(L"Data\\Interface\\Paris_LogCutoutsV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_PostAtlas = Bitmaps.LoadImage(L"Data\\Interface\\Paris_PostAtlasV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Search = new CUITextInputBox;
    m_Search->Init(g_hWnd, 180, 16, 40);
    m_Search->SetTextColor(255, 255, 255, 255);
    m_Search->SetBackColor(0, 0, 0, 0);
    m_Search->SetState(UISTATE_HIDE);
    UpdateLayout();
}
void ActivityLogView::Unload()
{
    ClearTooltip();
    delete m_Search;
    m_Search = nullptr;
    g_pRenderText->SetFont(g_hFont);
    for (auto font : {m_Font, m_Bold})
        if (font)
            DeleteObject(font);
    m_Font = m_Bold = nullptr;
    m_FontSize = 0;
    for (auto *texture :
         {&m_Background, &m_PostBackground, &m_Frame, &m_Widgets, &m_PostAtlas, &m_Cutouts, &m_Winner, &m_Buttons})
    {
        if (*texture != BITMAP_UNKNOWN)
            DeleteBitmap(*texture);
        *texture = BITMAP_UNKNOWN;
    }
    m_Visible.clear();
}
void ActivityLogView::UpdateLayout()
{
    scale = UI::Character::PanelAspect::Fit(WindowWidth, WindowHeight * .88f, Width, Height);
    originX = static_cast<int>((WindowWidth - Width * scale) / (2 * g_fScreenRate_x));
    originY = H(8);
    const int pixels = std::max(11, static_cast<int>(12 * scale));
    if (pixels != m_FontSize)
    {
        g_pRenderText->SetFont(g_hFont);
        for (auto font : {m_Font, m_Bold})
            if (font)
                DeleteObject(font);
        m_FontSize = pixels;
        auto make = [pixels](int weight) {
            return CreateFontW(pixels, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
        };
        m_Font = make(FW_NORMAL);
        m_Bold = make(FW_BOLD);
        m_Search->SetFont(m_Font);
    }
    m_Search->SetPosition(X(228), Y(94));
    m_Search->SetSize(W(163), H(18));
}
void ActivityLogView::SetOpen(bool open, bool kills)
{
    ClearTooltip();
    if (!m_Search)
        return;
    m_Search->SetState(open ? UISTATE_NORMAL : UISTATE_HIDE);
    if (open)
    {
        m_Kills = kills;
        m_First = 0;
        m_All = true;
        m_Query.clear();
        m_Search->SetText(L"");
        m_Revision = ~0ull;
        m_Dragging = false;
        UpdateLayout();
    }
}
void ActivityLogView::DrawText(int x, int y, int width, const wchar_t *text, bool gold, bool bold)
{
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    g_pRenderText->SetFont(bold ? m_Bold : m_Font);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(gold ? 238 : 255, gold ? 211 : 255, gold ? 139 : 255, 255);
    g_pRenderText->RenderText(X(x), Y(y), text, W(width), 0, RT3_SORT_LEFT);
}
void ActivityLogView::UpdateFilter()
{
    wchar_t raw[41]{};
    m_Search->GetText(raw, 41);
    const auto &history = History(m_Kills);
    if (m_Query == raw && m_Revision == history.Revision())
        return;
    m_Query = raw;
    m_Revision = history.Revision();
    m_Visible.clear();
    const auto query = Lower(m_Query);
    for (int i = 0; i < history.Count(); ++i)
    {
        const auto *r = history.At(i);
        if (m_Kills && !m_All && r->Winner)
            continue;
        const auto text = Lower(r->Actor + L" " + r->Description + L" " +
                                (m_Kills && r->Map >= 0 ? gMapManager.GetMapName(r->Map) : L""));
        if (text.find(query) != std::wstring::npos)
            m_Visible.push_back(i);
    }
    m_First = std::clamp(m_First, 0, std::max(0, static_cast<int>(m_Visible.size()) - Rows));
}
void ActivityLogView::DrawAtlas(int x, int y, int w, int h, int sx, int sy, int sw, int sh)
{
    const float aw = m_Kills ? 1360.f : 1349.f, ah = m_Kills ? 1157.f : 1166.f;
    Sprite(m_Kills ? m_Widgets : m_PostAtlas, x, y, w, h, sx / aw, sy / ah, sw / aw, sh / ah);
}
void ActivityLogView::DrawRow(int row, int index)
{
    const auto &r = *History(m_Kills).At(index);
    const int y = (m_Kills ? 151 : 147) + row * RowHeight;
    if (Hit(19, y, 484, RowHeight))
        Glow(19, y, 484, RowHeight);
    DrawText(34, y + 20, 82, r.Time.c_str(), true);
    if (m_Kills)
    {
        DrawText(125, y + 20, 85, r.Actor.c_str(), row == 0);
        if (r.Winner)
            Sprite(m_Winner, 232, y + 15, 123, 27, 12.f / 2167.f, 133.f / 725.f, 2142.f / 2167.f, 432.f / 725.f);
        else
            Sprite(m_Cutouts, 232, y + 15, 123, 27, 61.f / 1254.f, 138.f / 1254.f, 1132.f / 1254.f, 216.f / 1254.f);
        const int map = r.Map == 2 ? 1 : r.Map == 3 ? 2 : r.Map == 1 ? 3 : 0;
        Sprite(m_Cutouts, 378, y + 5, 37, 41, map * 313.5f / 1254.f, 680.f / 1254.f, 313.5f / 1254.f, 370.f / 1254.f);
        DrawText(422, y + 20, 80, r.Map >= 0 ? gMapManager.GetMapName(r.Map) : L"Desconocido", row == 0);
    }
    else
    {
        Sprite(m_Cutouts, 109, y + 16, 70, 24, 49.f / 1254.f, 465.f / 1254.f, 611.f / 1254.f, 172.f / 1254.f);
        UI::Character::ParisStyle::Solid(X(199), Y(y + 4), W(40), H(44), .04f, .035f, .025f);
        // Copy the original miniature border without copying its example item.
        DrawAtlas(199, y + 4, 40, 1, 498, 384, 100, 3);
        DrawAtlas(199, y + 47, 40, 1, 498, 489, 100, 3);
        DrawAtlas(199, y + 4, 1, 44, 498, 384, 3, 108);
        DrawAtlas(238, y + 4, 1, 44, 595, 384, 3, 108);
        DrawText(259, y + 19, 241, r.Description.c_str(), false, true);
    }
}
void ActivityLogView::DrawScroll()
{
    const int count = static_cast<int>(m_Visible.size()), maximum = std::max(0, count - Rows);
    const int thumb = std::max(22, ScrollHeight * Rows / std::max(Rows, count));
    const int y = ScrollTop + (maximum ? m_First * (ScrollHeight - thumb) / maximum : 0);
    UI::Character::ParisStyle::Solid(X(ScrollX + 1), Y(y), W(8), H(thumb), .4f, .27f, .045f);
    UI::Character::ParisStyle::Solid(X(ScrollX + 1), Y(y), W(8), H(1), .96f, .78f, .3f);
    UI::Character::ParisStyle::Solid(X(ScrollX + 1), Y(y + thumb - 1), W(8), H(1), .96f, .78f, .3f);
}
void ActivityLogView::DrawButton(int x, int width, const wchar_t *caption, bool green)
{
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    RenderBitmap(m_Buttons, X(x), Y(405), W(width), H(28), 0, (green ? 128.f : 0.f) / 256.f, 200.f / 256.f,
                 56.f / 256.f);
    g_pRenderText->SetFont(m_Bold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(238, 211, 139, 255);
    g_pRenderText->RenderText(X(x), Y(414), caption, W(width), 0, RT3_SORT_CENTER);
}
void ActivityLogView::Render()
{
    UpdateLayout();
    UpdateFilter();
    Sprite(m_Kills ? m_Background : m_PostBackground, 0, 0, Width, Height);
    // The exterior frame is the exact editable frame used by Stats.
    for (auto rect : {std::array<int, 4>{0, 0, Width, 11},
                      {0, Height - 11, Width, 11},
                      {0, 0, 12, Height},
                      {Width - 12, 0, 12, Height}})
        UI::Character::ParisStyle::Solid(X(rect[0]), Y(rect[1]), W(rect[2]), H(rect[3]), .03f, .035f, .04f);
    for (auto rect : {std::array<int, 4>{0, 0, 28, 28},
                      {Width - 28, 0, 28, 28},
                      {0, Height - 28, 28, 28},
                      {Width - 28, Height - 28, 28, 28}})
        UI::Character::ParisStyle::Solid(X(rect[0]), Y(rect[1]), W(rect[2]), H(rect[3]), .03f, .035f, .04f);
    UI::Character::RenderRankingBorder(m_Frame, X(0), Y(0), W(Width), H(Height));
    UI::Character::ParisStyle::Solid(X(25), Y(399), W(180), H(41), .025f, .03f, .035f);
    UI::Character::ParisStyle::Solid(X(404), Y(399), W(110), H(41), .025f, .03f, .035f);
    DrawButton(30, 72, L"‹ ANTERIOR");
    DrawButton(112, 72, L"SIGUIENTE ›", true);
    DrawButton(410, 100, L"CERRAR");
    for (auto rect : {std::array<int, 4>{25, 88, 76, 24},
                      {107, 88, 84, 24},
                      {30, 405, 72, 28},
                      {112, 405, 72, 28},
                      {410, 404, 100, 32}})
        if (Hit(rect[0], rect[1], rect[2], rect[3]))
            Glow(rect[0], rect[1], rect[2], rect[3]);
    if (!m_All)
        Glow(107, 88, 84, 24);
    m_Search->Render();
    if (m_Query.empty() && !m_Search->HaveFocus())
        DrawText(228, 94, 164, m_Kills ? L"Buscar personaje o mapa..." : L"Buscar objeto...");
    wchar_t text[80]{};
    swprintf_s(text, L"%zu REGISTROS", m_Visible.size());
    DrawText(439, 94, 80, text, true);
    for (int row = 0; row < Rows && m_First + row < static_cast<int>(m_Visible.size()); ++row)
        DrawRow(row, m_Visible[m_First + row]);
    if (m_Visible.empty())
        DrawText(180, 250, 275, L"Sin registros en esta sesion.");
    DrawScroll();
    swprintf_s(text, L"%d - %d de %zu registros", m_Visible.empty() ? 0 : m_First + 1,
               std::min(m_First + Rows, static_cast<int>(m_Visible.size())), m_Visible.size());
    DrawText(220, 416, 187, text, true);
    g_pRenderText->SetFont(g_hFont);
}
bool ActivityLogView::Mouse()
{
    UpdateLayout();
    m_Search->DoAction();
    UpdateFilter();
    const int maximum = std::max(0, static_cast<int>(m_Visible.size()) - Rows);
    if (MouseWheel && Hit(0, 0, Width, Height))
    {
        m_First = std::clamp(m_First + (MouseWheel > 0 ? -1 : 1), 0, maximum);
        MouseWheel = 0;
    }
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        m_Dragging = false;
    if (SEASON3B::IsPress(VK_LBUTTON))
    {
        if (Hit(410, 404, 100, 32))
        {
            SetOpen(false);
            return true;
        }
        if (Hit(30, 405, 72, 28))
            m_First = std::max(0, m_First - Rows);
        else if (Hit(112, 405, 72, 28))
            m_First = std::min(maximum, m_First + Rows);
        else if (Hit(25, 88, 76, 24))
        {
            m_All = true;
            m_Revision = ~0ull;
        }
        else if (Hit(107, 88, 84, 24))
        {
            m_All = false;
            m_Revision = ~0ull;
        }
        else if (Hit(ScrollX, 150, 13, 14))
            m_First = std::max(0, m_First - 1);
        else if (Hit(ScrollX, 362, 13, 14))
            m_First = std::min(maximum, m_First + 1);
        else if (Hit(ScrollX, ScrollTop, 13, ScrollHeight))
            m_Dragging = true;
    }
    if (m_Dragging)
        m_First = std::clamp((MouseY - Y(ScrollTop)) * maximum / std::max(1, H(ScrollHeight)), 0, maximum);
    MouseLButton = MouseLButtonPush = MouseLButtonPop = false;
    return false;
}
bool ActivityLogView::CapturesMouse() const
{
    return m_Dragging || Hit(0, 0, Width, Height);
}
void ActivityLogView::ClearTooltip()
{
    if (m_Tooltip && g_pNewItemMng)
        g_pNewItemMng->DeleteItem(m_Tooltip);
    m_Tooltip = nullptr;
    m_TooltipIndex = -1;
    m_TooltipRevision = ~0ull;
}
void ActivityLogView::RenderItemTooltip()
{
    int hovered = -1;
    for (int row = 0; row < Rows && m_First + row < static_cast<int>(m_Visible.size()); ++row)
        if (Hit(190, 147 + row * RowHeight, 311, RowHeight))
        {
            hovered = m_Visible[m_First + row];
            break;
        }
    if (hovered < 0 || !g_pNewItemMng)
    {
        ClearTooltip();
        return;
    }
    const auto &history = History(false);
    const auto *record = history.At(hovered);
    if (!record || record->ItemType < 0 || record->ItemType >= MAX_ITEM)
    {
        ClearTooltip();
        return;
    }
    if (m_TooltipIndex != hovered || m_TooltipRevision != history.Revision())
    {
        ClearTooltip();
        ItemCreationParams parameters{};
        parameters.Group = record->ItemType / MAX_ITEM_INDEX;
        parameters.Number = record->ItemType % MAX_ITEM_INDEX;
        parameters.Level = static_cast<BYTE>(record->ItemLevel);
        parameters.Durability = 255;
        parameters.WithLuck = record->Luck != 0;
        parameters.WithOption = record->AdditionalOptionLevel > 0;
        parameters.OptionLevel = static_cast<BYTE>(std::clamp(record->AdditionalOptionLevel, 0, 15));
        parameters.HasExcellentOption = record->Excellent != 0;
        parameters.ExcellentFlags = static_cast<BYTE>(record->Excellent);
        parameters.IsAncient = record->Ancient != 0;
        parameters.AncientDiscriminator = static_cast<BYTE>(record->Ancient);
        std::fill(std::begin(parameters.SocketOptions), std::end(parameters.SocketOptions), SOCKET_EMPTY);
        m_Tooltip = g_pNewItemMng->CreateItemByParameters(&parameters);
        m_TooltipIndex = hovered;
        m_TooltipRevision = history.Revision();
    }
    if (m_Tooltip)
        RenderMarketItemInfo(std::clamp(MouseX + 12, 12, 400), std::clamp(MouseY + 12, 30, 240), m_Tooltip);
}
void ActivityLogView::Render3D()
{
    if (m_Kills)
        return;
    for (int row = 0; row < Rows && m_First + row < static_cast<int>(m_Visible.size()); ++row)
    {
        const auto *r = History(false).At(m_Visible[m_First + row]);
        if (r->ItemType >= 0 && r->ItemType < MAX_ITEM)
            RenderMarketPreviewItem3D(X(202), Y(154 + row * RowHeight), W(34), H(35), r->ItemType, r->ItemLevel,
                                      r->Excellent, r->Ancient, .58f);
    }
    RenderItemTooltip();
}
} // namespace UI::ActivityLogs
