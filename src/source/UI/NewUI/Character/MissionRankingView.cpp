#include "stdafx.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "MissionRankingView.h"
#include "RankingHoverCard.h"
#include "PanelAspectScale.h"
#include "ParisFlatStyle.h"
#include "ParisPanelStyle.h"
#include "Network/Server/MissionRankingPacket.h"
#include "Network/Server/WSclient.h"
#include "UI/NewUI/NewUISystem.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/Widgets/ParisTintOverlay.h"
#include <algorithm>
#include <cwctype>
#include <cstring>
#include <cmath>
namespace UI::Character
{
namespace
{
MissionRankingView view;
constexpr int Rows = 6, RowTop = 157, RowStep = 59, ListBottom = 511;
constexpr DWORD CacheMilliseconds = 300000;
constexpr int WheelPixels = 20, TrackTop = RowTop + 14, TrackHeight = ListBottom - RowTop - 28, ThumbHeight = 28;
constexpr float ScrollResponse = 8.f;
bool Contains(const wchar_t *name, const std::wstring &query)
{
    if (query.empty())
        return true;
    const std::size_t length = wcslen(name);
    if (query.size() > length)
        return false;
    for (std::size_t start = 0; start + query.size() <= length; ++start)
    {
        std::size_t i = 0;
        while (i < query.size() && std::towlower(name[start + i]) == std::towlower(query[i]))
            ++i;
        if (i == query.size())
            return true;
    }
    return false;
}
} // namespace
MissionRankingView &MissionRanking()
{
    return view;
}
int MissionRankingView::X(int v) const
{
    return m_X + W(v);
}
int MissionRankingView::Y(int v) const
{
    return m_Y + H(v);
}
int MissionRankingView::W(int v) const
{
    return PanelAspect::Axis(v, m_Scale, g_fScreenRate_x);
}
int MissionRankingView::H(int v) const
{
    return PanelAspect::Axis(v, m_Scale, g_fScreenRate_y);
}
bool MissionRankingView::Hit(int x, int y, int w, int h) const
{
    return SEASON3B::CheckMouseIn(X(x), Y(y), W(w), H(h));
}
void MissionRankingView::Load()
{
    m_ScrollArt.Load();
    m_Background = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MissionRankingV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Atlas = Bitmaps.LoadImage(L"Data\\Interface\\Paris_MissionAtlasV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Frame = Bitmaps.LoadImage(L"Data\\Interface\\Paris_StatsFrameV22.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Coins = Bitmaps.LoadImage(L"Data\\Custom\\Interface\\BattlePass\\zen_coins.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    for (int i = 0; i < 3; ++i)
    {
        wchar_t path[128]{};
        swprintf_s(path, L"Data\\Interface\\Paris_RankMedalV17_%d.tga", i + 1);
        m_Medals[i] = Bitmaps.LoadImage(path, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }
}
void MissionRankingView::Release()
{
    m_ScrollArt.Unload();
    if (m_Background != BITMAP_UNKNOWN)
        DeleteBitmap(m_Background);
    m_Background = BITMAP_UNKNOWN;
    if (m_Coins != BITMAP_UNKNOWN)
        DeleteBitmap(m_Coins);
    m_Coins = BITMAP_UNKNOWN;
    for (auto &image : m_Medals)
    {
        if (image != BITMAP_UNKNOWN)
            DeleteBitmap(image);
        image = BITMAP_UNKNOWN;
    }
    g_pRenderText->SetFont(g_hFont);
    if (m_Font)
        DeleteObject(m_Font);
    if (m_Bold)
        DeleteObject(m_Bold);
    m_Font = m_Bold = nullptr;
    for (auto *texture : {&m_Atlas, &m_Frame})
    {
        if (*texture != BITMAP_UNKNOWN)
            DeleteBitmap(*texture);
        *texture = BITMAP_UNKNOWN;
    }
    if (m_Title)
        DeleteObject(m_Title);
    m_Title = nullptr;
    m_Catalog.clear();
    m_Filtered.clear();
}
void MissionRankingView::SetLayout(int x, int y, float scale)
{
    m_X = x;
    m_Y = y;
    if (m_Font && m_Scale == scale)
        return;
    m_Scale = scale;
    if (m_Font)
        DeleteObject(m_Font);
    if (m_Bold)
        DeleteObject(m_Bold);
    if (m_Title)
        DeleteObject(m_Title);
    const int size = std::max(10, static_cast<int>(14 * scale));
    m_Title =
        CreateFontW(std::max(14, static_cast<int>(24 * scale)), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
    m_Font = CreateFontW(std::max(10, static_cast<int>(12 * scale)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH,
                         L"Georgia");
    m_Bold = CreateFontW(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
}
void MissionRankingView::Open()
{
    if (!m_Catalog.empty() && GetTickCount() - m_CatalogRequestTick < CacheMilliseconds)
        return;
    MouseWheel = 0;
    m_CatalogRequestTick = GetTickCount();
    if (g_pCustomQuestCenter)
        g_pCustomQuestCenter->RequestCatalog();
}
void MissionRankingView::Clear()
{
    m_Revision = ~0ull;
    m_Selected = -1;
    m_First = 0;
    m_Scroll = m_TargetScroll = 0;
    m_Category = -1;
    m_Query.clear();
    m_Catalog.clear();
    m_Filtered.clear();
    m_Count = m_GlobalRank = 0;
    m_Pending = m_Deferred = m_Failed = false;
    m_CatalogRequestTick = 0;
}
void MissionRankingView::Text(int x, int y, int width, const wchar_t *text, bool gold, bool bold)
{
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    g_pRenderText->SetFont(bold ? m_Bold : m_Font);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(gold ? 238 : 255, gold ? 211 : 255, gold ? 139 : 255, 255);
    g_pRenderText->RenderText(X(x), Y(y), text, W(width), 0, RT3_SORT_LEFT);
}
void MissionRankingView::SyncCatalog(const std::wstring &query)
{
    if (!g_pCustomQuestCenter || g_pCustomQuestCenter->IsCatalogLoading())
        return;
    if (m_Revision == g_pCustomQuestCenter->CatalogRevision() && m_Query == query)
        return;
    const int oldId = m_Selected >= 0 ? m_Catalog[m_Selected].Id : -1;
    m_Revision = g_pCustomQuestCenter->CatalogRevision();
    m_Query = query;
    const auto &catalog = g_pCustomQuestCenter->MissionCatalog();
    m_Catalog.assign(catalog.begin(), catalog.begin() + std::min<std::size_t>(512, catalog.size()));
    m_Filtered.clear();
    m_Selected = -1;
    for (int i = 0; i < static_cast<int>(m_Catalog.size()); ++i)
    {
        if ((m_Category < 0 || m_Catalog[i].Category == m_Category) && Contains(m_Catalog[i].Name, m_Query))
            m_Filtered.push_back(i);
        if (m_Catalog[i].Id == oldId)
            m_Selected = i;
    }
    const float maximum = static_cast<float>(std::max(0, static_cast<int>(m_Filtered.size()) - Rows) * RowStep);
    m_TargetScroll = std::clamp(m_TargetScroll, 0.f, maximum);
    m_Scroll = std::clamp(m_Scroll, 0.f, maximum);
    m_First = static_cast<int>(m_Scroll / RowStep);
    if (!m_Filtered.empty() &&
        (m_Selected < 0 || std::find(m_Filtered.begin(), m_Filtered.end(), m_Selected) == m_Filtered.end()))
        Select(m_Filtered.front());
}
void MissionRankingView::Select(int index)
{
    m_Selected = index;
}
void MissionRankingView::Request()
{
    if (m_Selected < 0 || !SocketClient || !SocketClient->IsConnected() || GetTickCount() - m_RequestTick < 350)
        return;
    BYTE packet[9]{0xC1, 9, 0xED, 0x60, 1};
    const int id = m_Catalog[m_Selected].Id;
    std::memcpy(packet + 5, &id, 4);
    SocketClient->Send(packet, sizeof(packet));
    m_RequestTick = GetTickCount();
    m_Pending = true;
    m_Deferred = false;
}
bool MissionRankingView::Receive(const BYTE *packet, int size)
{
    if (!packet || size < 5 || packet[0] != 0xC2 || packet[3] != 0xED || packet[4] != 0x61)
        return false;
    const int count = Network::Ranking::MissionEntryCount({packet, static_cast<std::size_t>(size)});
    if (count < 0)
        return true;
    int id = 0;
    std::memcpy(&id, packet + 6, 4);
    if (m_Selected < 0 || m_Catalog[m_Selected].Id != id)
        return true;
    for (int i = 0; i < count; ++i)
    {
        const auto *row = packet + 11 + i * 18;
        auto &entry = m_Entries[i];
        entry = {};
        int length = 0;
        while (length < 10 && row[length])
            ++length;
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char *>(row), length, entry.Name, 10);
        std::memcpy(&entry.Progress, row + 10, 4);
        std::memcpy(&entry.Elapsed, row + 14, 4);
    }
    m_Count = count;
    m_Pending = m_Failed = false;
    return true;
}
void MissionRankingView::ReceiveGlobal(const BYTE *packet, int size, int payloadOffset)
{
    const int count = packet[payloadOffset + 1];
    if (count > 50 || size != payloadOffset + 2 + count * 18)
        return;
    for (int i = 0; i < count; ++i)
    {
        const auto *row = packet + payloadOffset + 2 + i * 18;
        auto &entry = m_Entries[i];
        entry = {};
        int length = 0;
        while (length < 10 && row[length])
            ++length;
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char *>(row), length, entry.Name, 10);
        std::memcpy(&entry.Progress, row + 10, 4);
        std::memcpy(&entry.Elapsed, row + 14, 4); // Configured mission total in the global packet.
    }
    m_Count = count;
    m_Pending = m_Failed = false;
    m_RequestTick = GetTickCount();
}
void MissionRankingView::AdvanceScroll()
{
    const DWORD now = GetTickCount();
    const float dt = std::min(0.05f, (now - m_ScrollTick) / 1000.f);
    m_ScrollTick = now;
    m_Scroll += (m_TargetScroll - m_Scroll) * (1.f - std::exp(-ScrollResponse * dt));
    if (std::abs(m_TargetScroll - m_Scroll) < .15f)
        m_Scroll = m_TargetScroll;
    m_First = static_cast<int>(m_Scroll / RowStep);
}
void MissionRankingView::RenderBackground()
{
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    if (m_Background != BITMAP_UNKNOWN)
        RenderBitmap(m_Background, X(16), Y(84), W(718), H(453), 16.f / 750.f, 84.f / 560.f, 718.f / 750.f,
                     453.f / 560.f);
}
void MissionRankingView::RenderFrame()
{
    for (auto r : {std::array<int, 4>{0, 0, 750, 9}, {0, 551, 750, 9}, {0, 0, 9, 560}, {741, 0, 9, 560}})
        ParisStyle::Solid(X(r[0]), Y(r[1]), W(r[2]), H(r[3]), .03f, .035f, .04f);
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    if (m_Frame != BITMAP_UNKNOWN)
        RenderBitmap(m_Frame, m_X, m_Y, W(750), H(560));
}
void MissionRankingView::Atlas(int x, int y, int w, int h, int sx, int sy, int sw, int sh)
{
    if (m_Atlas == BITMAP_UNKNOWN)
        return;
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    RenderBitmap(m_Atlas, X(x), Y(y), W(w), H(h), sx / 1448.f, sy / 1086.f, sw / 1448.f, sh / 1086.f);
}
void MissionRankingView::DrawList()
{
    ParisStyle::Solid(X(21), Y(154), W(258), H(361), .025f, .032f, .035f);
    ParisStyle::Solid(X(293), Y(503), W(439), H(31), .025f, .032f, .035f);
    wchar_t text[128]{};
    ParisStyle::Solid(X(25), Y(94), W(222), H(23), .025f, .032f, .035f);
    Text(26, 99, 215, L"MISIONES", true, true);
    swprintf_s(text, L"%zu", m_Catalog.size());
    Text(253, 100, 22, text, false, true);
    UI::Paris::Clip(X(21), Y(RowTop), W(258), H(ListBottom - RowTop));
    for (int row = 0; row < Rows + 1 && m_First + row < static_cast<int>(m_Filtered.size()); ++row)
    {
        const int index = m_Filtered[m_First + row],
                  y = RowTop + row * RowStep - static_cast<int>(m_Scroll - m_First * RowStep);
        const auto &q = m_Catalog[index];
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(m_Background, X(22), Y(y), W(239), H(46), 42.f / 1448.f,
                     (index == m_Selected ? 377.f : 478.f) / 1086.f, 462.f / 1448.f, 89.f / 1086.f);
        ParisStyle::Solid(X(60), Y(y + 4), W(35), H(38), .025f, .032f, .035f);
        swprintf_s(text, L"#%02d", q.Id);
        Text(32, y + 17, 24, text, true, true);
        Text(99, y + 10, 88, q.Name, false, true);
        swprintf_s(text, L"%u x %ls", q.RequiredKills, q.ObjectiveName);
        Text(99, y + 29, 88, text);
        if (q.RewardCount)
        {
            swprintf_s(text, L"%u", q.Rewards[0].Amount);
            Text(211, y + 11, 46, text);
            Text(211, y + 29, 46, q.Rewards[0].Type == 3 ? L"Paris Coins" : L"Premio");
        }
    }
    UI::Paris::ResetClip();
    const int maximum = std::max(0, static_cast<int>(m_Filtered.size()) - Rows) * RowStep;
    const int thumb = ThumbHeight;
    m_ScrollArt.Scroll(X(269), Y(TrackTop), W(7), H(TrackHeight),
                       Y(TrackTop + (maximum ? static_cast<int>(m_Scroll * (TrackHeight - thumb) / maximum) : 0)),
                       H(thumb));
    if (m_Filtered.empty())
        Text(40, 310, 200, m_Catalog.empty() ? L"Cargando misiones..." : L"Sin resultados");
}
void MissionRankingView::DrawDetails()
{
    if (m_Selected < 0)
        return;
    const auto &q = m_Catalog[m_Selected];
    wchar_t text[256]{};
    // Cover both reference badges, including their frames, and place the title in their freed area.
    ParisStyle::Solid(X(300), Y(96), W(425), H(49), .025f, .032f, .035f);
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    g_pRenderText->SetFont(m_Title);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(238, 211, 139, 255);
    g_pRenderText->RenderText(X(302), Y(106), q.Name, W(420), 0, RT3_SORT_LEFT);
    ParisStyle::Solid(X(302), Y(149), W(118), H(94), .025f, .032f, .035f);
    swprintf_s(text, L"Derrota %u %ls", q.RequiredKills, q.ObjectiveName);
    Text(479, 173, 240, text, false, true);
    if (q.RewardCount)
    {
        swprintf_s(text, L"%ls", q.Rewards[0].DisplayName);
        Text(479, 223, 242, text, false, true);
    }
    const unsigned current = std::min(q.CurrentKills, q.RequiredKills);
    const int percent = q.RequiredKills ? static_cast<int>(100ull * current / q.RequiredKills) : 0;
    swprintf_s(text, L"%u / %u  (%d%%)", current, q.RequiredKills, percent);
    Text(638, 262, 90, text, false, true);
    const int filled = 410 * percent / 100;
    for (int row = 0; row < 11; ++row)
    {
        const float shine = row < 4 ? .3f + row * .16f : .92f - (row - 4) * .075f;
        ParisStyle::Solid(X(305), Y(280 + row), W(filled), H(1), .02f, shine, .045f);
    }
    ParisStyle::Solid(X(294), Y(314), W(431), H(187), .025f, .032f, .035f);
    Text(304, 320, 400, L"RANKING DE MISIONES", true, true);
    Text(312, 344, 72, L"PUESTO", true, true);
    Text(392, 344, 180, L"JUGADOR", true, true);
    Text(610, 344, 105, L"MISIONES", true, true);
    for (int row = 0; row < 6; ++row)
        ParisStyle::Solid(X(301), Y(358 + row * 27), W(418), H(1), .35f, .26f, .09f);
    for (int row = 0; row < std::min(5, m_Count); ++row)
    {
        const int y = 365 + row * 27;
        if (row < 3)
            Atlas(318, y - 4, 25, 24, 610, 690 + row * 54, 58, 54);
        else
        {
            swprintf_s(text, L"%d", row + 1);
            Text(328, y, 26, text);
        }
        Text(392, y, 145, m_Entries[row].Name, false, true);
        if (Hit(301, y - 5, 418, 27))
            RankingHover::Hover(m_Entries[row].Name);
        swprintf_s(text, L"%d / %d", m_Entries[row].Progress, m_Entries[row].Elapsed);
        Text(624, y, 85, text);
    }
    if (m_Count == 0)
        Text(414, 410, 283,
             m_Pending  ? L"Cargando ranking..."
             : m_Failed ? L"Sin respuesta del servidor"
                        : L"Sin misiones completadas");
}
void MissionRankingView::Render(const std::wstring &search)
{
    SyncCatalog(search);
    AdvanceScroll();
    if (m_Pending && GetTickCount() - m_RequestTick > 6000)
    {
        m_Pending = false;
        m_Failed = true;
    }
    DrawList();
    DrawDetails();
}
bool MissionRankingView::Mouse()
{
    const float maximum = static_cast<float>(std::max(0, static_cast<int>(m_Filtered.size()) - Rows) * RowStep);
    if (MouseWheel && Hit(16, 85, 271, 452))
    {
        m_TargetScroll = std::clamp(m_TargetScroll + (MouseWheel > 0 ? -WheelPixels : WheelPixels), 0.f, maximum);
        MouseWheel = 0;
    }
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        m_Dragging = false;
    if (SEASON3B::IsPress(VK_LBUTTON))
    {
        const int arrow = UI::Theme::ParisExactAssets::ScrollArrow(X(269), Y(TrackTop), W(7), H(TrackHeight));
        if (arrow)
        {
            m_TargetScroll = std::clamp(m_TargetScroll + arrow * WheelPixels, 0.f, maximum);
            return true;
        }
        if (Hit(264, TrackTop, 16, TrackHeight))
        {
            const float mouse = (MouseY - Y(TrackTop)) / static_cast<float>(std::max(1, H(TrackHeight))) * TrackHeight;
            const float thumb = maximum > 0 ? m_Scroll * (TrackHeight - ThumbHeight) / maximum : 0;
            m_GrabOffset = mouse >= thumb && mouse <= thumb + ThumbHeight ? mouse - thumb : ThumbHeight * .5f;
            m_Dragging = true;
        }
        for (int row = 0; row < Rows + 1 && m_First + row < static_cast<int>(m_Filtered.size()); ++row)
        {
            const int y = RowTop + row * RowStep - static_cast<int>(m_Scroll - m_First * RowStep);
            if (Hit(22, std::max(RowTop, y), 239, std::max(0, std::min(ListBottom, y + 46) - std::max(RowTop, y))))
            {
                Select(m_Filtered[m_First + row]);
                return true;
            }
        }
    }
    if (m_Dragging)
    {
        const float mouse = (MouseY - Y(TrackTop)) / static_cast<float>(std::max(1, H(TrackHeight))) * TrackHeight;
        m_TargetScroll = std::clamp((mouse - m_GrabOffset) * maximum / (TrackHeight - ThumbHeight), 0.f, maximum);
    }
    return Hit(16, 85, 720, 457);
}

bool MissionRankingView::MonsterPreview(SEASON3B::CustomQuestMonsterPreviewInfo &preview, int row) const
{
    if (m_Selected < 0 || (row >= 0 && m_First + row >= static_cast<int>(m_Filtered.size())))
        return false;
    const auto &quest = m_Catalog[row < 0 ? m_Selected : m_Filtered[m_First + row]];
    preview = {};
    preview.Active = true;
    preview.MonsterNumber = quest.MonsterNumber;
    preview.X = X(row < 0 ? 302 : 60);
    preview.Y = Y(row < 0 ? 149 : RowTop + row * RowStep + 4 - static_cast<int>(m_Scroll - m_First * RowStep));
    preview.Width = W(row < 0 ? 118 : 35);
    preview.Height = H(row < 0 ? 94 : 38);
    wcsncpy_s(preview.Name, quest.ObjectiveName, _TRUNCATE);
    return true;
}
} // namespace UI::Character
