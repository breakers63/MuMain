#include "stdafx.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "GuildRankingView.h"
#include "RankingHoverCard.h"
#include "Character/CharacterManager.h"
#include "ClassPortraits.h"
#include "Network/Server/GuildMembersPacket.h"
#include "Network/Server/GuildRosterRetry.h"
#include "Network/Server/WSclient.h"
#include "PanelAspectScale.h"
#include "ParisFlatStyle.h"
#include "ParisPanelStyle.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzTexture.h"
#include "Scenes/SceneCore.h"
#include <algorithm>
#include <cstring>
namespace UI::Character
{
namespace
{
constexpr int PageSize = 6, CardWidth = 207, CardHeight = 37, CardStep = 41, CardX = 209, CardY = 189;
constexpr DWORD RefreshMilliseconds = 300000;
GuildRankingView view;
const wchar_t *Classes[] = {L"Dark Wizard", L"Dark Knight", L"Fairy Elf",   L"Magic Gladiator",
                            L"Dark Lord",   L"Summoner",    L"Rage Fighter"};
int BaseClass(BYTE serverClass)
{
    return gCharacterManager.GetBaseClass(
        gCharacterManager.ChangeServerClassTypeToClientClassType(static_cast<SERVER_CLASS_TYPE>(serverClass)));
}
int Read(const BYTE *bytes)
{
    int value;
    std::memcpy(&value, bytes, 4);
    return std::max(0, value);
}
} // namespace
GuildRankingView &GuildRanking()
{
    return view;
}
void GuildRankingView::Load()
{
    m_Background = Bitmaps.LoadImage(L"Data\\Interface\\Paris_GuildRankingV14.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_Buttons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingColorsV14.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
}
void GuildRankingView::Release()
{
    for (auto texture : {m_Background, m_Buttons})
        if (texture != BITMAP_UNKNOWN)
            DeleteBitmap(texture);
    m_Background = m_Buttons = BITMAP_UNKNOWN;
    if (m_Font)
        DeleteObject(m_Font);
    if (m_Bold)
        DeleteObject(m_Bold);
    if (m_Title)
        DeleteObject(m_Title);
    m_Font = m_Bold = m_Title = nullptr;
    m_FontSize = 0;
    Clear();
}
void GuildRankingView::Clear()
{
    m_Members.clear();
    m_Guild.clear();
    m_Page = 0;
    m_SelectedMember = 0;
    m_Pending = m_Received = false;
    m_RequestFailed = false;
    m_RequestedAt = 0;
}
int GuildRankingView::X(int v) const
{
    return m_X + W(v);
}
int GuildRankingView::Y(int v) const
{
    return m_Y + H(v);
}
int GuildRankingView::W(int v) const
{
    return PanelAspect::Axis(v, m_Scale, g_fScreenRate_x);
}
int GuildRankingView::H(int v) const
{
    return PanelAspect::Axis(v, m_Scale, g_fScreenRate_y);
}
bool GuildRankingView::Hit(int x, int y, int width, int height) const
{
    return CheckMouseIn(X(x), Y(y), W(width), H(height));
}
void GuildRankingView::SetLayout(int x, int y, float scale)
{
    m_X = x;
    m_Y = y;
    m_Scale = scale;
    const int size = std::max(8, static_cast<int>(11 * scale));
    if (size == m_FontSize)
        return;
    if (m_Font)
        DeleteObject(m_Font);
    if (m_Bold)
        DeleteObject(m_Bold);
    if (m_Title)
        DeleteObject(m_Title);
    m_FontSize = size;
    m_Font = CreateFontW(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_Bold = CreateFontW(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
    m_Title =
        CreateFontW(std::max(12, static_cast<int>(18 * scale)), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
}
void GuildRankingView::Request(const std::wstring &guild)
{
    if (guild == m_Guild)
    {
        if (m_Pending || m_RequestFailed || (m_Received && GetTickCount() - m_RequestedAt < RefreshMilliseconds))
            return;
    }
    else
    {
        Clear();
        m_Guild = guild;
    }
    m_ProtocolVersion = 2;
    SendRequest();
}
void GuildRankingView::Retry()
{
    if (!m_RequestFailed || m_Pending) return;
    m_RequestFailed = false;
    m_ProtocolVersion = 2;
    SendRequest();
}
void GuildRankingView::SendRequest()
{
    if (m_Guild.empty() || !SocketClient || !SocketClient->IsConnected() || SceneFlag != MAIN_SCENE)
        return;
    BYTE packet[15]{0xC1, 15, 0xED, 0x5E, m_ProtocolVersion};
    char name[41]{};
    const int size = WideCharToMultiByte(CP_UTF8, 0, m_Guild.data(), static_cast<int>(m_Guild.size()), name,
                                         sizeof(name), nullptr, nullptr);
    if (size <= 0 || size > 10)
        return;
    std::memcpy(packet + 5, name, size);
    SocketClient->Send(packet, sizeof(packet));
    m_RequestedAt = GetTickCount();
    m_Pending = true;
}
GuildRankingView::Member GuildRankingView::DecodeMember(const BYTE *record, int version)
{
    Member member;
    int nameSize = 0;
    while (nameSize < 10 && record[nameSize])
        ++nameSize;
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char *>(record), nameSize, member.Name,
                        10);
    member.Class = record[10];
    member.Position = record[11];
    member.Level = Read(record + 12);
    member.Resets = Read(record + 16);
    member.Stats = Read(record + 20);
    member.Rank = Read(record + 24);
    if (version == 2)
    {
        member.DetailsAvailable = record[49] == 1;
        member.OnlineKnown = true;
        member.Online = record[48] == 1;
        if (member.DetailsAvailable)
            for (int i = 0; i < 5; ++i)
                member.Attributes[i] = Read(record + 28 + i * 4);
    }
    return member;
}
bool GuildRankingView::Receive(const BYTE *packet, int size)
{
    if (!packet || size < 5 || packet[0] != 0xC2 || packet[3] != 0xED || packet[4] != 0x5F)
        return false;
    const int count = Network::Ranking::GuildMemberCount({packet, static_cast<std::size_t>(size)});
    if (count < 0)
        return true;
    wchar_t name[11]{};
    int length = 0;
    while (length < 10 && packet[6 + length])
        ++length;
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char *>(packet + 6), length, name, 10);
    if (m_Guild != name)
        return true;
    if (m_Received && m_ProtocolVersion == 2 && packet[5] == 1)
        return true;
    std::vector<Member> members;
    members.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        const BYTE *record = packet + 18 + i * (packet[5] == 2 ? 50 : 28);
        const Member member = DecodeMember(record, packet[5]);
        members.push_back(member);
    }
    m_Members = std::move(members);
    m_Page = std::min(m_Page, std::max(0, (static_cast<int>(m_Members.size()) - 1) / PageSize));
    m_SelectedMember = std::clamp(m_SelectedMember, 0, std::max(0, count - 1));
    m_ProtocolVersion = packet[5];
    m_Pending = false;
    m_Received = true;
    m_RequestFailed = false;
    return true;
}
void GuildRankingView::DrawText(int x, int y, int width, const wchar_t *text, bool bold, int alignment)
{
    g_pRenderText->SetFont(bold ? m_Bold : m_Font);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(bold ? 238 : 235, bold ? 209 : 235, bold ? 142 : 235, 255);
    g_pRenderText->RenderText(X(x), Y(y), text, W(width), 0, alignment == 0 ? RT3_SORT_LEFT : alignment);
}
void GuildRankingView::DrawTitle(int x, int y, int width, const wchar_t *text)
{
    g_pRenderText->SetFont(m_Title ? m_Title : m_Bold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(239, 212, 142, 255);
    g_pRenderText->RenderText(X(x), Y(y), text, W(width), 0, RT3_SORT_LEFT);
}
void GuildRankingView::DrawPortrait(const Member &member, int x, int y, int width, int height)
{
    const int base = BaseClass(member.Class);
    if (base >= 0 && base < 6)
        Portraits::Render(base, X(x), Y(y), W(width), H(height));
    else
        DrawText(x, y + height / 2 - 5, width, L"RF", true, RT3_SORT_CENTER);
}
bool GuildRankingView::HandleClick()
{
    if (Hit(212, 447, 72, 26))
    {
        if (m_Page > 0)
            --m_Page;
        return true;
    }
    if (Hit(340, 447, 72, 26))
    {
        if ((m_Page + 1) * PageSize < static_cast<int>(m_Members.size()))
            ++m_Page;
        return true;
    }
    for (int slot = 0; slot < PageSize; ++slot)
    {
        const int index = m_Page * PageSize + slot;
        if (index < static_cast<int>(m_Members.size()) && Hit(CardX, CardY + slot * CardStep, CardWidth, CardHeight))
        {
            m_SelectedMember = index;
            return true;
        }
    }
    return false;
}
void GuildRankingView::RenderCards()
{
    const int first = m_Page * PageSize;
    for (int slot = 0; slot < PageSize && first + slot < static_cast<int>(m_Members.size()); ++slot)
    {
        const auto &member = m_Members[first + slot];
        const int y = CardY + slot * CardStep;
        if (Hit(CardX, y, CardWidth, CardHeight))
        { m_SelectedMember = first + slot; RankingHover::Hover(member.Name); }
        const bool selected = first + slot == m_SelectedMember;
        if (selected)
            ParisFlat::Box(X(CardX), Y(y), W(CardWidth), H(CardHeight), true);
        DrawPortrait(member, CardX + 6, y + 3, 31, 31);
        DrawText(CardX + 45, y + 9, 145, member.Name, selected);
        const int base = BaseClass(member.Class);
        DrawText(CardX + 45, y + 24, 145, base >= 0 && base < 7 ? Classes[base] : L"Personaje");
        if (member.OnlineKnown)
            ParisStyle::Solid(X(CardX + 194), Y(y + 15), W(5), H(5), member.Online ? .36f : .3f,
                              member.Online ? .73f : .3f, member.Online ? .48f : .3f);
    }
    if (m_Members.empty())
    {
        const wchar_t* status = m_RequestFailed ? L"El servidor no respondió" : m_Received ? L"Sin integrantes disponibles"
            : m_Guild.empty() ? L"Selecciona un clan" : L"Cargando integrantes...";
        DrawText(214, 304, 197, status, false, RT3_SORT_CENTER);
        if (m_RequestFailed)
            DrawText(214, 320, 197, L"Selecciona el clan para reintentar.", false, RT3_SORT_CENTER);
    }
}
void GuildRankingView::DrawPagerButton(int x, bool next, bool enabled)
{
    constexpr int Top = 447, Width = 72, Height = 26;
    EnableAlphaTest();
    const float brightness = enabled ? (Hit(x, Top, Width, Height) ? 1.15f : 1.f) : .65f;
    UI::Paris::Color(brightness, brightness, brightness, 1);
    if (m_Buttons != BITMAP_UNKNOWN)
        RenderBitmap(m_Buttons, X(x), Y(Top), W(Width), H(Height), 0.f, (next ? 128.f : 0.f) / 256.f, 200.f / 256.f,
                     56.f / 256.f);
    UI::Paris::Color(1, 1, 1, 1);
    const wchar_t *caption = next ? L"SIGUIENTE ›" : L"‹ ANTERIOR";
    g_pRenderText->SetFont(m_Bold);
    const SIZE textSize = g_pRenderText->MeasureText(caption, lstrlen(caption));
    const int offset = static_cast<int>((H(Height) - textSize.cy / g_fScreenRate_y) * .5f);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, enabled ? 222 : 255, enabled ? 139 : 255, 255);
    g_pRenderText->RenderText(X(x), Y(Top) + offset, caption, W(Width), 0, RT3_SORT_CENTER);
}
void GuildRankingView::RenderPager()
{
    DrawPagerButton(212, false, m_Page > 0);
    DrawPagerButton(340, true, (m_Page + 1) * PageSize < static_cast<int>(m_Members.size()));
    wchar_t page[32]{};
    swprintf_s(page, L"%d / %d", m_Page + 1,
               std::max(1, static_cast<int>((m_Members.size() + PageSize - 1) / PageSize)));
    DrawText(287, 455, 50, page, false, RT3_SORT_CENTER);
    DrawText(214, 491, 196, L"Selecciona un miembro para ver su ficha.", false, RT3_SORT_CENTER);
}
void GuildRankingView::RenderEmblem(const std::array<BYTE, 32> &mark)
{
    constexpr BYTE colors[16][3] = {{0, 0, 0},   {0, 0, 0},     {128, 128, 128}, {255, 255, 255},
                                    {255, 0, 0}, {255, 128, 0}, {255, 255, 0},   {128, 255, 0},
                                    {0, 255, 0}, {0, 255, 128}, {0, 255, 255},   {0, 128, 255},
                                    {0, 0, 255}, {128, 0, 255}, {255, 0, 255},   {255, 0, 128}};
    for (int pixel = 0; pixel < 64; ++pixel)
    {
        const int index = (pixel % 2 == 0) ? mark[pixel / 2] >> 4 : mark[pixel / 2] & 15;
        if (index == 0)
            continue;
        const auto &color = colors[index];
        ParisStyle::Solid(X(221 + (pixel % 8) * 4), Y(103 + (pixel / 8) * 4), W(4), H(4), color[0] / 255.f,
                          color[1] / 255.f, color[2] / 255.f);
    }
}
void GuildRankingView::RenderBackground()
{
    if (m_Background != BITMAP_UNKNOWN)
    {
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(m_Background, X(0), Y(0), W(750), H(560));
    }
}
void GuildRankingView::UpdateRequestState()
{
    if (m_Pending)
    {
        const auto action = Network::Ranking::GuildRosterRetryAction(GetTickCount(), m_RequestedAt, m_ProtocolVersion);
        if (action == Network::Ranking::GuildRosterRetry::LegacyRequest)
        {
            m_ProtocolVersion = 1;
            SendRequest();
        }
        else if (action == Network::Ranking::GuildRosterRetry::Unavailable)
        {
            m_Pending = false;
            m_RequestFailed = true;
        }
    }
    else if (!m_RequestFailed && m_Received && GetTickCount() - m_RequestedAt >= RefreshMilliseconds)
        SendRequest();
}

void GuildRankingView::Render(const std::wstring &guild, const std::wstring &master, const std::array<BYTE, 32> &mark,
                              int rank, int score, int count)
{
    UpdateRequestState();
    DrawTitle(275, 98, 225, guild.empty() ? L"Selecciona un clan" : guild.c_str());
    wchar_t subtitle[96]{};
    swprintf_s(subtitle, L"Master: %ls · GUILD ACTIVO", master.c_str());
    DrawText(275, 121, 225, subtitle);
    RenderEmblem(mark);
    wchar_t text[40]{};
    swprintf_s(text, L"%d TOTAL", count);
    DrawText(374, 171, 40, text, true, RT3_SORT_CENTER);
    RenderStats(rank, score, count, master);
    RenderPager();
    RenderCards();
    RenderSelectedMember();
}
void GuildRankingView::RenderStats(int rank, int score, int count, const std::wstring &master)
{
    const wchar_t *labels[] = {L"PUESTO", L"PUNTOS", L"MIEMBROS"};
    for (int column = 0; column < 3; ++column)
    {
        const int value = column == 0 ? rank : column == 1 ? score : count;
        wchar_t text[40]{};
        swprintf_s(text, column == 0 ? L"#%d" : L"%d", value);
        DrawText(509 + column * 71, 106, 60, labels[column], true, RT3_SORT_CENTER);
        DrawText(509 + column * 71, 126, 60, text, true, RT3_SORT_CENTER);
    }
}
void GuildRankingView::RenderSelectedMember()
{
    if (m_Members.empty())
        return;
    m_SelectedMember = std::clamp(m_SelectedMember, 0, static_cast<int>(m_Members.size()) - 1);
    DrawMemberPanel(m_Members[m_SelectedMember]);
}
void GuildRankingView::DrawMemberPanel(const Member &member)
{
    const int base = BaseClass(member.Class);
    const wchar_t *className = base >= 0 && base < 7 ? Classes[base] : L"Personaje";
    Portraits::RenderPortrait(base, X(446), Y(196), W(83), H(127));
    DrawTitle(545, 197, 173, member.Name);
    wchar_t text[100]{};
    swprintf_s(text, L"%ls · %ls", className, member.Position == 2 ? L"Guild Master" : L"Miembro");
    DrawText(545, 216, 173, text);
    swprintf_s(text, member.Rank > 0 ? L"#%d" : L"--", member.Rank);
    DrawText(685, 169, 35, text, true, RT3_SORT_CENTER);
    swprintf_s(text, L"%ls · LV %d", member.Name, member.Level);
    ParisFlat::Box(X(445), Y(305), W(85), H(20));
    DrawText(446, 311, 83, text, true, RT3_SORT_CENTER);
    DrawText(552, 236, 68, L"NIVEL", true);
    DrawText(640, 236, 68, L"RESET", true);
    swprintf_s(text, L"%d", member.Level);
    DrawText(552, 247, 68, text, true);
    swprintf_s(text, L"%d", member.Resets);
    DrawText(640, 247, 68, text, true);
    DrawText(545, 270, 94, L"TOTAL STATS", true);
    swprintf_s(text, L"%d", member.Stats);
    DrawText(642, 270, 76, text, true, RT3_SORT_RIGHT);
    DrawMemberDetails(member, className);
}
void GuildRankingView::DrawMemberDetails(const Member &member, const wchar_t *className)
{
    wchar_t text[100]{};
    const wchar_t *labels[] = {L"Fuerza", L"Agilidad", L"Vitalidad", L"Energia"};
    for (int i = 0; i < 4; ++i)
    {
        DrawText(545, 292 + i * 12, 90, labels[i]);
        if (member.DetailsAvailable)
            swprintf_s(text, L"%d", member.Attributes[i]);
        else
            wcscpy_s(text, L"--");
        DrawText(642, 292 + i * 12, 76, text, false, RT3_SORT_RIGHT);
    }
    DrawText(552, 350, 162,
             member.OnlineKnown ? (member.Online ? L"EN LINEA" : L"DESCONECTADO") : L"ESTADO NO DISPONIBLE", true);
    const wchar_t *detailLabels[] = {L"Clase", L"Guild", L"Posicion personal", L"Detalles"};
    const wchar_t *detailValues[] = {className, m_Guild.c_str(), nullptr,
                                     member.DetailsAvailable ? L"Publicos" : L"No publicados"};
    for (int i = 0; i < 4; ++i)
    {
        DrawText(454, 412 + i * 16, 116, detailLabels[i]);
        if (i == 2)
            swprintf_s(text, member.Rank > 0 ? L"#%d" : L"Fuera del Top 50", member.Rank);
        else
            wcscpy_s(text, detailValues[i]);
        DrawText(576, 412 + i * 16, 132, text, false, RT3_SORT_RIGHT);
    }
}
} // namespace UI::Character
