#include "stdafx.h"
#include "UI/NewUI/Character/ParisExactAssets.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
namespace { UI::Theme::ParisExactAssets g_InfoScroll;  }
#include "UI/NewUI/Character/NewUIAdvancedStatsWindow.h"

#include "Audio/DSPlaySound.h"
#include "Network/Paris/ParisRequests.h"
#include "UI/NewUI/Character/NewUICharacterInfoWindow.h"
#include "UI/NewUI/NewUISystem.h"

#include "GameLogic/Character/AdvancedStatsSummary.h"
#include <algorithm>
#include <cwchar>

extern int MouseWheel;

using namespace SEASON3B;

static_assert(
    static_cast<int>(CNewUIAdvancedStatsWindow::Field::Count)
        == CNewUIAdvancedStatsWindow::PROTOCOL_FIELD_COUNT,
    "Advanced Stats field count must match the network protocol.");

SEASON3B::CNewUIAdvancedStatsWindow* g_pAdvancedStatsWindow = nullptr;

namespace
{
    constexpr BYTE AdvancedStatsHeadCode = 0xED;
    constexpr BYTE AdvancedStatsRequest = 0x20;
    constexpr BYTE AdvancedStatsSnapshot = 0x30;
    constexpr BYTE AdvancedStatsProtocolVersion = 2;
    // Keep the client interval slightly above the server-side 2000 ms guard.
    // This avoids sending a request which the server silently rejects at the boundary.
    constexpr DWORD RequestCooldownMilliseconds = 2250;
    constexpr DWORD RequestTimeoutMilliseconds = 5000;
    constexpr DWORD SnapshotRefreshMilliseconds = 2500;

    constexpr int ContentX = 12;
    constexpr int ContentY = 58;
    constexpr int ContentWidth = 158;
    constexpr int ContentHeight = 323;
    constexpr int ScrollTrackX = 174;
    constexpr int ScrollTrackY = 60;
    constexpr int ScrollTrackWidth = 6;
    constexpr int ScrollTrackHeight = 304;
    constexpr int SectionHeight = 18;
    constexpr int RowHeight = 15;

    bool IsCharacterVisible()
    {
        return g_pNewUISystem != nullptr
            && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER);
    }


}

CNewUIAdvancedStatsWindow::CNewUIAdvancedStatsWindow()
    : m_Manager(nullptr),
      m_PanelPos{ 260, 0 },
      m_WindowOpen(false),
      m_HasSnapshot(false),
      m_RequestPending(false),
      m_LastRequestTick(0),
      m_RequestSentTick(0),
      m_LastSnapshotTick(0),
      m_Values{},
      m_ScrollOffset(0),
      m_ContentHeight(525),
      m_DraggingScroll(false),
      m_ScrollDragOffset(0),
      m_CloseRect{ 169, 7, 13, 12 },
      m_DebugEnabled(false)
{
}

CNewUIAdvancedStatsWindow::~CNewUIAdvancedStatsWindow()
{
    Release();
}

bool CNewUIAdvancedStatsWindow::Create(CNewUIManager* manager)
{
    if (manager == nullptr)
    {
        return false;
    }

    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    m_Manager = manager;
    m_Manager->AddUIObj(ADVANCED_STATS_INTERFACE_KEY, this);
    LoadImages();
    Show(true);
    Enable(true);
    g_pAdvancedStatsWindow = this;
    return true;
}

void CNewUIAdvancedStatsWindow::Release()
{
    if (g_pAdvancedStatsWindow == this)
    {
        g_pAdvancedStatsWindow = nullptr;
    }

    UnloadImages();

    if (m_Manager != nullptr)
    {
        m_Manager->RemoveUIObj(this);
        m_Manager = nullptr;
    }
}

void CNewUIAdvancedStatsWindow::LoadImages()
{
    g_InfoScroll.Load();
}

void CNewUIAdvancedStatsWindow::UnloadImages()
{
    g_InfoScroll.Unload();
}

void CNewUIAdvancedStatsWindow::AnchorToCharacter()
{
    if (g_pCharacterInfoWindow == nullptr)
    {
        return;
    }

    const POINT& characterPosition = g_pCharacterInfoWindow->GetPos();
    m_PanelPos.x = characterPosition.x - WINDOW_WIDTH + 10;
    m_PanelPos.y = characterPosition.y;

    if (m_PanelPos.x < 0)
    {
        m_PanelPos.x = characterPosition.x + CNewUICharacterInfoWindow::CHAINFO_WINDOW_WIDTH;
    }
}

void CNewUIAdvancedStatsWindow::OpenWindow()
{
    if (!IsCharacterVisible())
    {
        return;
    }

    AnchorToCharacter();

    if (g_pNewUISystem->IsVisible(INTERFACE_INVENTORY))
    {
        g_pNewUISystem->Hide(INTERFACE_INVENTORY);
    }

    if (g_pNewUISystem->IsVisible(INTERFACE_MYQUEST))
    {
        g_pNewUISystem->Hide(INTERFACE_MYQUEST);
    }

    if (g_pNewUISystem->IsVisible(INTERFACE_PET))
    {
        g_pNewUISystem->Hide(INTERFACE_PET);
    }

    if (g_pNewUISystem->IsVisible(INTERFACE_MASTER_LEVEL))
    {
        g_pNewUISystem->Hide(INTERFACE_MASTER_LEVEL);
    }

    m_WindowOpen = true;
    m_DraggingScroll = false;

    // Never display values from a previous stat/equipment state while a new
    // server snapshot is pending.
    InvalidateSnapshot();
    RequestSnapshot();
    PlayBuffer(SOUND_CLICK01);
}

void CNewUIAdvancedStatsWindow::CloseWindow(bool playSound)
{
    if (!m_WindowOpen)
    {
        return;
    }

    m_WindowOpen = false;
    m_DraggingScroll = false;
    m_RequestPending = false;

    if (playSound)
    {
        PlayBuffer(SOUND_CLICK01);
    }
}

bool CNewUIAdvancedStatsWindow::IsWindowOpen() const
{
    return m_WindowOpen;
}

void CNewUIAdvancedStatsWindow::InvalidateSnapshot()
{
    m_HasSnapshot = false;
    m_RequestPending = false;
    m_LastSnapshotTick = 0;
    m_Values.fill(0);
}

void CNewUIAdvancedStatsWindow::RequestSnapshot()
{
    if (m_RequestPending)
    {
        return;
    }

    const DWORD now = GetTickCount();
    if (m_LastRequestTick != 0
        && now - m_LastRequestTick < RequestCooldownMilliseconds)
    {
        return;
    }

    if (!Network::Paris::RequestAdvancedStats())
    {
        m_RequestPending = false;
        return;
    }

    m_LastRequestTick = now;
    m_RequestSentTick = now;
    m_RequestPending = true;
}

void CNewUIAdvancedStatsWindow::ReceivePacket(const BYTE* packet, int size)
{
    if (packet == nullptr || size < 8)
    {
        return;
    }

    // Ignore delayed snapshots after the window was closed, or packets which
    // were not produced by the currently pending request.
    if (!m_WindowOpen || !m_RequestPending)
    {
        return;
    }

    int headCodeOffset = 2;
    int subCodeOffset = 3;
    int payloadOffset = 4;

    if (packet[0] == 0xC2 || packet[0] == 0xC4)
    {
        headCodeOffset = 3;
        subCodeOffset = 4;
        payloadOffset = 5;
    }

    if (size <= payloadOffset + 3
        || packet[headCodeOffset] != AdvancedStatsHeadCode
        || packet[subCodeOffset] != AdvancedStatsSnapshot
        || packet[payloadOffset] != AdvancedStatsProtocolVersion
        || packet[payloadOffset + 1] != PROTOCOL_FIELD_COUNT)
    {
        return;
    }

    const int valuesOffset = payloadOffset + 4;
    const int requiredSize = valuesOffset + (PROTOCOL_FIELD_COUNT * static_cast<int>(sizeof(std::int32_t)));
    if (size < requiredSize)
    {
        return;
    }

    for (int i = 0; i < PROTOCOL_FIELD_COUNT; ++i)
    {
        m_Values[i] = ReadInt32(packet + valuesOffset + (i * sizeof(std::int32_t)));
    }

    m_HasSnapshot = true;
    m_RequestPending = false;
    m_LastSnapshotTick = GetTickCount();
}

std::int32_t CNewUIAdvancedStatsWindow::ReadInt32(const BYTE* data)
{
    const std::uint32_t value = static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
    return static_cast<std::int32_t>(value);
}

bool CNewUIAdvancedStatsWindow::Update()
{
    if (!IsCharacterVisible())
    {
        CloseWindow(false);
        return true;
    }

    AnchorToCharacter();

    const DWORD now = GetTickCount();

    if (m_RequestPending
        && now - m_RequestSentTick >= RequestTimeoutMilliseconds)
    {
        m_RequestPending = false;
    }

    // The original implementation requested data only once when the panel was
    // opened. Equipment, buffs and stat changes therefore left the old values
    // visible indefinitely. Refresh at a low frequency while the panel is open.
    if (m_WindowOpen
        && !m_RequestPending
        && (!m_HasSnapshot
            || now - m_LastSnapshotTick >= SnapshotRefreshMilliseconds))
    {
        RequestSnapshot();
    }

    ClampScroll();
    return true;
}

bool CNewUIAdvancedStatsWindow::UpdateKeyEvent()
{
    if (!IsCharacterVisible())
    {
        return true;
    }

    const bool control = SEASON3B::IsRepeat(VK_CONTROL);

    if (false && control && SEASON3B::IsPress(VK_F11))
    {
        m_DebugEnabled = !m_DebugEnabled;
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_DebugEnabled)
    {
        UiRect& rectangle = m_CloseRect;

        const bool resize = SEASON3B::IsRepeat(VK_SHIFT);
        int dx = 0;
        int dy = 0;

        if (SEASON3B::IsPress(VK_LEFT))
        {
            dx = -1;
        }
        else if (SEASON3B::IsPress(VK_RIGHT))
        {
            dx = 1;
        }
        else if (SEASON3B::IsPress(VK_UP))
        {
            dy = -1;
        }
        else if (SEASON3B::IsPress(VK_DOWN))
        {
            dy = 1;
        }

        if (dx != 0 || dy != 0)
        {
            if (resize)
            {
                // Free resize for fine tuning.
                rectangle.Width = std::max(1, rectangle.Width + dx);
                rectangle.Height = std::max(1, rectangle.Height + dy);
            }
            else
            {
                rectangle.X += dx;
                rectangle.Y += dy;
            }

            return false;
        }
    }

    if (m_WindowOpen && SEASON3B::IsPress(VK_ESCAPE))
    {
        CloseWindow();
        return false;
    }

    return true;
}

bool CNewUIAdvancedStatsWindow::UpdateMouseEvent()
{
    if (!IsCharacterVisible())
    {
        return true;
    }

    AnchorToCharacter();

    if (m_WindowOpen)
    {
        if (m_DraggingScroll)
        {
            if (SEASON3B::IsRepeat(VK_LBUTTON))
            {
                SetScrollFromThumb(MouseY - m_ScrollDragOffset);
                return false;
            }

            m_DraggingScroll = false;
        }

        if (CheckPanelMouse() && MouseWheel != 0)
        {
            m_ScrollOffset -= MouseWheel * 45;
            MouseWheel = 0;
            ClampScroll();
            return false;
        }

        if (SEASON3B::IsPress(VK_LBUTTON) && CheckCloseMouse())
        {
            CloseWindow();
            return false;
        }

        const int arrow=UI::Theme::ParisExactAssets::ScrollArrow(m_PanelPos.x+ScrollTrackX,m_PanelPos.y+ScrollTrackY,ScrollTrackWidth,ScrollTrackHeight);
        if(arrow && SEASON3B::IsPress(VK_LBUTTON))
        {
            m_ScrollOffset+=arrow*45; ClampScroll(); return false;
        }
        if (SEASON3B::IsPress(VK_LBUTTON) && CheckScrollThumbMouse())
        {
            m_DraggingScroll = true;
            m_ScrollDragOffset = MouseY - GetScrollThumbY();
            return false;
        }

        if (SEASON3B::IsPress(VK_LBUTTON) && CheckScrollTrackMouse())
        {
            m_ScrollDragOffset = GetScrollThumbHeight() / 2;
            SetScrollFromThumb(MouseY - m_ScrollDragOffset);
            return false;
        }
    }

    if (m_WindowOpen && CheckPanelMouse())
    {
        return false;
    }

    return true;
}

bool CNewUIAdvancedStatsWindow::Render()
{
    if (!IsCharacterVisible())
    {
        return true;
    }

    AnchorToCharacter();

    EnableAlphaTest();
    UI::Paris::Color(1.0f, 1.0f, 1.0f, 1.0f);

    if (m_WindowOpen)
    {
        RenderPanel();
    }

    if (m_DebugEnabled)
    {
        RenderDebug();
    }

    DisableAlphaBlend();
    return true;
}

void CNewUIAdvancedStatsWindow::RenderPanel()
{
    UI::Paris::Frame(m_PanelPos.x, m_PanelPos.y, WINDOW_WIDTH, WINDOW_HEIGHT);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 220, 115, 255);
    g_pRenderText->RenderText(
        m_PanelPos.x,
        m_PanelPos.y + 43,
        L"Informacion del personaje",
        WINDOW_WIDTH,
        0,
        RT3_SORT_CENTER);

    const int closeX = m_PanelPos.x + m_CloseRect.X;
    const int closeY = m_PanelPos.y + m_CloseRect.Y;
    const bool closeHovered = CheckCloseMouse();

    if (closeHovered)
    {
        UI::Paris::Color(0.90f, 0.65f, 0.10f, 0.35f);
        RenderColor(closeX, closeY, m_CloseRect.Width, m_CloseRect.Height);
        EndRenderColor();
    }

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(closeHovered ? 255 : 210, closeHovered ? 220 : 195, closeHovered ? 100 : 175, 255);

    RenderRows();
    RenderScrollBar();
    RenderStatus();
}

void CNewUIAdvancedStatsWindow::RenderRows()
{
    static const RowDefinition rows[] =
    {
        { L"OFFENSIVE", Field::Count, ValueFormat::Integer, true },
        { L"Critical DMG Rate", Field::CriticalDamageChance, ValueFormat::Percent, false },
        { L"Critical DMG Bonus", Field::CriticalDamageBonus, ValueFormat::Integer, false },
        { L"Excellent DMG Rate", Field::ExcellentDamageChance, ValueFormat::Percent, false },
        { L"Double DMG Rate", Field::DoubleDamageChance, ValueFormat::Percent, false },
        { L"Ignore Defense", Field::DefenseIgnoreChance, ValueFormat::Percent, false },
        { L"Min Physical DMG", Field::MinimumPhysBaseDmg, ValueFormat::Integer, false },
        { L"Max Physical DMG", Field::MaximumPhysBaseDmg, ValueFormat::Integer, false },
        { L"Min Magic DMG", Field::MinimumWizBaseDmg, ValueFormat::Integer, false },
        { L"Max Magic DMG", Field::MaximumWizBaseDmg, ValueFormat::Integer, false },

        { L"DEFENSIVE", Field::Count, ValueFormat::Integer, true },
        { L"Reflect DMG", Field::DamageReflection, ValueFormat::Percent, false },
        { L"Return Enemy Attack Power", Field::FullyReflectDamageAfterHitChance, ValueFormat::Percent, false },
        { L"Armor DMG Reduction", Field::ArmorDamageDecrease, ValueFormat::Percent, false },
        { L"Soul Barrier Reduction", Field::SoulBarrierReceiveDecrement, ValueFormat::Percent, false },

        { L"PVP & RATES", Field::Count, ValueFormat::Integer, true },
        { L"PvM Attack Rate", Field::AttackRatePvm, ValueFormat::Integer, false },
        { L"PvP Attack Rate", Field::AttackRatePvp, ValueFormat::Integer, false },
        { L"PvM Defense Rate", Field::DefenseRatePvm, ValueFormat::Integer, false },
        { L"PvP Defense Rate", Field::DefenseRatePvp, ValueFormat::Integer, false },
        { L"Ignore SD Rate", Field::ShieldBypassChance, ValueFormat::Percent, false },
        { L"SD Absorption Increase", Field::ShieldRateIncrease, ValueFormat::Percent, false },
        { L"SD Damage Increase", Field::ShieldDecreaseRateIncrease, ValueFormat::Percent, false },

        { L"RECOVERY", Field::Count, ValueFormat::Integer, true },
        { L"Full HP Recovery on Hit", Field::FullyRecoverHealthAfterHitChance, ValueFormat::Percent, false },
        { L"Full MP Recovery on Hit", Field::FullyRecoverManaAfterHitChance, ValueFormat::Percent, false },

        { L"RESISTANCES", Field::Count, ValueFormat::Integer, true },
        { L"Ice Resistance", Field::IceResistance, ValueFormat::Integer, false },
        { L"Poison Resistance", Field::PoisonResistance, ValueFormat::Integer, false },
        { L"Lightning Resistance", Field::LightningResistance, ValueFormat::Integer, false },
        { L"Fire Resistance", Field::FireResistance, ValueFormat::Integer, false },
        { L"Earth Resistance", Field::EarthResistance, ValueFormat::Integer, false },
        { L"Wind Resistance", Field::WindResistance, ValueFormat::Integer, false },
        { L"Water Resistance", Field::WaterResistance, ValueFormat::Integer, false },
    };

    const int viewportTop = m_PanelPos.y + ContentY;
    const int viewportBottom = viewportTop + ContentHeight;
    int contentY = 0;
    int optionRowIndex = 0;

    for (const RowDefinition& row : rows)
    {
        const int height = row.Section ? SectionHeight : RowHeight;
        const int y = viewportTop + contentY - m_ScrollOffset;
        const int currentOptionRow = optionRowIndex;

        if (!row.Section)
        {
            ++optionRowIndex;
        }

        if (y >= viewportTop && y + height <= viewportBottom)
        {
            if (row.Section)
            {
                g_pRenderText->SetFont(g_hFontBold);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->SetTextColor(255, 188, 70, 255);
                g_pRenderText->RenderText(
                    m_PanelPos.x + ContentX,
                    y + 4,
                    row.Label,
                    ContentWidth,
                    0,
                    RT3_SORT_CENTER);
            }
            else
            {
                // Only keep the alternating darkness fill.
                // No visible rectangular frame so the panel stays cleaner.
                const BYTE backgroundAlpha = (currentOptionRow % 2) == 0 ? 128 : 51;
                const int frameX = m_PanelPos.x + ContentX;
                const int frameY = y;
                const int frameWidth = ContentWidth;
                const int frameHeight = height - 1;

                EnableAlphaTest();
                SetRenderColor(0, 0, 0, backgroundAlpha);
                RenderColor(frameX, frameY, frameWidth, frameHeight);
                EndRenderColor();

                g_pRenderText->SetFont(g_hFont);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->SetTextColor(220, 220, 220, 255);
                g_pRenderText->RenderText(
                    m_PanelPos.x + ContentX + 4,
                    y + 3,
                    row.Label,
                    112,
                    0,
                    RT3_SORT_LEFT);

                RenderValue(m_PanelPos.x + ContentX + 112, y + 3, 42, row);
                UI::Character::ParisStyle::Solid(frameX, y+height-1, frameWidth, 1, .26f,.22f,.13f);
            }
        }

        contentY += height;
    }

    m_ContentHeight = contentY;
    ClampScroll();
}

void CNewUIAdvancedStatsWindow::RenderValue(int x, int y, int width, const RowDefinition& row) const
{
    wchar_t valueText[64] = {};
    const int index = static_cast<int>(row.DataField);
    const std::int32_t value = index >= 0 && index < PROTOCOL_FIELD_COUNT
        ? m_Values[index]
        : 0;

    if (!m_HasSnapshot)
    {
        mu_swprintf(valueText, L"--");
    }
    else if (row.Format == ValueFormat::Percent)
    {
        const double percentage = static_cast<double>(value) / 100.0;
        mu_swprintf(valueText, L"%.2f%%", percentage);
    }
    else
    {
        mu_swprintf(valueText, L"%d", value);
    }

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 220, 115, 255);
    g_pRenderText->RenderText(x, y, valueText, width, 0, RT3_SORT_RIGHT);
}

void CNewUIAdvancedStatsWindow::RenderScrollBar()
{
    const int trackX = m_PanelPos.x + ScrollTrackX;
    const int trackY = m_PanelPos.y + ScrollTrackY;
    const int thumbY = GetScrollThumbY();
    const int thumbHeight = GetScrollThumbHeight();

    using namespace UI::Character::ParisStyle;
    const bool hovered = CheckScrollThumbMouse() || m_DraggingScroll;
    g_InfoScroll.Scroll(trackX, trackY, ScrollTrackWidth, ScrollTrackHeight, thumbY, thumbHeight);

}

void CNewUIAdvancedStatsWindow::RenderStatus()
{
    wchar_t totalPowerText[96] = {};
    wchar_t totalDefenseText[96] = {};

    if (m_HasSnapshot)
    {
        const long long totalPower = GameLogic::Character::AdvancedStats::ComputeTotalPower(m_Values);
        const long long totalDefense = GameLogic::Character::AdvancedStats::ComputeTotalDefense(m_Values);
        mu_swprintf(totalPowerText, L"Total Power: %lld", totalPower);
        mu_swprintf(totalDefenseText, L"Total Defense: %lld", totalDefense);
    }
    else
    {
        mu_swprintf(totalPowerText, L"Total Power: --");
        mu_swprintf(totalDefenseText, L"Total Defense: --");
    }

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 220, 115, 255);
    g_pRenderText->RenderText(
        m_PanelPos.x + 10,
        m_PanelPos.y + 392,
        totalPowerText,
        170,
        0,
        RT3_SORT_CENTER);

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(220, 220, 220, 255);
    g_pRenderText->RenderText(
        m_PanelPos.x + 10,
        m_PanelPos.y + 406,
        totalDefenseText,
        170,
        0,
        RT3_SORT_CENTER);
}

void CNewUIAdvancedStatsWindow::RenderCodeButton(
    int x,
    int y,
    int width,
    int height,
    const wchar_t* text,
    bool hovered,
    bool active) const
{
    if (active)
    {
        UI::Paris::Color(0.48f, 0.30f, 0.06f, 0.78f);
    }
    else if (hovered)
    {
        UI::Paris::Color(0.38f, 0.25f, 0.06f, 0.72f);
    }
    else
    {
        UI::Paris::Color(0.05f, 0.05f, 0.05f, 0.72f);
    }

    RenderColor(x, y, width, height);
    EndRenderColor();

    UI::Paris::Color(0.82f, 0.62f, 0.22f, hovered || active ? 0.95f : 0.70f);
    RenderColor(x, y, width, 1);
    RenderColor(x, y + height - 1, width, 1);
    RenderColor(x, y, 1, height);
    RenderColor(x + width - 1, y, 1, height);
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(hovered || active ? 255 : 220, hovered || active ? 220 : 205, hovered || active ? 115 : 170, 255);
    g_pRenderText->RenderText(x, y + 5, text, width, 0, RT3_SORT_CENTER);
}

void CNewUIAdvancedStatsWindow::RenderDebug()
{
    const UiRect& rectangle = m_CloseRect;
    const int baseX = m_PanelPos.x;
    const int baseY = m_PanelPos.y;

    UI::Paris::Color(0.95f, 0.12f, 0.12f, 0.28f);
    RenderColor(baseX + rectangle.X, baseY + rectangle.Y, rectangle.Width, rectangle.Height);
    EndRenderColor();

    wchar_t debugText[192] = {};
    mu_swprintf(
        debugText,
        L"DEBUG CLOSE X=%d Y=%d W=%d H=%d | Arrows move | SHIFT resize",
        rectangle.X,
        rectangle.Y,
        rectangle.Width,
        rectangle.Height);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 180);
    g_pRenderText->SetTextColor(255, 100, 100, 255);
    g_pRenderText->RenderText(180, 5, debugText, 450, 0, RT3_SORT_LEFT);
}

bool CNewUIAdvancedStatsWindow::CheckCloseMouse() const
{
    return CheckMouseIn(
        m_PanelPos.x + m_CloseRect.X,
        m_PanelPos.y + m_CloseRect.Y,
        m_CloseRect.Width,
        m_CloseRect.Height);
}

bool CNewUIAdvancedStatsWindow::CheckPanelMouse() const
{
    return CheckMouseIn(m_PanelPos.x, m_PanelPos.y, WINDOW_WIDTH, WINDOW_HEIGHT);
}

bool CNewUIAdvancedStatsWindow::CheckScrollTrackMouse() const
{
    return CheckMouseIn(
        m_PanelPos.x + ScrollTrackX - 4,
        m_PanelPos.y + ScrollTrackY,
        ScrollTrackWidth + 8,
        ScrollTrackHeight);
}

bool CNewUIAdvancedStatsWindow::CheckScrollThumbMouse() const
{
    return CheckMouseIn(
        m_PanelPos.x + ScrollTrackX - 4,
        GetScrollThumbY(),
        ScrollTrackWidth + 8,
        GetScrollThumbHeight());
}

int CNewUIAdvancedStatsWindow::GetMaxScroll() const
{
    return std::max(0, m_ContentHeight - ContentHeight);
}

int CNewUIAdvancedStatsWindow::GetScrollThumbHeight() const
{
    return 26;
}

int CNewUIAdvancedStatsWindow::GetScrollThumbY() const
{
    const int trackY = m_PanelPos.y + ScrollTrackY;
    const int maxScroll = GetMaxScroll();
    const int movable = ScrollTrackHeight - GetScrollThumbHeight();

    if (maxScroll <= 0 || movable <= 0)
    {
        return trackY;
    }

    return trackY + ((m_ScrollOffset * movable) / maxScroll);
}

void CNewUIAdvancedStatsWindow::SetScrollFromThumb(int pointerY)
{
    const int trackY = m_PanelPos.y + ScrollTrackY;
    const int movable = ScrollTrackHeight - GetScrollThumbHeight();
    const int maxScroll = GetMaxScroll();

    if (movable <= 0 || maxScroll <= 0)
    {
        m_ScrollOffset = 0;
        return;
    }

    const int relative = std::clamp(pointerY - trackY, 0, movable);
    m_ScrollOffset = (relative * maxScroll) / movable;
    ClampScroll();
}

void CNewUIAdvancedStatsWindow::ClampScroll()
{
    m_ScrollOffset = std::clamp(m_ScrollOffset, 0, GetMaxScroll());
}

float CNewUIAdvancedStatsWindow::GetLayerDepth()
{
    return 5.2f;
}

float CNewUIAdvancedStatsWindow::GetKeyEventOrder()
{
    return 4.0f;
}
