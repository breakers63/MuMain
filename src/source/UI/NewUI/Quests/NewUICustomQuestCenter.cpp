#include "stdafx.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
// ETRO_QUEST_SIDE_PANEL_STAGE1_TEMP_FLOW_20260820
#include "UI/NewUI/Quests/NewUICustomQuestCenter.h"
#include "I18N/All.h"
namespace
{
    UI::Theme::ParisFrameAssets questPanelArt;
    // ETRO_CUSTOM_QUEST_LOCAL_I18N_V1
    bool EtroCustomQuestIsSpanish()
    {
        const char* locale = I18N::GetCurrentLocale();

        return locale != nullptr
            && locale[0] == 'e'
            && locale[1] == 's'
            && (locale[2] == '\0'
                || locale[2] == '-'
                || locale[2] == '_');
    }

    const wchar_t* EtroCustomQuestText(
        const wchar_t* spanish,
        const wchar_t* english)
    {
        return EtroCustomQuestIsSpanish()
            ? spanish
            : english;
    }
}


#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/NewUI/NewUISystem.h"

#include "UI/Legacy/UIManager.h"
#include "Network/Server/WSclient.h"
#include "World/MapInfra/MapManager.h"

#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

// ItemConvert is implemented in ZzzInfomation.cpp but is not declared by
// this MuMain branch's public header. Declare the existing global function
// here so the quest reward tooltip can reuse the native item conversion path.
void ItemConvert(ITEM* item, BYTE attribute1, BYTE attribute2, BYTE attribute3);

#if defined(_WIN32)
extern HWND g_hWnd;
extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
#endif

using namespace SEASON3B;

SEASON3B::CNewUICustomQuestCenter* g_pCustomQuestCenter = nullptr;

namespace
{
    bool IsNpcDialogBlockingQuestLauncher()
    {
        if (g_pUIPopup != nullptr && g_pUIPopup->GetPopupID() != 0)
        {
            return true;
        }
        constexpr DWORD npcWindows[] = {
            SEASON3B::INTERFACE_NPCQUEST, SEASON3B::INTERFACE_NPC_DIALOGUE,
            SEASON3B::INTERFACE_GOLD_BOWMAN, SEASON3B::INTERFACE_GOLD_BOWMAN_LENA,
            SEASON3B::INTERFACE_NPCBREEDER, SEASON3B::INTERFACE_DEVILSQUARE,
            SEASON3B::INTERFACE_BLOODCASTLE, SEASON3B::INTERFACE_NPCSHOP,
            SEASON3B::INTERFACE_CURSEDTEMPLE_NPC, SEASON3B::INTERFACE_MESSAGEBOX,
            SEASON3B::INTERFACE_QUEST_PROGRESS, SEASON3B::INTERFACE_QUEST_PROGRESS_ETC,
            SEASON3B::INTERFACE_DUELWATCH
        };
        for (DWORD window : npcWindows)
        {
            if (g_pNewUISystem->IsVisible(window)) return true;
        }
        return false;
    }

    bool IsQuestLauncherBlocked()
    {
        return g_pNewUISystem == nullptr
            || g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_PARTY)
            || g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MUHELPER)
            || g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MUHELPER_EXT)
            || g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MUHELPER_SKILL_LIST)
            || IsNpcDialogBlockingQuestLauncher();
    }

    void GetQuestLogicalViewportSize(int& width, int& height)
    {
        width = 640;
        height = 480;

#if defined(_WIN32)
        RECT clientRect = {};
        if (g_hWnd != nullptr && ::GetClientRect(g_hWnd, &clientRect))
        {
            const int clientWidth = std::max(
                1,
                static_cast<int>(clientRect.right - clientRect.left));
            const int clientHeight = std::max(
                1,
                static_cast<int>(clientRect.bottom - clientRect.top));
            const float scaleX = g_fScreenRate_x > 0.0f
                ? g_fScreenRate_x
                : 1.0f;
            const float scaleY = g_fScreenRate_y > 0.0f
                ? g_fScreenRate_y
                : 1.0f;

            width = std::max(
                640,
                static_cast<int>(
                    (static_cast<float>(clientWidth) / scaleX) + 0.5f));
            height = std::max(
                480,
                static_cast<int>(
                    (static_cast<float>(clientHeight) / scaleY) + 0.5f));
        }
#endif
    }

    int GetQuestLogicalWidth()
    {
        int width = 640;
        int height = 480;
        GetQuestLogicalViewportSize(width, height);
        return width;
    }

    int GetQuestLogicalHeight()
    {
        int width = 640;
        int height = 480;
        GetQuestLogicalViewportSize(width, height);
        return height;
    }

    void GetQuestPointerPosition(int& x, int& y)
    {
        x = MouseX;
        y = MouseY;

#if defined(_WIN32)
        POINT cursor = {};
        if (g_hWnd != nullptr
            && ::GetCursorPos(&cursor)
            && ::ScreenToClient(g_hWnd, &cursor))
        {
            const float scaleX = g_fScreenRate_x > 0.0f
                ? g_fScreenRate_x
                : 1.0f;
            const float scaleY = g_fScreenRate_y > 0.0f
                ? g_fScreenRate_y
                : 1.0f;

            x = static_cast<int>(
                (static_cast<float>(cursor.x) / scaleX) + 0.5f);
            y = static_cast<int>(
                (static_cast<float>(cursor.y) / scaleY) + 0.5f);
            x = std::clamp(x, 0, GetQuestLogicalWidth());
            y = std::clamp(y, 0, GetQuestLogicalHeight());
        }
#endif
    }

    bool QuestCheckMouseIn(int x, int y, int width, int height)
    {
        int pointerX = 0;
        int pointerY = 0;
        GetQuestPointerPosition(pointerX, pointerY);
        return pointerX >= x
            && pointerX < x + width
            && pointerY >= y
            && pointerY < y + height;
    }

    HFONT g_QuestBodyFont = nullptr;
    HFONT g_QuestHeadingFont = nullptr;
    int g_QuestFontClientWidth = 0;
    int g_QuestFontClientHeight = 0;

    void ReleaseQuestFonts()
    {
#if defined(_WIN32)
        if (g_QuestBodyFont != nullptr)
        {
            ::DeleteObject(g_QuestBodyFont);
            g_QuestBodyFont = nullptr;
        }

        if (g_QuestHeadingFont != nullptr)
        {
            ::DeleteObject(g_QuestHeadingFont);
            g_QuestHeadingFont = nullptr;
        }
#endif

        g_QuestFontClientWidth = 0;
        g_QuestFontClientHeight = 0;
    }

#if defined(_WIN32)
    HFONT CreateScaledQuestFont(
        HFONT sourceFont,
        int targetHeight,
        int fallbackWeight)
    {
        LOGFONT logFont = {};

        if (sourceFont == nullptr
            || ::GetObject(
                sourceFont,
                sizeof(LOGFONT),
                &logFont) != sizeof(LOGFONT))
        {
            logFont.lfHeight = targetHeight;
            logFont.lfWidth = 0;
            logFont.lfWeight = fallbackWeight;
            logFont.lfCharSet = DEFAULT_CHARSET;
            logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
            logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            logFont.lfQuality = NONANTIALIASED_QUALITY;
            logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
            ::lstrcpyn(logFont.lfFaceName, TEXT("Arial"), LF_FACESIZE);
        }
        else
        {
            // Preserve the sign convention of the source font. Some MuMain
            // branches create positive-height fonts while others use negative
            // character heights.
            logFont.lfHeight = logFont.lfHeight < 0
                ? -targetHeight
                : targetHeight;
            logFont.lfWidth = 0;
        }

        return ::CreateFontIndirect(&logFont);
    }
#endif

    void EnsureQuestFonts()
    {
#if defined(_WIN32)
        RECT clientRect = {};
        if (g_hWnd == nullptr
            || !::GetClientRect(g_hWnd, &clientRect))
        {
            return;
        }

        const int clientWidth = std::max(
            1,
            static_cast<int>(clientRect.right - clientRect.left));
        const int clientHeight = std::max(
            1,
            static_cast<int>(clientRect.bottom - clientRect.top));

        if (g_QuestBodyFont != nullptr
            && g_QuestHeadingFont != nullptr
            && g_QuestFontClientWidth == clientWidth
            && g_QuestFontClientHeight == clientHeight)
        {
            return;
        }

        ReleaseQuestFonts();

        // Recreate rasterized glyphs for the actual client resolution instead
        // of stretching letters baked into the panel image.
        const int targetHeight = std::clamp(
            ((16 * clientHeight) + 540) / 1080,
            10,
            24);

        g_QuestBodyFont = CreateScaledQuestFont(
            g_hFont,
            targetHeight,
            FW_NORMAL);
        g_QuestHeadingFont = CreateScaledQuestFont(
            g_hFontBold,
            targetHeight,
            FW_BOLD);

        g_QuestFontClientWidth = clientWidth;
        g_QuestFontClientHeight = clientHeight;
#endif
    }

    HFONT GetQuestBodyFont()
    {
        return g_hFont;
    }

    HFONT GetQuestHeadingFont()
    {
        return g_hFontBold;
    }

    constexpr BYTE CustomQuestHeadCode = 0xED;
    constexpr BYTE ProtocolVersion = 2;

    constexpr BYTE CatalogRequest = 0x00;
    constexpr BYTE AcceptRequest = 0x01;
    constexpr BYTE ClaimRequest = 0x02;

    constexpr BYTE CatalogBegin = 0x10;
    constexpr BYTE QuestDefinition = 0x11;
    constexpr BYTE CatalogEnd = 0x12;
    constexpr BYTE ProgressUpdate = 0x13;
    constexpr BYTE ActionResult = 0x14;

    constexpr int QuestDefinitionPacketLength = 669;
    constexpr int ProgressPacketLength = 22;
    constexpr int ActionResultPacketLength = 24;

    constexpr int LayoutElementCount = static_cast<int>(CustomQuestUiElement::Count);

    constexpr int QuestRowFirstElement = static_cast<int>(CustomQuestUiElement::QuestRow1);
    constexpr int QuestTextFirstElement = static_cast<int>(CustomQuestUiElement::QuestText1);
    constexpr int RewardSlotFirstElement = static_cast<int>(CustomQuestUiElement::RewardSlot1);

    constexpr int DebugOverlayX = 6;
    constexpr int DebugOverlayY = 386;
    constexpr int DebugOverlayWidth = 500;
    constexpr int DebugOverlayHeight = 88;

    constexpr int DefaultPanelScalePercent = 170;
    constexpr int MinimumPanelScalePercent = 60;
    constexpr int MaximumPanelScalePercent = 180;
    constexpr int PanelScaleStepPercent = 5;
    constexpr bool EnableQuestUiDebug = false;
    // FINAL BUILD: layout values are baked below. Calibration hotkeys/overlays are disabled.
    constexpr bool EnableQuestCalibrationDebug = false;
    constexpr bool MiniMapButtonFocusedDebug = false;
    constexpr BYTE ItemRewardType = 6;
    constexpr BYTE RewardItemFlagSkill = 0x01;
    constexpr BYTE RewardItemFlagLuck = 0x02;

    // Category 0 is the progressive chain in the current Custom Quest admin
    // configuration. Only one unresolved progressive mission is exposed at a
    // time; non-progressive categories keep their normal list behavior.
    constexpr BYTE ProgressiveQuestCategory = 0;

    // Temporary side-panel layout. These are intentionally code-drawn
    // placeholders so the final texture artwork can be integrated later
    // without changing the quest protocol or server logic.
    constexpr int TempPanelWidth = 220;
    constexpr int TempPanelHeight = 360;
    constexpr int TempPanelTop = 58;
    constexpr int TempPanelRightGap = 38;
    constexpr int TempLauncherWidth = 28;
    constexpr int TempLauncherHeight = 28;
    // Compact tracker: the closed tab stays unobtrusive in the lower-right
    // corner. Clicking it expands only the progress box upward; it no longer
    // opens the large quest panel.
    constexpr int TempTrackerWidth = 66;
    constexpr int TempTrackerHeight = 14;
    constexpr int TempTrackerExpandedWidth = 128;
    constexpr int TempTrackerExpandedHeight = 36;
    constexpr int TempTrackerExpandedGap = 2;
    constexpr int TempTrackerBottomGap = 56;
    constexpr int TempQuestRowHeight = 24;
    constexpr int TempQuestRowGap = 3;
    constexpr int TempExpandedBaseHeight = 76;
    constexpr int TempRewardLineHeight = 15;

    // Final texture names prepared for the designer. Physical runtime files:
    // Data\Interface\QuestSystem\QuestButton.OZT
    // Data\Interface\QuestSystem\QuestPanel.OZT
    // Data\Interface\QuestSystem\QuestTracker.OZT
    // Data\Interface\QuestSystem\QuestTrackerExpanded.OZT
    constexpr const wchar_t* QuestButtonTexturePath = L"Data\\Interface\\Paris_Quest_Scroll.tga";
    constexpr const wchar_t* QuestPanelTexturePath = L"Data\\Interface\\QuestSystem\\QuestPanel.tga";
    constexpr const wchar_t* QuestTrackerTexturePath = L"Data\\Interface\\QuestSystem\\QuestTracker.tga";
    constexpr const wchar_t* QuestTrackerExpandedTexturePath = L"Data\\Interface\\QuestSystem\\QuestTrackerExpanded.tga";

    struct QuestRewardModelTuning
    {
        float Width;
        float Height;
        float OffsetX;
        float OffsetY;
    };

    QuestRewardModelTuning GetQuestRewardModelTuning(BYTE itemGroup)
    {
        switch (itemGroup)
        {
        case 0: // Swords
            // Long sword models are visually biased to the right by the
            // native renderer, so compensate without moving the tooltip.
            return { 88.0f, 92.0f, -22.0f, 2.0f };

        case 1: // Axes
        case 2: // Maces
        case 3: // Spears
        case 4: // Bows
        case 5: // Staffs
            return { 88.0f, 92.0f, -14.0f, 2.0f };

        case 6: // Shields
            return { 86.0f, 86.0f, 0.0f, 6.0f };

        case 7: // Helms
            return { 76.0f, 80.0f, 0.0f, 7.0f };

        case 8: // Armors
            return { 82.0f, 88.0f, 0.0f, 4.0f };

        case 9: // Pants
            return { 78.0f, 84.0f, 0.0f, 6.0f };

        case 10: // Gloves
        case 11: // Boots
            return { 74.0f, 78.0f, 0.0f, 8.0f };

        case 12: // Wings and helpers
            return { 96.0f, 88.0f, 0.0f, 4.0f };

        default:
            return { 78.0f, 82.0f, 0.0f, 6.0f };
        }
    }

    bool BuildQuestRewardItem(
        const CustomQuestRewardView& reward,
        ITEM& item)
    {
        if (reward.Type != ItemRewardType
            || reward.ItemGroup > 15
            || reward.ItemNumber >= MAX_ITEM_INDEX)
        {
            return false;
        }

        const int itemType =
            (static_cast<int>(reward.ItemGroup) * MAX_ITEM_INDEX)
            + static_cast<int>(reward.ItemNumber);

        if (itemType < 0)
        {
            return false;
        }

        ZeroMemory(&item, sizeof(item));
        item.Type = static_cast<short>(itemType);

        const BYTE safeLevel =
            static_cast<BYTE>(std::min<int>(reward.ItemLevel, 15));
        const BYTE safeOption =
            static_cast<BYTE>(std::min<int>(reward.OptionLevel, 7));

        BYTE attribute1 =
            static_cast<BYTE>((safeLevel << 3) | (safeOption & 0x03));

        if ((reward.ItemFlags & RewardItemFlagSkill) != 0)
        {
            attribute1 = static_cast<BYTE>(attribute1 | 0x80);
        }

        if ((reward.ItemFlags & RewardItemFlagLuck) != 0)
        {
            attribute1 = static_cast<BYTE>(attribute1 | 0x04);
        }

        BYTE attribute2 =
            static_cast<BYTE>(reward.ExcellentMask & 0x3F);

        if ((safeOption & 0x04) != 0)
        {
            attribute2 = static_cast<BYTE>(attribute2 | 0x40);
        }

        const BYTE attribute3 = static_cast<BYTE>(
            (reward.AncientDiscriminator & 0x03)
            | ((reward.AncientBonusLevel & 0x0F) << 2));

        // Reuse the native item conversion path. This builds the same skill,
        // luck, normal, Excellent and Ancient option data used by inventory
        // and Post Item tooltips.
        ItemConvert(&item, attribute1, attribute2, attribute3);

        item.Level = safeLevel;
        item.ExcellentFlags =
            static_cast<BYTE>(reward.ExcellentMask & 0x3F);
        item.AncientDiscriminator =
            static_cast<BYTE>(reward.AncientDiscriminator & 0x03);
        item.Durability = reward.Durability;
        item.Key = static_cast<DWORD>(-1);
        return true;
    }
}

CNewUICustomQuestCenter::CNewUICustomQuestCenter()
    : m_Manager(nullptr),
      m_WindowOpen(false),
      m_Dragging(false),
      m_DragOffsetX(0),
      m_DragOffsetY(0),
      m_Page(0),
      m_SelectedIndex(-1),
      m_RewardsExpanded(false),
      m_TrackedQuestId(0),
      m_TrackerExpanded(false),
      m_AutoCatalogRequested(false),
      m_LoadingCatalog(false),
      m_ExpectedQuestCount(0),
      m_LastCatalogRequestTick(0),
      m_PanelScalePercent(DefaultPanelScalePercent),
      m_InteractionDebugEnabled(false),
      m_InteractionDebugIndex(InteractionQuestRowHover),
      m_TextDebugEnabled(false),
      m_TextDebugIndex(TextPanelTitle),
      m_DebugEnabled(false),
      m_DebugElementIndex(0)
{
    m_Pos.x = 640 - GetPanelWidth() - 12;
    m_Pos.y = 38;
    m_StatusText[0] = L'\0';
    for (int index = 0; index < TextureCount; ++index)
    {
        m_TextureIds[index] = BITMAP_UNKNOWN;
        m_TextureLoaded[index] = false;
    }
    m_TextureDebugEnabled = false;
    m_TextureDebugIndex = TextureQuestButton;
    InitializeTextureDebugLayout();
    InitializeInteractionDebugLayout();
    InitializeTextDebugLayout();
    InitializeDebugLayout();
}

CNewUICustomQuestCenter::~CNewUICustomQuestCenter()
{
    Release();
}

bool CNewUICustomQuestCenter::Create(CNewUIManager* manager, int x, int y)
{
    if (manager == nullptr)
    {
        return false;
    }

    m_Manager = manager;
    m_Manager->AddUIObj(CUSTOM_QUEST_CENTER_INTERFACE_KEY, this);
    (void)x;
    (void)y;
    SetPos(640 - GetPanelWidth() - 12, 38);
    // Recalculate the four final-art rectangles after the window exists, so
    // widescreen logical dimensions are used instead of an early 640x480 fallback.
    InitializeTextureDebugLayout();
    InitializeInteractionDebugLayout();
    LoadImages();
    Show(true);
    Enable(true);
    g_pCustomQuestCenter = this;
    return true;
}

void CNewUICustomQuestCenter::Release()
{
    ReleaseQuestFonts();

    if (g_pCustomQuestCenter == this)
    {
        g_pCustomQuestCenter = nullptr;
    }

    UnloadImages();

    if (m_Manager != nullptr)
    {
        m_Manager->RemoveUIObj(this);
        m_Manager = nullptr;
    }

    m_Quests.clear();
}

void CNewUICustomQuestCenter::SetPos(int x, int y)
{
    // Keep the panel recoverable, but allow it to cross the classic
    // right-side limit. This is required because the visible widescreen
    // extension can exist beyond the 640-wide logical interaction region.
    // Leaving at least one third visible preserves enough title bar to drag
    // the panel back on both 4:3 and widescreen resolutions.
    const int minimumVisibleWidth = std::max(
        80,
        GetPanelWidth() / 3);
    const int maximumX = std::max(
        0,
        GetQuestLogicalWidth() - minimumVisibleWidth);

    // The panel is taller than half of the 480 reference viewport. Requiring
    // it to remain fully visible limited its top edge to about y=93. Keep at
    // least half of it visible instead, so it can be moved into the lower
    // half without becoming impossible to recover.
    const int minimumVisibleHeight = std::max(40, GetPanelHeight() / 2);
    const int maximumY = std::max(
        0,
        GetQuestLogicalHeight() - minimumVisibleHeight);

    m_Pos.x = std::clamp(x, 0, maximumX);
    m_Pos.y = std::clamp(y, 0, maximumY);
}

void CNewUICustomQuestCenter::ToggleWindow()
{
    if (m_WindowOpen)
    {
        CloseWindow();
    }
    else
    {
        OpenWindow();
    }
}

void CNewUICustomQuestCenter::OpenWindow()
{
    m_WindowOpen = true;
    RequestCatalog();
    ::PlayBuffer(SOUND_CLICK01);
}

void CNewUICustomQuestCenter::CloseWindow()
{
    m_WindowOpen = false;
    m_Dragging = false;
    ::PlayBuffer(SOUND_CLICK01);
}

bool CNewUICustomQuestCenter::IsWindowOpen() const
{
    return m_WindowOpen;
}

bool CNewUICustomQuestCenter::UpdateMouseEvent()
{
    if (SceneFlag != MAIN_SCENE)
    {
        m_WindowOpen = false;
        m_TextureDebugEnabled = false;
        m_InteractionDebugEnabled = false;
        m_TextDebugEnabled = false;
        return true;
    }

    if (m_TextureDebugEnabled || m_InteractionDebugEnabled || m_TextDebugEnabled)
    {
        // Calibration is keyboard-only; suppress normal quest clicks while
        // artwork or text anchors are being adjusted.
        return false;
    }

    // The small launcher remains interactive even while the panel is closed.
    if (ProcessLauncherButton())
    {
        UI::Paris::ConsumeMouse();
        return false;
    }

    // PARIS_CUSTOM_QUEST_TRACKER_CLICK_DISABLED_V1
    // El tracker ya no existe visualmente, por lo que tampoco intercepta clicks.

    if (!m_WindowOpen)
    {
        return true;
    }

    if (ProcessWindowButtons()
        || ProcessPageControls()
        || ProcessQuestSelection()
        || ProcessRewardsToggle()
        || ProcessActionButton())
    {
        UI::Paris::ConsumeMouse();
        return false;
    }

    const CustomQuestUiRect panel = GetTemporaryPanelRect();
    if (QuestCheckMouseIn(panel.X, panel.Y, panel.Width, panel.Height))
    {
        UI::Paris::ConsumeMouse();
        return false;
    }
    return true;
}

bool CNewUICustomQuestCenter::UpdateKeyEvent()
{
    if (SceneFlag != MAIN_SCENE)
    {
        m_WindowOpen = false;
        return true;
    }

    if (EnableQuestCalibrationDebug && ProcessInteractionDebugKeyEvent())
    {
        return false;
    }

    if (EnableQuestCalibrationDebug && ProcessTextDebugKeyEvent())
    {
        return false;
    }

    if (EnableQuestCalibrationDebug && ProcessTextureDebugKeyEvent())
    {
        return false;
    }

    if (EnableQuestUiDebug && ProcessDebugKeyEvent())
    {
        return false;
    }

    if (SEASON3B::IsPress(VK_F7))
    {
        ToggleWindow();
        return false;
    }

    if (m_WindowOpen && SEASON3B::IsPress(VK_ESCAPE))
    {
        CloseWindow();
        return false;
    }

    return true;
}

bool CNewUICustomQuestCenter::Update()
{
    if (SceneFlag != MAIN_SCENE)
    {
        m_WindowOpen = false;
        m_TextureDebugEnabled = false;
        m_InteractionDebugEnabled = false;
        m_TextDebugEnabled = false;
        m_RewardsExpanded = false;
        m_TrackerExpanded = false;
        if (m_AutoCatalogRequested || !m_Quests.empty())
        {
            ClearCatalog();
            m_TrackedQuestId = 0;
        }

        m_AutoCatalogRequested = false;
        return true;
    }

    if (SocketClient != nullptr && SocketClient->IsConnected())
    {
        if (!m_AutoCatalogRequested)
        {
            RequestCatalog();
            m_AutoCatalogRequested = m_LoadingCatalog;
        }
    }
    else
    {
        m_AutoCatalogRequested = false;
    }

    return true;
}

bool CNewUICustomQuestCenter::Render()
{
    if (SceneFlag != MAIN_SCENE)
    {
        m_WindowOpen = false;
        return true;
    }

    EnableAlphaTest();
    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);

    // Temporary code-drawn launcher and tracker remain visible independently
    // from the large quest panel.
    RenderLauncherButton();
    // PARIS_CUSTOM_QUEST_TRACKER_RENDER_DISABLED_V1
    // Tracker inferior derecho retirado; la Q y la ventana completa se conservan.

    if (m_WindowOpen)
    {
        RenderWindow();
    }

    if (EnableQuestCalibrationDebug && m_TextureDebugEnabled)
    {
        RenderTextureDebugOverlay();
    }

    if (EnableQuestCalibrationDebug && m_InteractionDebugEnabled)
    {
        RenderInteractionDebugOverlay();
    }

    if (EnableQuestCalibrationDebug && m_TextDebugEnabled)
    {
        RenderTextDebugOverlay();
    }

    if (EnableQuestUiDebug && m_DebugEnabled)
    {
        RenderDebugOverlay();
    }

    DisableAlphaBlend();
    return true;
}

float CNewUICustomQuestCenter::GetLayerDepth()
{
    return 3.65f;
}

void CNewUICustomQuestCenter::ReceivePacket(const BYTE* packet, int size)
{
    if (packet == nullptr || size < 4)
    {
        return;
    }

    const bool c1 = (packet[0] == 0xC1 || packet[0] == 0xC3);
    const bool c2 = (packet[0] == 0xC2 || packet[0] == 0xC4);
    if (!c1 && !c2)
    {
        return;
    }

    const int headOffset = c1 ? 2 : 3;
    const int subCodeOffset = c1 ? 3 : 4;
    if (size <= subCodeOffset || packet[headOffset] != CustomQuestHeadCode)
    {
        return;
    }

    switch (packet[subCodeOffset])
    {
    case CatalogBegin:
        ++m_CatalogRevision;
        ReceiveCatalogBegin(packet, size);
        break;
    case QuestDefinition:
        ++m_CatalogRevision;
        ReceiveQuestDefinition(packet, size);
        break;
    case CatalogEnd:
        ++m_CatalogRevision;
        ReceiveCatalogEnd(packet, size);
        break;
    case ProgressUpdate:
        ++m_CatalogRevision;
        ReceiveProgressUpdate(packet, size);
        break;
    case ActionResult:
        ReceiveActionResult(packet, size);
        break;
    default:
        break;
    }
}

void CNewUICustomQuestCenter::ClearCatalog()
{
    m_Quests.clear();
    m_Page = 0;
    m_SelectedIndex = -1;
    m_RewardsExpanded = false;
    m_ExpectedQuestCount = 0;
    m_LoadingCatalog = false;
}

void CNewUICustomQuestCenter::RequestCatalog()
{
    const DWORD now = GetTickCount();
    if (now - m_LastCatalogRequestTick < 500)
    {
        return;
    }

    m_LastCatalogRequestTick = now;

    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        SetStatus(EtroCustomQuestText(L"No hay conexion con el GameServer", L"No connection to GameServer"));
        return;
    }

    const char* locale = I18N::GetCurrentLocale();

    // Custom Quest language:
    // 0 = English / fallback
    // 1 = Spanish
    const BYTE language =
        locale != nullptr
        && locale[0] == 'e'
        && locale[1] == 's'
        && (locale[2] == '\0' || locale[2] == '-' || locale[2] == '_')
            ? 1
            : 0;

    BYTE packet[5] = { 0xC1, 5, CustomQuestHeadCode, CatalogRequest, language };
    SocketClient->Send(packet, sizeof(packet));
    m_LoadingCatalog = true;
    SetStatus(EtroCustomQuestText(L"Cargando misiones...", L"Loading quests..."));
}

void CNewUICustomQuestCenter::SendAcceptRequest(std::int32_t questId)
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        SetStatus(EtroCustomQuestText(L"No hay conexion con el GameServer", L"No connection to GameServer"));
        return;
    }

    BYTE packet[8] = { 0xC1, 8, CustomQuestHeadCode, AcceptRequest, 0, 0, 0, 0 };
    WriteInt32(packet + 4, questId);
    SocketClient->Send(packet, sizeof(packet));
    SetStatus(EtroCustomQuestText(L"Solicitud de aceptacion enviada", L"Accept request sent"));
}

void CNewUICustomQuestCenter::SendClaimRequest(std::int32_t questId)
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        SetStatus(EtroCustomQuestText(L"No hay conexion con el GameServer", L"No connection to GameServer"));
        return;
    }

    BYTE packet[8] = { 0xC1, 8, CustomQuestHeadCode, ClaimRequest, 0, 0, 0, 0 };
    WriteInt32(packet + 4, questId);
    SocketClient->Send(packet, sizeof(packet));
    SetStatus(EtroCustomQuestText(L"Solicitud de recompensa enviada", L"Reward request sent"));
}

void CNewUICustomQuestCenter::LoadImages()
{
    questPanelArt.Load();
    // ETRO QUEST STAGE 4.2: dynamic non-named texture IDs.
    // Fixed IDs (previously 31910..31913) can collide with textures owned by
    // Advanced Stats or another custom UI. Bitmaps.LoadImage(filename) asks the
    // global bitmap manager for a free non-named ID, so each resource owns its
    // actual runtime ID without overwriting an existing interface texture.
    const wchar_t* paths[TextureCount] =
    {
        QuestButtonTexturePath,
        QuestPanelTexturePath,
        QuestTrackerTexturePath,
        QuestTrackerExpandedTexturePath,
    };

    for (int index = 0; index < TextureCount; ++index)
    {
        if (m_TextureIds[index] != BITMAP_UNKNOWN)
        {
            DeleteBitmap(m_TextureIds[index]);
            m_TextureIds[index] = BITMAP_UNKNOWN;
        }

        m_TextureIds[index] = Bitmaps.LoadImage(
            paths[index],
            GL_LINEAR,
            GL_CLAMP_TO_EDGE);
        m_TextureLoaded[index] = m_TextureIds[index] != BITMAP_UNKNOWN;
    }
}

void CNewUICustomQuestCenter::UnloadImages()
{
    questPanelArt.Unload();
    for (int index = 0; index < TextureCount; ++index)
    {
        if (m_TextureIds[index] != BITMAP_UNKNOWN)
        {
            DeleteBitmap(m_TextureIds[index]);
            m_TextureIds[index] = BITMAP_UNKNOWN;
        }

        m_TextureLoaded[index] = false;
    }
}

void CNewUICustomQuestCenter::ReceiveCatalogBegin(const BYTE* packet, int size)
{
    if (size < 8 || packet[4] != ProtocolVersion)
    {
        return;
    }

    ClearCatalog();
    m_LoadingCatalog = true;
    m_ExpectedQuestCount = ReadUInt16(packet + 5);
    m_Quests.reserve(m_ExpectedQuestCount);
    SetStatus(EtroCustomQuestText(L"Recibiendo catalogo...", L"Receiving catalog..."));
}

void CNewUICustomQuestCenter::ReceiveQuestDefinition(const BYTE* packet, int size)
{
    if (size < QuestDefinitionPacketLength || packet[0] != 0xC2)
    {
        return;
    }

    const BYTE* payload = packet + 5;
    if (payload[0] != ProtocolVersion)
    {
        return;
    }

    CustomQuestDefinitionView quest;
    quest.Id = ReadInt32(payload + 1);
    quest.Flags = payload[5];
    quest.Category = payload[6];
    quest.State = static_cast<CustomQuestClientState>(payload[7]);
    quest.SortOrder = ReadUInt16(payload + 8);
    quest.MinimumLevel = ReadUInt16(payload + 10);
    quest.MaximumLevel = ReadUInt16(payload + 12);
    quest.CurrentKills = ReadUInt32(payload + 14);
    quest.RequiredKills = ReadUInt32(payload + 18);
    quest.MonsterNumber = ReadUInt16(payload + 22);
    quest.RemainingCooldownSeconds = ReadUInt32(payload + 24);
    quest.RewardCount = static_cast<BYTE>(std::min<int>(payload[28], MAX_REWARDS));

    CopyFixedUtf8(quest.Name, 65, payload + 29, 64);
    CopyFixedUtf8(quest.Description, 193, payload + 93, 192);
    CopyFixedUtf8(quest.ObjectiveName, 65, payload + 285, 64);

    int rewardOffset = 349;
    for (int i = 0; i < quest.RewardCount; ++i)
    {
        const BYTE* reward = payload + rewardOffset;
        quest.Rewards[i].Type = reward[0];
        quest.Rewards[i].Amount = ReadUInt32(reward + 1);
        quest.Rewards[i].ItemGroup = reward[5];
        quest.Rewards[i].ItemNumber = ReadUInt16(reward + 6);
        quest.Rewards[i].ItemLevel = reward[8];
        quest.Rewards[i].Durability = reward[9];
        quest.Rewards[i].OptionLevel = reward[10];
        quest.Rewards[i].ExcellentMask = reward[11];
        quest.Rewards[i].AncientDiscriminator = reward[12];
        quest.Rewards[i].AncientBonusLevel = reward[13];
        quest.Rewards[i].ItemFlags = reward[14];
        CopyFixedUtf8(quest.Rewards[i].DisplayName, 64, reward + 15, 48);
        rewardOffset += 63;
    }

    auto existing = FindQuest(quest.Id);
    if (existing != nullptr)
    {
        *existing = quest;
    }
    else
    {
        m_Quests.push_back(quest);
    }
}

void CNewUICustomQuestCenter::ReceiveCatalogEnd(const BYTE* packet, int size)
{
    if (size < 5 || packet[4] != ProtocolVersion)
    {
        return;
    }

    std::stable_sort(
        m_Quests.begin(),
        m_Quests.end(),
        [](const CustomQuestDefinitionView& left, const CustomQuestDefinitionView& right)
        {
            if (left.SortOrder != right.SortOrder)
            {
                return left.SortOrder < right.SortOrder;
            }

            return left.Id < right.Id;
        });

    m_LoadingCatalog = false;
    m_Page = 0;
    m_SelectedIndex = -1;
    m_RewardsExpanded = false;

    // Preserve a valid tracked quest across catalog refreshes. If the tracked
    // quest no longer exists or is no longer active, select the first active
    // or ready-to-claim mission as the tracker source.
    const CustomQuestDefinitionView* tracked = GetTrackedQuest();
    if (tracked == nullptr)
    {
        m_TrackedQuestId = 0;
        for (const auto& quest : m_Quests)
        {
            if (quest.State == CustomQuestClientState::Active
                || quest.State == CustomQuestClientState::ReadyToClaim)
            {
                m_TrackedQuestId = quest.Id;
                break;
            }
        }
    }

    std::vector<int> visibleQuestIndices;
    BuildVisibleQuestIndices(visibleQuestIndices);

    if (visibleQuestIndices.empty())
    {
        SetStatus(EtroCustomQuestText(L"No hay misiones activas", L"No active quests"));
    }
    else
    {
        wchar_t text[96];
        mu_swprintf(
            text,
            visibleQuestIndices.size() == 1
                ? EtroCustomQuestText(L"1 mision disponible", L"1 quest available")
                : EtroCustomQuestText(L"%d misiones disponibles", L"%d quests available"),
            static_cast<int>(visibleQuestIndices.size()));
        SetStatus(text);
    }

    // Progressive missions are accepted by the client as soon as the server
    // exposes the next step as Available. Claiming remains explicit so a full
    // inventory cannot silently consume/fail a reward. After a successful
    // claim the refreshed catalog advances to the next SortOrder and this
    // method auto-accepts it.
    AutoAcceptCurrentProgressiveQuest();
}

void CNewUICustomQuestCenter::ReceiveProgressUpdate(const BYTE* packet, int size)
{
    if (size < ProgressPacketLength || packet[4] != ProtocolVersion)
    {
        return;
    }

    const std::int32_t questId = ReadInt32(packet + 5);
    CustomQuestDefinitionView* quest = FindQuest(questId);
    if (quest == nullptr)
    {
        RequestCatalog();
        return;
    }

    quest->State = static_cast<CustomQuestClientState>(packet[9]);
    quest->CurrentKills = ReadUInt32(packet + 10);
    quest->RequiredKills = ReadUInt32(packet + 14);
    quest->RemainingCooldownSeconds = ReadUInt32(packet + 18);

    if (quest->State == CustomQuestClientState::Active
        || quest->State == CustomQuestClientState::ReadyToClaim)
    {
        m_TrackedQuestId = quest->Id;
    }
    else if (m_TrackedQuestId == quest->Id)
    {
        m_TrackedQuestId = 0;
    }
}

void CNewUICustomQuestCenter::ReceiveActionResult(const BYTE* packet, int size)
{
    if (size < ActionResultPacketLength || packet[4] != ProtocolVersion)
    {
        return;
    }

    const BYTE action = packet[5];
    const BYTE result = packet[6];
    const std::int32_t questId = ReadInt32(packet + 8);
    CustomQuestDefinitionView* quest = FindQuest(questId);
    if (quest != nullptr)
    {
        quest->State = static_cast<CustomQuestClientState>(packet[7]);
        quest->CurrentKills = ReadUInt32(packet + 12);
        quest->RequiredKills = ReadUInt32(packet + 16);
        quest->RemainingCooldownSeconds = ReadUInt32(packet + 20);

        if (quest->State == CustomQuestClientState::Active
            || quest->State == CustomQuestClientState::ReadyToClaim)
        {
            m_TrackedQuestId = quest->Id;
        }
        else if (m_TrackedQuestId == quest->Id)
        {
            m_TrackedQuestId = 0;
        }
    }

    if (result == 0)
    {
        SetStatus(action == 1
            ? EtroCustomQuestText(L"Mision aceptada correctamente", L"Quest accepted successfully")
            : EtroCustomQuestText(L"Recompensa entregada correctamente", L"Reward claimed successfully"));
    }
    else
    {
        wchar_t text[160];
        mu_swprintf(text, EtroCustomQuestText(L"Operacion rechazada: %ls", L"Operation rejected: %ls"), GetActionResultText(result));
        SetStatus(text);
    }

    if (result == 0)
    {
        RequestCatalog();
    }
}

void CNewUICustomQuestCenter::RenderLauncherButton()
{
    if (m_WindowOpen || IsQuestLauncherBlocked()) return;
    const CustomQuestUiRect button = GetTemporaryLauncherRect();
    const CustomQuestUiRect hoverRectangle = GetQuestButtonHoverRect();
    const bool hover = QuestCheckMouseIn(
        hoverRectangle.X,
        hoverRectangle.Y,
        hoverRectangle.Width,
        hoverRectangle.Height);
    const bool pressed = hover
        && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    const int side=std::min(button.Width,button.Height);
    const int x=button.X+(button.Width-side)/2,y=button.Y+(button.Height-side)/2;
    if(m_TextureLoaded[TextureQuestButton])
    {
        EnableAlphaTest();UI::Paris::Color(1.f,1.f,1.f,1.f);
        RenderBitmap(m_TextureIds[TextureQuestButton],x,y,side,side,0.f,0.f,1.f,1.f);
    }
    else UI::Character::ParisStyle::Label(x,y+side/3,side,L"?");
    UI::Theme::HoverTip(hoverRectangle.X,hoverRectangle.Y,hoverRectangle.Width,hoverRectangle.Height,L"Quest Center (F7)");

}

void CNewUICustomQuestCenter::RenderQuestTracker()
{
    const CustomQuestDefinitionView* quest = GetTrackedQuest();
    if (quest == nullptr)
    {
        m_TrackerExpanded = false;
        return;
    }

    const CustomQuestUiRect tracker = GetTemporaryTrackerRect();
    const bool hover = QuestCheckMouseIn(
        tracker.X,
        tracker.Y,
        tracker.Width,
        tracker.Height);

    if (m_TextureLoaded[TextureQuestTracker])
    {
        UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
        RenderBitmap(
            m_TextureIds[TextureQuestTracker],
            static_cast<float>(tracker.X),
            static_cast<float>(tracker.Y),
            static_cast<float>(tracker.Width),
            static_cast<float>(tracker.Height),
            0.0f,
            0.0f,
            1.0f,
            1.0f);
    }
    else
    {
        EnableAlphaTest();
        UI::Paris::Color(0.0f, 0.0f, 0.0f, hover ? 0.68f : 0.52f);
        RenderColor(tracker.X, tracker.Y, tracker.Width, tracker.Height);
        EndRenderColor();

        UI::Paris::Color(0.70f, 0.58f, 0.22f, hover ? 0.78f : 0.48f);
        RenderColor(tracker.X, tracker.Y, tracker.Width, 1.0f);
        RenderColor(
            tracker.X,
            tracker.Y + tracker.Height - 1,
            tracker.Width,
            1.0f);
        EndRenderColor();
    }

    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(
        hover ? 255 : 220,
        hover ? 226 : 210,
        hover ? 145 : 185,
        255);
    g_pRenderText->RenderText(
        tracker.X + 3 + GetTextOffsetX(TextTrackerLabel),
        tracker.Y + 2 + GetTextOffsetY(TextTrackerLabel),
        m_TrackerExpanded ? L"Seguimiento  -" : L"Seguimiento  +",
        tracker.Width - 6,
        0,
        RT3_SORT_CENTER);

    if (m_TrackerExpanded)
    {
        const CustomQuestUiRect expanded = GetTemporaryTrackerExpandedRect();

        if (m_TextureLoaded[TextureQuestTrackerExpanded])
        {
            UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
            RenderBitmap(
                m_TextureIds[TextureQuestTrackerExpanded],
                static_cast<float>(expanded.X),
                static_cast<float>(expanded.Y),
                static_cast<float>(expanded.Width),
                static_cast<float>(expanded.Height),
                0.0f,
                0.0f,
                1.0f,
                1.0f);
        }
        else
        {
            EnableAlphaTest();
            UI::Paris::Color(0.0f, 0.0f, 0.0f, 0.62f);
            RenderColor(
                expanded.X,
                expanded.Y,
                expanded.Width,
                expanded.Height);
            EndRenderColor();

            UI::Paris::Color(0.62f, 0.62f, 0.62f, 0.48f);
            RenderColor(expanded.X, expanded.Y, expanded.Width, 1.0f);
            RenderColor(
                expanded.X,
                expanded.Y + expanded.Height - 1,
                expanded.Width,
                1.0f);
            EndRenderColor();
        }

        wchar_t progress[160] = {};
        mu_swprintf(
            progress,
            L"%ls: %u / %u",
            quest->ObjectiveName,
            quest->CurrentKills,
            quest->RequiredKills);

        if (quest->State == CustomQuestClientState::ReadyToClaim)
        {
            g_pRenderText->SetTextColor(110, 255, 145, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(235, 235, 235, 255);
        }

        g_pRenderText->SetFont(GetQuestHeadingFont());
        g_pRenderText->RenderText(
            expanded.X + 5 + GetTextOffsetX(TextTrackerProgress),
            expanded.Y + 10 + GetTextOffsetY(TextTrackerProgress),
            progress,
            expanded.Width - 10,
            0,
            RT3_SORT_CENTER);
    }

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderWindow()
{
    const CustomQuestUiRect panel = GetTemporaryPanelRect();

    questPanelArt.Frame(panel.X, panel.Y, panel.Width, panel.Height);
    UI::Character::ParisStyle::Solid(panel.X + 12, panel.Y + 28,
        panel.Width - 24, 1, .65f, .49f, .17f);
    if (m_TextureLoaded[TextureQuestButton])
    {
        constexpr int emblemSize = 20;
        EnableAlphaTest();
        UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
        RenderBitmap(m_TextureIds[TextureQuestButton], panel.X + 10, panel.Y + 6,
            emblemSize, emblemSize, 0.f, 0.f, 1.f, 1.f);
    }

    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetTextColor(245, 220, 150, 255);
    g_pRenderText->RenderText(
        panel.X + 30 + GetTextOffsetX(TextPanelTitle),
        panel.Y + 12 + GetTextOffsetY(TextPanelTitle),
        EtroCustomQuestText(L"Panel de misiones", L"Quest panel"),
        panel.Width - 57,
        0,
        RT3_SORT_LEFT);

    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(185, 185, 185, 255);
    g_pRenderText->RenderText(
        panel.X + 8 + GetTextOffsetX(TextPanelSubtitle),
        panel.Y + 29 + GetTextOffsetY(TextPanelSubtitle),
        EtroCustomQuestText(L"", L""),
        panel.Width - 16,
        0,
        RT3_SORT_LEFT);

    const CustomQuestUiRect close = GetTemporaryCloseRect();
    const bool closeHover = QuestCheckMouseIn(
        close.X,
        close.Y,
        close.Width,
        close.Height);
    UI::Theme::Bevel(close.X, close.Y, close.Width, close.Height, L"X", closeHover);
    UI::Theme::HoverTip(close.X, close.Y, close.Width, close.Height, L"Close (F7 / Esc)");

    RenderQuestButtons();
    RenderQuestDetails();
    RenderPageControls();
    RenderActionButton();
    RenderStatusText();
    RenderRewardSlotHover();
    RenderHoverPreviewPanel();

    // Native reward information remains above the temporary side panel.
    RenderRewardItemTooltip();

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderPanelLabels()
{
    const CustomQuestUiRect& questTitle =
        GetLayout(CustomQuestUiElement::QuestTitle);
    const CustomQuestUiRect& miniMapLabel =
        GetLayout(CustomQuestUiElement::MiniMapLabel);

    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 214, 108, 255);
    g_pRenderText->RenderText(
        GetLayoutScreenX(questTitle),
        GetLayoutScreenY(questTitle),
        L"",
        GetLayoutScreenWidth(questTitle),
        0,
        RT3_SORT_CENTER);

    const bool miniMapHovered =
        CheckLayoutMouse(CustomQuestUiElement::LauncherButton);
    const bool miniMapPressed = miniMapHovered
        && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    const int miniMapTextX = GetLayoutScreenX(miniMapLabel);
    const int miniMapTextY = GetLayoutScreenY(miniMapLabel);
    const int miniMapTextWidth = GetLayoutScreenWidth(miniMapLabel);

    if (miniMapHovered)
    {
        // Subtle text-only hover. The button art remains untouched so the
        // rounded ends painted into the panel texture stay perfectly clean.
        g_pRenderText->SetTextColor(88, 54, 8, 180);
        g_pRenderText->RenderText(
            miniMapTextX,
            miniMapTextY + 1,
            L"Mini-Map",
            miniMapTextWidth,
            0,
            RT3_SORT_CENTER);

        g_pRenderText->SetTextColor(
            miniMapPressed ? 225 : 255,
            miniMapPressed ? 180 : 232,
            miniMapPressed ? 78 : 145,
            255);
    }
    else
    {
        g_pRenderText->SetTextColor(235, 218, 178, 255);
    }

    g_pRenderText->RenderText(
        miniMapTextX,
        miniMapTextY,
        L"Mini-Map",
        miniMapTextWidth,
        0,
        RT3_SORT_CENTER);

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderQuestButtons()
{
    for (int row = 0; row < QUESTS_PER_PAGE; ++row)
    {
        const int index = GetQuestIndexForRow(row);
        if (index < 0 || index >= static_cast<int>(m_Quests.size()))
        {
            continue;
        }

        const CustomQuestDefinitionView& quest = m_Quests[index];
        const CustomQuestUiRect rectangle = GetTemporaryQuestRowRect(row);
        const int previous = row > 0 ? GetQuestIndexForRow(row - 1) : -1;
        if (previous < 0 || (m_Quests[previous].Category == ProgressiveQuestCategory)
            != (quest.Category == ProgressiveQuestCategory))
        {
            UI::Character::ParisStyle::Solid(rectangle.X, rectangle.Y - 17,
                rectangle.Width, 15, .16f, .14f, .09f);
            g_pRenderText->SetFont(GetQuestHeadingFont());
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->SetTextColor(245, 220, 150, 255);
            g_pRenderText->RenderText(rectangle.X, rectangle.Y - 15,
                quest.Category == ProgressiveQuestCategory
                    ? EtroCustomQuestText(L"Mision principal", L"Main quest")
                    : EtroCustomQuestText(L"Misiones de NPC", L"NPC quests"),
                rectangle.Width, 0, RT3_SORT_CENTER);
        }
        const CustomQuestUiRect hoverRectangle = GetQuestRowHoverRect(row);
        const bool selected = index == m_SelectedIndex;
        const bool hover = QuestCheckMouseIn(
            hoverRectangle.X,
            hoverRectangle.Y,
            hoverRectangle.Width,
            hoverRectangle.Height);

        EnableAlphaTest();
        if (selected)
        {
            UI::Paris::Color(0.34f, 0.25f, 0.08f, 0.70f);
        }
        else if (hover)
        {
            UI::Paris::Color(0.18f, 0.18f, 0.18f, 0.82f);
        }
        else
        {
            UI::Paris::Color(0.06f, 0.06f, 0.07f, 0.68f);
        }

        RenderColor(
            hoverRectangle.X,
            hoverRectangle.Y,
            hoverRectangle.Width,
            hoverRectangle.Height);
        EndRenderColor();

        UI::Paris::Color(
            selected ? 0.88f : 0.33f,
            selected ? 0.66f : 0.33f,
            selected ? 0.20f : 0.33f,
            selected ? 0.85f : 0.45f);
        RenderColor(
            hoverRectangle.X,
            hoverRectangle.Y + hoverRectangle.Height - 1,
            hoverRectangle.Width,
            1.0f);
        EndRenderColor();

        g_pRenderText->SetFont(GetQuestBodyFont());
        g_pRenderText->SetBgColor(0, 0, 0, 0);

        switch (quest.State)
        {
        case CustomQuestClientState::ReadyToClaim:
            g_pRenderText->SetTextColor(110, 255, 145, 255);
            break;
        case CustomQuestClientState::Active:
            g_pRenderText->SetTextColor(115, 220, 255, 255);
            break;
        case CustomQuestClientState::Claimed:
            g_pRenderText->SetTextColor(145, 145, 145, 255);
            break;
        case CustomQuestClientState::Cooldown:
            g_pRenderText->SetTextColor(240, 175, 95, 255);
            break;
        case CustomQuestClientState::Locked:
            g_pRenderText->SetTextColor(170, 120, 120, 255);
            break;
        default:
            g_pRenderText->SetTextColor(
                selected ? 255 : 220,
                selected ? 235 : 220,
                selected ? 165 : 220,
                255);
            break;
        }

        const int displayNumber = quest.Category == ProgressiveQuestCategory
            ? GetProgressiveSequenceNumber(index)
            : ((m_Page * QUESTS_PER_PAGE) + row + 1);

        wchar_t title[96] = {};
        mu_swprintf(
            title,
            L"#%d  %ls",
            std::max(1, displayNumber),
            quest.Name);

        g_pRenderText->RenderText(
            rectangle.X + 8 + GetTextOffsetX(TextQuestName),
            rectangle.Y + 6 + GetTextOffsetY(TextQuestName),
            title,
            rectangle.Width - 34,
            0,
            RT3_SORT_LEFT);

        g_pRenderText->SetFont(GetQuestHeadingFont());
        g_pRenderText->SetTextColor(205, 195, 165, 255);
        g_pRenderText->RenderText(
            rectangle.X + rectangle.Width - 24 + GetTextOffsetX(TextQuestExpandSymbol),
            rectangle.Y + 5 + GetTextOffsetY(TextQuestExpandSymbol),
            selected ? L"-" : L"+",
            18,
            0,
            RT3_SORT_CENTER);
    }

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderQuestDetails()
{
    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (quest == nullptr)
    {
        return;
    }

    const int selectedRow = GetSelectedRowOnPage();
    if (selectedRow < 0)
    {
        return;
    }

    g_pRenderText->SetBgColor(0, 0, 0, 0);

    if (!m_RewardsExpanded)
    {
        const CustomQuestUiRect objective = GetTemporaryObjectiveRect();
        wchar_t requirement[192] = {};
        mu_swprintf(
            requirement,
            EtroCustomQuestText(L"Derrota %ls  %u / %u", L"Defeat %ls  %u / %u"),
            quest->ObjectiveName,
            quest->CurrentKills,
            quest->RequiredKills);

        g_pRenderText->SetFont(GetQuestBodyFont());
        if (quest->CurrentKills >= quest->RequiredKills
            && quest->RequiredKills > 0)
        {
            g_pRenderText->SetTextColor(105, 255, 140, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(220, 220, 220, 255);
        }

        g_pRenderText->RenderText(
            objective.X + GetTextOffsetX(TextObjective),
            objective.Y + GetTextOffsetY(TextObjective),
            requirement,
            objective.Width,
            0,
            RT3_SORT_LEFT);

    }

    const CustomQuestUiRect rewardsHeader = GetTemporaryRewardsHeaderRect();
    const bool rewardsHover = QuestCheckMouseIn(
        rewardsHeader.X,
        rewardsHeader.Y,
        rewardsHeader.Width,
        rewardsHeader.Height);

    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetTextColor(
        rewardsHover ? 255 : 220,
        rewardsHover ? 225 : 205,
        rewardsHover ? 135 : 170,
        255);

    const wchar_t* rewardsTitle = m_RewardsExpanded
        ? EtroCustomQuestText(L"Recompensas  >", L"Rewards  >")
        : EtroCustomQuestText(L"Requisitos  >", L"Requirements  >");

    g_pRenderText->RenderText(
        rewardsHeader.X + GetTextOffsetX(TextRewardsTitle),
        rewardsHeader.Y + GetTextOffsetY(TextRewardsTitle),
        rewardsTitle,
        rewardsHeader.Width,
        0,
        RT3_SORT_LEFT);

    if (m_RewardsExpanded)
    {
        const int rewardCount = std::min<int>(
            quest->RewardCount,
            MAX_REWARDS);

        for (int index = 0; index < rewardCount; ++index)
        {
            const CustomQuestRewardView& reward = quest->Rewards[index];
            const CustomQuestUiRect line =
                GetTemporaryRewardLineRect(index);
            const CustomQuestUiRect hoverRect =
                GetRewardHoverRect(index);
            const bool hover = QuestCheckMouseIn(
                hoverRect.X,
                hoverRect.Y,
                hoverRect.Width,
                hoverRect.Height);

            if (hover)
            {
                EnableAlphaTest();
                UI::Paris::Color(0.30f, 0.22f, 0.06f, 0.42f);
                RenderColor(
                    hoverRect.X,
                    hoverRect.Y,
                    hoverRect.Width,
                    hoverRect.Height);
                EndRenderColor();
            }

            const wchar_t* displayName =
                reward.DisplayName[0] != L'\0'
                    ? reward.DisplayName
                    : EtroCustomQuestText(L"Recompensa", L"Reward");

            g_pRenderText->SetFont(GetQuestBodyFont());
            g_pRenderText->SetTextColor(
                hover ? 255 : 205,
                hover ? 235 : 205,
                hover ? 160 : 205,
                255);
            const int rewardTextSlot = TextReward1 + index;
            g_pRenderText->RenderText(
                line.X + 7 + GetTextOffsetX(rewardTextSlot),
                line.Y + 2 + GetTextOffsetY(rewardTextSlot),
                displayName,
                line.Width - 14,
                0,
                RT3_SORT_LEFT);
        }
    }

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

CustomQuestUiRect CNewUICustomQuestCenter::GetRewardHoverRect(
    int rewardIndex) const
{
    const CustomQuestUiRect base = GetTemporaryRewardLineRect(rewardIndex);
    const CustomQuestUiRect& tuning =
        m_InteractionDebugLayout[InteractionRewardHover];

    CustomQuestUiRect rectangle;
    rectangle.Name = L"RewardHover";
    rectangle.ScreenSpace = true;
    rectangle.X = base.X + tuning.X;
    rectangle.Y = base.Y + tuning.Y;
    rectangle.Width = std::max(1, base.Width);
    rectangle.Height = std::max(1, base.Height);
    return rectangle;
}

int CNewUICustomQuestCenter::GetHoveredRewardIndex() const
{
    if (!m_WindowOpen)
    {
        return -1;
    }

    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (quest == nullptr)
    {
        return -1;
    }

    const int rewardCount = std::min<int>(quest->RewardCount, MAX_REWARDS);
    for (int index = 0; index < rewardCount; ++index)
    {
        const CustomQuestRewardView& reward = quest->Rewards[index];
        if (reward.Type != ItemRewardType
            || reward.ItemGroup > 15
            || reward.ItemNumber >= MAX_ITEM_INDEX)
        {
            continue;
        }

        const CustomQuestUiRect hover = GetRewardHoverRect(index);
        if (QuestCheckMouseIn(
            hover.X,
            hover.Y,
            hover.Width,
            hover.Height))
        {
            return index;
        }
    }

    return -1;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetRewardTooltipRect() const
{
    CustomQuestUiRect preview;
    preview.Name = L"RewardTooltip";
    preview.ScreenSpace = true;
    preview.Width = std::max(168, ScalePanelValue(108));
    preview.Height = std::max(188, ScalePanelValue(120));

    const CustomQuestUiRect panel = GetTemporaryPanelRect();
    const int gap = 6;
    preview.X = panel.X - preview.Width - gap;
    if (preview.X < 2)
    {
        preview.X = panel.X + panel.Width + gap;
    }

    preview.X = std::clamp(
        preview.X,
        2,
        std::max(2, GetQuestLogicalWidth() - 2 - preview.Width));

    preview.Y = panel.Y + 18;
    preview.Y = std::clamp(
        preview.Y,
        2,
        std::max(2, GetQuestLogicalHeight() - 2 - preview.Height));

    return preview;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetHoverPreviewRect() const
{
    CustomQuestUiRect preview;
    preview.Name = L"HoverPreview";
    preview.ScreenSpace = true;
    preview.Width = std::max(96, ScalePanelValue(76));
    preview.Height = std::max(132, ScalePanelValue(106));

    const CustomQuestUiRect panel = GetTemporaryPanelRect();
    const int gap = 6;
    preview.X = panel.X - preview.Width - gap;
    if (preview.X < 2)
    {
        preview.X = panel.X + panel.Width + gap;
    }

    preview.X = std::clamp(
        preview.X,
        2,
        std::max(2, GetQuestLogicalWidth() - 2 - preview.Width));
    preview.Y = panel.Y + 46;
    preview.Y = std::clamp(
        preview.Y,
        2,
        std::max(2, GetQuestLogicalHeight() - 2 - preview.Height));
    return preview;
}

bool CNewUICustomQuestCenter::GetHoveredRewardPreview(
    CustomQuestRewardPreviewInfo& preview) const
{
    preview = {};

    if (EnableQuestUiDebug
        && m_DebugEnabled
        && m_DebugElementIndex
            == static_cast<int>(CustomQuestUiElement::CloseButton))
    {
        return false;
    }

    const int rewardIndex = GetHoveredRewardIndex();
    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (rewardIndex < 0 || quest == nullptr)
    {
        return false;
    }

    const CustomQuestRewardView& reward = quest->Rewards[rewardIndex];
    const CustomQuestUiRect panel = GetRewardTooltipRect();
    const QuestRewardModelTuning tuning =
        GetQuestRewardModelTuning(reward.ItemGroup);

    const int tooltipWidth = std::max(184, ScalePanelValue(112));
    int tooltipX = panel.X + ((panel.Width - tooltipWidth) / 2);
    tooltipX = std::clamp(
        tooltipX,
        2,
        std::max(2, GetQuestLogicalWidth() - 2 - tooltipWidth));

    preview.Active = true;
    preview.Width = static_cast<int>(tuning.Width);
    preview.Height = static_cast<int>(tuning.Height);
    preview.X =
        tooltipX
        + ((tooltipWidth - preview.Width) / 2)
        + static_cast<int>(tuning.OffsetX);
    preview.Y =
        panel.Y
        + std::max(4, ScalePanelValue(3))
        + static_cast<int>(tuning.OffsetY);

    // RenderItem3D expects the raw MU item type index:
    // group * MAX_ITEM_INDEX + item number.
    preview.ItemType =
        (static_cast<int>(reward.ItemGroup) * MAX_ITEM_INDEX)
        + static_cast<int>(reward.ItemNumber);
    preview.Level = static_cast<int>(reward.ItemLevel) << 3;
    preview.ExcellentFlags = static_cast<int>(reward.ExcellentMask);
    preview.AncientDiscriminator =
        static_cast<int>(reward.AncientDiscriminator);
    wcsncpy_s(preview.Name, reward.DisplayName, _TRUNCATE);
    return true;
}

bool CNewUICustomQuestCenter::GetHoveredMonsterPreview(
    CustomQuestMonsterPreviewInfo& preview) const
{
    preview = {};
    if (!m_WindowOpen || m_RewardsExpanded)
    {
        return false;
    }

    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (quest == nullptr || quest->RequiredKills == 0)
    {
        return false;
    }

    const CustomQuestUiRect objective = GetTemporaryObjectiveRect();
    if (!QuestCheckMouseIn(
        objective.X,
        objective.Y,
        objective.Width,
        objective.Height))
    {
        return false;
    }

    const CustomQuestUiRect panel = GetHoverPreviewRect();
    const int horizontalPadding = std::max(5, ScalePanelValue(5));
    const int topPadding = std::max(20, ScalePanelValue(18));
    const int bottomPadding = std::max(18, ScalePanelValue(16));

    preview.Active = true;
    preview.MonsterNumber = quest->MonsterNumber;
    preview.X = panel.X + horizontalPadding;
    preview.Y = panel.Y + topPadding;
    preview.Width = std::max(24, panel.Width - (horizontalPadding * 2));
    preview.Height = std::max(32, panel.Height - topPadding - bottomPadding);
    wcsncpy_s(preview.Name, quest->ObjectiveName, _TRUNCATE);
    return true;
}

void CNewUICustomQuestCenter::RenderRewardSlotHover()
{
    const int rewardIndex = GetHoveredRewardIndex();
    if (rewardIndex < 0)
    {
        return;
    }

    const CustomQuestUiRect rectangle = GetRewardHoverRect(rewardIndex);

    EnableAlphaTest();
    UI::Paris::Color(1.0f, 0.76f, 0.18f, 0.15f);
    RenderColor(
        rectangle.X,
        rectangle.Y,
        rectangle.Width,
        rectangle.Height);
    EndRenderColor();

    UI::Paris::Color(1.0f, 0.76f, 0.18f, 0.92f);
    RenderColor(rectangle.X, rectangle.Y, rectangle.Width, 1.0f);
    RenderColor(
        rectangle.X,
        rectangle.Y + rectangle.Height - 1,
        rectangle.Width,
        1.0f);
    RenderColor(rectangle.X, rectangle.Y, 1.0f, rectangle.Height);
    RenderColor(
        rectangle.X + rectangle.Width - 1,
        rectangle.Y,
        1.0f,
        rectangle.Height);
    EndRenderColor();

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderRewardItemTooltip()
{
    if (EnableQuestUiDebug
        && m_DebugEnabled
        && m_DebugElementIndex
            == static_cast<int>(CustomQuestUiElement::CloseButton))
    {
        return;
    }

    const int rewardIndex = GetHoveredRewardIndex();
    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (rewardIndex < 0 || quest == nullptr)
    {
        return;
    }

    const CustomQuestRewardView& reward = quest->Rewards[rewardIndex];
    ITEM item;
    if (!BuildQuestRewardItem(reward, item))
    {
        return;
    }

    struct TooltipLine
    {
        std::wstring Text;
        BYTE Red;
        BYTE Green;
        BYTE Blue;
        bool Bold;
    };

    std::vector<TooltipLine> lines;
    lines.reserve(18);

    auto addLine = [&lines](
        const std::wstring& line,
        BYTE red,
        BYTE green,
        BYTE blue,
        bool bold = false)
    {
        if (!line.empty())
        {
            lines.push_back({ line, red, green, blue, bold });
        }
    };

    std::wstring title = reward.DisplayName;
    if (reward.ExcellentMask != 0
        && title.rfind(L"Excellent ", 0) != 0)
    {
        title = L"Excellent " + title;
    }
    else if (reward.AncientDiscriminator != 0
        && title.rfind(L"Ancient ", 0) != 0)
    {
        title = L"Ancient " + title;
    }

    if (reward.ItemLevel > 0)
    {
        wchar_t levelSuffix[24] = {};
        mu_swprintf(levelSuffix, L" +%u", reward.ItemLevel);
        title += levelSuffix;
    }

    if (reward.AncientDiscriminator != 0)
    {
        addLine(title, 90, 220, 255, true);
    }
    else if (reward.ExcellentMask != 0)
    {
        // Excellent item names must be green. The native tooltip only turns
        // them green when SpecialNum was populated, which is not reliable for
        // these synthetic quest-preview items.
        addLine(title, 80, 255, 120, true);
    }
    else if (reward.ItemLevel >= 7)
    {
        addLine(title, 255, 220, 80, true);
    }
    else
    {
        addLine(title, 255, 255, 255, true);
    }

    wchar_t text[128] = {};
    if (reward.ItemGroup == 6)
    {
        mu_swprintf(text, L"Defense rate: %u", item.SuccessfulBlocking);
        addLine(text, 235, 235, 235);
    }
    else if (reward.ItemGroup >= 7 && reward.ItemGroup <= 11)
    {
        mu_swprintf(text, L"Defense: %u", item.Defense);
        addLine(text, 235, 235, 235);
    }

    if (reward.Durability > 0)
    {
        mu_swprintf(text, L"Durability: %u", reward.Durability);
        addLine(text, 235, 235, 235);
    }

    if ((reward.ItemFlags & RewardItemFlagSkill) != 0)
    {
        addLine(L"Skill", 100, 190, 255);
    }

    if ((reward.ItemFlags & RewardItemFlagLuck) != 0)
    {
        addLine(L"Luck (Soul Success +25%)", 100, 190, 255);
        addLine(L"Luck (Critical damage +5%)", 100, 190, 255);
    }

    if (reward.OptionLevel > 0)
    {
        const unsigned int optionValue =
            static_cast<unsigned int>(reward.OptionLevel) * 4U;

        if (reward.ItemGroup <= 5)
        {
            mu_swprintf(text, L"Additional damage +%u", optionValue);
        }
        else if (reward.ItemGroup >= 6 && reward.ItemGroup <= 11)
        {
            mu_swprintf(text, L"Additional defense +%u", optionValue);
        }
        else
        {
            mu_swprintf(text, L"Additional option +%u", reward.OptionLevel);
        }

        addLine(text, 100, 190, 255);
    }

    const BYTE excellent = static_cast<BYTE>(reward.ExcellentMask & 0x3F);
    if (reward.ItemGroup <= 5)
    {
        // Standard MU weapon Excellent bit layout.
        if ((excellent & 0x20) != 0)
            addLine(L"Excellent damage +10%", 100, 190, 255);
        if ((excellent & 0x10) != 0)
            addLine(L"Increase damage +level/20", 100, 190, 255);
        if ((excellent & 0x08) != 0)
            addLine(L"Increase damage +2%", 100, 190, 255);
        if ((excellent & 0x04) != 0)
            addLine(L"Increase attack speed +7", 100, 190, 255);
        if ((excellent & 0x02) != 0)
            addLine(L"Increase Life after hunt", 100, 190, 255);
        if ((excellent & 0x01) != 0)
            addLine(L"Increase Mana after hunt", 100, 190, 255);
    }
    else if (reward.ItemGroup >= 6 && reward.ItemGroup <= 11)
    {
        // Standard MU shield/armor Excellent bit layout.
        if ((excellent & 0x20) != 0)
            addLine(L"Increase maximum HP +4%", 100, 190, 255);
        if ((excellent & 0x10) != 0)
            addLine(L"Increase maximum Mana +4%", 100, 190, 255);
        if ((excellent & 0x08) != 0)
            addLine(L"Damage decrease +4%", 100, 190, 255);
        if ((excellent & 0x04) != 0)
            addLine(L"Reflect damage +5%", 100, 190, 255);
        if ((excellent & 0x02) != 0)
            addLine(L"Defense success rate +10%", 100, 190, 255);
        if ((excellent & 0x01) != 0)
            addLine(L"Zen after hunt +40%", 100, 190, 255);
    }

    if (reward.AncientDiscriminator != 0)
    {
        mu_swprintf(
            text,
            L"Ancient option +%u",
            reward.AncientBonusLevel);
        addLine(text, 90, 220, 255);
    }

    const CustomQuestUiRect anchor = GetRewardTooltipRect();
    const int tooltipWidth = std::max(184, ScalePanelValue(112));
    const int modelAreaHeight = std::max(96, ScalePanelValue(58));
    const int titleLineHeight = std::max(13, ScalePanelValue(8));
    const int optionLineHeight = std::max(10, ScalePanelValue(6));
    const int textTopPadding = std::max(4, ScalePanelValue(3));
    const int bottomPadding = std::max(6, ScalePanelValue(4));

    int textHeight = 0;
    if (!lines.empty())
    {
        textHeight = titleLineHeight;
        if (lines.size() > 1)
        {
            textHeight +=
                (static_cast<int>(lines.size()) - 1)
                * optionLineHeight;
        }
    }

    const int tooltipHeight =
        modelAreaHeight
        + textTopPadding
        + textHeight
        + bottomPadding;

    int tooltipX = anchor.X + ((anchor.Width - tooltipWidth) / 2);
    tooltipX = std::clamp(
        tooltipX,
        2,
        std::max(2, GetQuestLogicalWidth() - 2 - tooltipWidth));

    // One compact background contains both the 3D item and all option lines.
    int tooltipY = anchor.Y;
    tooltipY = std::clamp(
        tooltipY,
        2,
        std::max(2, GetQuestLogicalHeight() - 2 - tooltipHeight));

    EnableAlphaTest();
    UI::Paris::Color(0.0f, 0.0f, 0.0f, 0.90f);
    RenderColor(tooltipX, tooltipY, tooltipWidth, tooltipHeight);
    EndRenderColor();

    UI::Paris::Color(0.58f, 0.38f, 0.10f, 0.75f);
    RenderColor(tooltipX, tooltipY, tooltipWidth, 1.0f);
    RenderColor(
        tooltipX,
        tooltipY + tooltipHeight - 1,
        tooltipWidth,
        1.0f);
    EndRenderColor();

    int lineY = tooltipY + modelAreaHeight + textTopPadding;
    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        const TooltipLine& line = lines[index];
        g_pRenderText->SetFont(line.Bold ? GetQuestHeadingFont() : GetQuestBodyFont());
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->SetTextColor(
            line.Red,
            line.Green,
            line.Blue,
            255);
        g_pRenderText->RenderText(
            tooltipX + 5,
            lineY,
            line.Text.c_str(),
            tooltipWidth - 10,
            0,
            RT3_SORT_CENTER);

        lineY += (index == 0)
            ? titleLineHeight
            : optionLineHeight;
    }

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderHoverPreviewPanel()
{
    CustomQuestMonsterPreviewInfo monsterPreview;
    if (!GetHoveredMonsterPreview(monsterPreview))
    {
        return;
    }

    const CustomQuestUiRect panel = GetHoverPreviewRect();
    EnableAlphaTest();
    UI::Paris::Color(0.0f, 0.0f, 0.0f, 0.90f);
    RenderColor(panel.X, panel.Y, panel.Width, panel.Height);
    EndRenderColor();

    UI::Paris::Color(0.60f, 0.40f, 0.12f, 0.82f);
    RenderColor(panel.X, panel.Y, panel.Width, 1.0f);
    RenderColor(panel.X, panel.Y + panel.Height - 1.0f, panel.Width, 1.0f);
    RenderColor(panel.X, panel.Y, 1.0f, panel.Height);
    RenderColor(panel.X + panel.Width - 1.0f, panel.Y, 1.0f, panel.Height);
    EndRenderColor();

    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 220, 125, 255);
    g_pRenderText->RenderText(
        panel.X + 4,
        panel.Y + 5,
        monsterPreview.Name,
        panel.Width - 8,
        0,
        RT3_SORT_CENTER);

    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    wchar_t footer[128] = {};
    if (quest != nullptr)
    {
        mu_swprintf(
            footer,
            EtroCustomQuestText(L"Objetivo %u / %u", L"Objective %u / %u"),
            quest->CurrentKills,
            quest->RequiredKills);
    }

    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(225, 225, 225, 255);
    g_pRenderText->RenderText(
        panel.X + 4,
        panel.Y + panel.Height - 15,
        footer,
        panel.Width - 8,
        0,
        RT3_SORT_CENTER);

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderRewardItems3D()
{
    CustomQuestRewardPreviewInfo hoverPreview;
    if (!GetHoveredRewardPreview(hoverPreview))
    {
        return;
    }

    ::RenderItem3D(
        static_cast<float>(hoverPreview.X),
        static_cast<float>(hoverPreview.Y),
        static_cast<float>(hoverPreview.Width),
        static_cast<float>(hoverPreview.Height),
        hoverPreview.ItemType,
        hoverPreview.Level,
        hoverPreview.ExcellentFlags,
        hoverPreview.AncientDiscriminator,
        false);

    EnableAlphaTest();
    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderActionButton()
{
    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (quest == nullptr)
    {
        return;
    }

    const CustomQuestUiRect rectangle = GetTemporaryActionRect();
    const bool hover = QuestCheckMouseIn(
        rectangle.X,
        rectangle.Y,
        rectangle.Width,
        rectangle.Height);
    const bool actionable =
        quest->State == CustomQuestClientState::Available
        || quest->State == CustomQuestClientState::ReadyToClaim;

    // Stage 4.1: action/state frame removed by request.
    // The invisible hitbox is preserved for Accept/Claim clicks.

    const wchar_t* label = EtroCustomQuestText(L"Bloqueada", L"Locked");
    switch (quest->State)
    {
    case CustomQuestClientState::Available:
        label = EtroCustomQuestText(L"Aceptar mision", L"Accept quest");
        break;
    case CustomQuestClientState::Active:
        label = EtroCustomQuestText(L"En progreso", L"In progress");
        break;
    case CustomQuestClientState::ReadyToClaim:
        label = EtroCustomQuestText(L"Reclamar recompensa", L"Claim reward");
        break;
    case CustomQuestClientState::Claimed:
        label = EtroCustomQuestText(L"Completada", L"Completed");
        break;
    case CustomQuestClientState::Cooldown:
        label = EtroCustomQuestText(L"En espera", L"Cooldown");
        break;
    default:
        break;
    }

    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    if (quest->State == CustomQuestClientState::ReadyToClaim)
    {
        g_pRenderText->SetTextColor(110, 255, 145, 255);
    }
    else if (actionable)
    {
        g_pRenderText->SetTextColor(
            hover ? 255 : 225,
            hover ? 230 : 215,
            hover ? 145 : 180,
            255);
    }
    else
    {
        g_pRenderText->SetTextColor(145, 145, 145, 255);
    }

    g_pRenderText->RenderText(
        rectangle.X + GetTextOffsetX(TextActionButton),
        rectangle.Y + 5 + GetTextOffsetY(TextActionButton),
        label,
        rectangle.Width,
        0,
        RT3_SORT_CENTER);

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::RenderPageControls()
{
    const int pages = GetPageCount();
    if (pages <= 1)
    {
        return;
    }

    const CustomQuestUiRect previous = GetTemporaryPagePreviousRect();
    const CustomQuestUiRect next = GetTemporaryPageNextRect();

    const bool previousHover = QuestCheckMouseIn(
        previous.X,
        previous.Y,
        previous.Width,
        previous.Height);
    const bool nextHover = QuestCheckMouseIn(
        next.X,
        next.Y,
        next.Width,
        next.Height);

    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(
        previousHover ? 255 : 190,
        previousHover ? 225 : 190,
        previousHover ? 135 : 190,
        255);
    g_pRenderText->RenderText(
        previous.X,
        previous.Y + 2,
        L"<",
        previous.Width,
        0,
        RT3_SORT_CENTER);

    g_pRenderText->SetTextColor(
        nextHover ? 255 : 190,
        nextHover ? 225 : 190,
        nextHover ? 135 : 190,
        255);
    g_pRenderText->RenderText(
        next.X,
        next.Y + 2,
        L">",
        next.Width,
        0,
        RT3_SORT_CENTER);

    const CustomQuestUiRect panel = GetTemporaryPanelRect();
    wchar_t pageText[48] = {};
    mu_swprintf(pageText, L"%d / %d", m_Page + 1, pages);

    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(180, 180, 180, 255);
    g_pRenderText->RenderText(
        panel.X + 74 + GetTextOffsetX(TextPageCounter),
        panel.Y + panel.Height - 25 + GetTextOffsetY(TextPageCounter),
        pageText,
        panel.Width - 148,
        0,
        RT3_SORT_CENTER);
}

void CNewUICustomQuestCenter::RenderWrappedText(
    const wchar_t* text,
    int x,
    int y,
    int width,
    int maxLines,
    int lineHeight)
{
    if (text == nullptr || text[0] == L'\0')
    {
        g_pRenderText->RenderText(x, y, L"Sin descripción", width, 0, RT3_SORT_LEFT);
        return;
    }

    const int approximateCharactersPerLine = std::max(12, width / 6);
    std::wstring source(text);
    std::wstring current;
    int line = 0;
    std::size_t position = 0;

    while (position < source.size() && line < maxLines)
    {
        if (source[position] == L'|')
        {
            if (!current.empty())
            {
                g_pRenderText->RenderText(x, y + (line * lineHeight), current.c_str(), width, 0, RT3_SORT_LEFT);
                current.clear();
                ++line;
            }

            ++position;
            continue;
        }

        std::size_t nextSpace = source.find_first_of(L" |", position);
        std::wstring word = source.substr(position, nextSpace == std::wstring::npos ? std::wstring::npos : nextSpace - position);

        if (!current.empty() && current.size() + 1 + word.size() > static_cast<std::size_t>(approximateCharactersPerLine))
        {
            g_pRenderText->RenderText(x, y + (line * lineHeight), current.c_str(), width, 0, RT3_SORT_LEFT);
            current.clear();
            ++line;
            if (line >= maxLines)
            {
                break;
            }
        }

        if (!current.empty())
        {
            current += L' ';
        }

        current += word;

        if (nextSpace == std::wstring::npos)
        {
            position = source.size();
        }
        else
        {
            position = nextSpace + 1;
        }
    }

    if (!current.empty() && line < maxLines)
    {
        g_pRenderText->RenderText(x, y + (line * lineHeight), current.c_str(), width, 0, RT3_SORT_LEFT);
    }
}

void CNewUICustomQuestCenter::RenderStatusText()
{
    if (m_StatusText[0] == L'\0')
    {
        return;
    }

    const CustomQuestUiRect panel = GetTemporaryPanelRect();
    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(155, 155, 155, 255);
    g_pRenderText->RenderText(
        panel.X + 10 + GetTextOffsetX(TextStatus),
        panel.Y + panel.Height - 43 + GetTextOffsetY(TextStatus),
        m_StatusText,
        panel.Width - 20,
        0,
        RT3_SORT_CENTER);
}

void CNewUICustomQuestCenter::RenderStateText(
    const CustomQuestDefinitionView& quest,
    int x,
    int y,
    int width)
{
    switch (quest.State)
    {
    case CustomQuestClientState::Available:
        g_pRenderText->SetTextColor(255, 225, 90, 255);
        break;
    case CustomQuestClientState::Active:
        g_pRenderText->SetTextColor(80, 220, 255, 255);
        break;
    case CustomQuestClientState::ReadyToClaim:
        g_pRenderText->SetTextColor(100, 255, 130, 255);
        break;
    case CustomQuestClientState::Claimed:
        g_pRenderText->SetTextColor(165, 165, 165, 255);
        break;
    case CustomQuestClientState::Cooldown:
        g_pRenderText->SetTextColor(255, 165, 70, 255);
        break;
    default:
        g_pRenderText->SetTextColor(255, 100, 100, 255);
        break;
    }

    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    wchar_t text[128];
    if (quest.State == CustomQuestClientState::Cooldown)
    {
        mu_swprintf(text, L"%ls (%u s)", GetStateText(quest.State), quest.RemainingCooldownSeconds);
    }
    else
    {
        mu_swprintf(text, L"%ls", GetStateText(quest.State));
    }

    g_pRenderText->RenderText(x, y, text, width, 0, RT3_SORT_LEFT);
}

bool CNewUICustomQuestCenter::ProcessLauncherButton()
{
    if (m_WindowOpen || IsQuestLauncherBlocked()) return false;
    if (!SEASON3B::IsPress(VK_LBUTTON))
    {
        return false;
    }

    const CustomQuestUiRect buttonHover = GetQuestButtonHoverRect();
    if (!QuestCheckMouseIn(
        buttonHover.X,
        buttonHover.Y,
        buttonHover.Width,
        buttonHover.Height))
    {
        return false;
    }

    ToggleWindow();
    return true;
}

bool CNewUICustomQuestCenter::ProcessTrackerClick()
{
    if (!SEASON3B::IsPress(VK_LBUTTON))
    {
        return false;
    }

    if (GetTrackedQuest() == nullptr)
    {
        m_TrackerExpanded = false;
        return false;
    }

    const CustomQuestUiRect tracker = GetTemporaryTrackerRect();
    if (!QuestCheckMouseIn(
        tracker.X,
        tracker.Y,
        tracker.Width,
        tracker.Height))
    {
        return false;
    }

    m_TrackerExpanded = !m_TrackerExpanded;
    ::PlayBuffer(SOUND_CLICK01);
    return true;
}

bool CNewUICustomQuestCenter::ProcessWindowButtons()
{
    if (!SEASON3B::IsPress(VK_LBUTTON))
    {
        return false;
    }

    const CustomQuestUiRect close = GetTemporaryCloseRect();
    if (QuestCheckMouseIn(
        close.X,
        close.Y,
        close.Width,
        close.Height))
    {
        CloseWindow();
        return true;
    }

    return false;
}

bool CNewUICustomQuestCenter::ProcessQuestSelection()
{
    if (!SEASON3B::IsPress(VK_LBUTTON))
    {
        return false;
    }

    for (int row = 0; row < QUESTS_PER_PAGE; ++row)
    {
        const int index = GetQuestIndexForRow(row);
        if (index < 0)
        {
            continue;
        }

        const CustomQuestUiRect rectangle = GetTemporaryQuestRowRect(row);
        if (!QuestCheckMouseIn(
            rectangle.X,
            rectangle.Y,
            rectangle.Width,
            TempQuestRowHeight))
        {
            continue;
        }

        if (m_SelectedIndex == index)
        {
            m_SelectedIndex = -1;
            m_RewardsExpanded = false;
        }
        else
        {
            m_SelectedIndex = index;
            m_RewardsExpanded = false;

            const CustomQuestDefinitionView& quest = m_Quests[index];
            if (quest.State == CustomQuestClientState::Active
                || quest.State == CustomQuestClientState::ReadyToClaim)
            {
                m_TrackedQuestId = quest.Id;
            }
        }

        ::PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool CNewUICustomQuestCenter::ProcessRewardsToggle()
{
    if (!SEASON3B::IsPress(VK_LBUTTON)
        || GetSelectedQuest() == nullptr)
    {
        return false;
    }

    const CustomQuestUiRect rewards = GetTemporaryRewardsHeaderRect();
    if (!QuestCheckMouseIn(
        rewards.X,
        rewards.Y,
        rewards.Width,
        rewards.Height))
    {
        return false;
    }

    m_RewardsExpanded = !m_RewardsExpanded;
    ::PlayBuffer(SOUND_CLICK01);
    return true;
}

bool CNewUICustomQuestCenter::ProcessActionButton()
{
    if (!SEASON3B::IsPress(VK_LBUTTON))
    {
        return false;
    }

    CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (quest == nullptr)
    {
        return false;
    }

    const CustomQuestUiRect action = GetTemporaryActionRect();
    if (!QuestCheckMouseIn(
        action.X,
        action.Y,
        action.Width,
        action.Height))
    {
        return false;
    }

    if (quest->State == CustomQuestClientState::Available)
    {
        SendAcceptRequest(quest->Id);
        m_TrackedQuestId = quest->Id;
        ::PlayBuffer(SOUND_CLICK01);
    }
    else if (quest->State == CustomQuestClientState::ReadyToClaim)
    {
        SendClaimRequest(quest->Id);
        ::PlayBuffer(SOUND_CLICK01);
    }
    else
    {
        SetStatus(EtroCustomQuestText(L"La mision no permite esta accion", L"This quest does not allow this action"));
    }

    return true;
}

bool CNewUICustomQuestCenter::ProcessPageControls()
{
    if (!SEASON3B::IsPress(VK_LBUTTON) || GetPageCount() <= 1)
    {
        return false;
    }

    const CustomQuestUiRect previous = GetTemporaryPagePreviousRect();
    if (QuestCheckMouseIn(
        previous.X,
        previous.Y,
        previous.Width,
        previous.Height))
    {
        if (m_Page > 0)
        {
            --m_Page;
            m_SelectedIndex = -1;
            m_RewardsExpanded = false;
            ::PlayBuffer(SOUND_CLICK01);
        }

        return true;
    }

    const CustomQuestUiRect next = GetTemporaryPageNextRect();
    if (QuestCheckMouseIn(
        next.X,
        next.Y,
        next.Width,
        next.Height))
    {
        if (m_Page + 1 < GetPageCount())
        {
            ++m_Page;
            m_SelectedIndex = -1;
            m_RewardsExpanded = false;
            ::PlayBuffer(SOUND_CLICK01);
        }

        return true;
    }

    return false;
}

bool CNewUICustomQuestCenter::ProcessDragging()
{
    int pointerX = 0;
    int pointerY = 0;
    GetQuestPointerPosition(pointerX, pointerY);

    if (m_Dragging)
    {
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
        {
            SetPos(pointerX - m_DragOffsetX, pointerY - m_DragOffsetY);
            return true;
        }

        m_Dragging = false;
    }

    if (SEASON3B::IsPress(VK_LBUTTON)
        && !CheckLayoutMouse(CustomQuestUiElement::CloseButton)
        && CheckLayoutMouse(CustomQuestUiElement::DragArea))
    {
        m_Dragging = true;
        m_DragOffsetX = pointerX - m_Pos.x;
        m_DragOffsetY = pointerY - m_Pos.y;
        return true;
    }

    return false;
}

void CNewUICustomQuestCenter::InitializeTextureDebugLayout()
{
    // ETRO_MUMAIN_DUNGEON_STACK_LAYOUT_FINAL_V1
    // Final launcher position calibrated together with the five dungeon banners.
    CustomQuestUiRect& button = m_TextureLayout[TextureQuestButton];
    button.Name = L"QuestButton";
    button.ScreenSpace = true;
    button.X = 613;
    button.Y = 214;
    button.Width = 31;
    button.Height = 27;

    CustomQuestUiRect& panel = m_TextureLayout[TextureQuestPanel];
    panel.Name = L"QuestPanel";
    panel.ScreenSpace = true;
    panel.X = 468;
    panel.Y = 105;
    panel.Width = 171;
    panel.Height = 364;

    CustomQuestUiRect& tracker = m_TextureLayout[TextureQuestTracker];
    tracker.Name = L"QuestTracker";
    tracker.ScreenSpace = true;
    tracker.X = 571;
    tracker.Y = 410;
    tracker.Width = 130;
    tracker.Height = 22;

    CustomQuestUiRect& expanded = m_TextureLayout[TextureQuestTrackerExpanded];
    expanded.Name = L"QuestTrackerExpanded";
    expanded.ScreenSpace = true;
    expanded.X = 573;
    expanded.Y = 388;
    expanded.Width = 63;
    expanded.Height = 44;

    for (int index = 0; index < TextureCount; ++index)
    {
        m_DefaultTextureLayout[index] = m_TextureLayout[index];
    }
}

const CustomQuestUiRect& CNewUICustomQuestCenter::GetTextureDebugRect(int index) const
{
    return m_TextureLayout[std::clamp(index, 0, TextureCount - 1)];
}

void CNewUICustomQuestCenter::ResetTextureDebugElement(int index)
{
    index = std::clamp(index, 0, TextureCount - 1);
    m_TextureLayout[index] = m_DefaultTextureLayout[index];

    if (index == TextureQuestTracker)
    {
        m_TextureLayout[TextureQuestTrackerExpanded] =
            m_DefaultTextureLayout[TextureQuestTrackerExpanded];
    }
}

bool CNewUICustomQuestCenter::ProcessTextureDebugKeyEvent()
{
    const bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (control && SEASON3B::IsPress(VK_F9))
    {
        m_TextureDebugEnabled = !m_TextureDebugEnabled;
        if (m_TextureDebugEnabled)
        {
            m_InteractionDebugEnabled = false;
            m_TextDebugEnabled = false;
            m_WindowOpen = true;
            m_TrackerExpanded = true;
            SetStatus(L"Debug de texturas Quest activado");
        }
        else
        {
            SetStatus(L"Debug de texturas Quest desactivado");
        }
        return true;
    }

    if (!m_TextureDebugEnabled)
    {
        return false;
    }

    if (SEASON3B::IsPress('1'))
    {
        m_TextureDebugIndex = TextureQuestButton;
        return true;
    }
    if (SEASON3B::IsPress('2'))
    {
        m_TextureDebugIndex = TextureQuestPanel;
        return true;
    }
    if (SEASON3B::IsPress('3'))
    {
        m_TextureDebugIndex = TextureQuestTracker;
        return true;
    }
    if (SEASON3B::IsPress('4'))
    {
        m_TextureDebugIndex = TextureQuestTrackerExpanded;
        return true;
    }

    if (SEASON3B::IsPress('R'))
    {
        ResetTextureDebugElement(m_TextureDebugIndex);
        return true;
    }

    if (SEASON3B::IsPress(VK_F6))
    {
        ExportTextureDebugLayout();
        return true;
    }

    CustomQuestUiRect& rectangle = m_TextureLayout[m_TextureDebugIndex];
    const int step = shift ? 5 : 1;
    int moveX = 0;
    int moveY = 0;
    bool changed = false;

    if (control)
    {
        if (SEASON3B::IsPress(VK_LEFT))
        {
            rectangle.Width = std::max(1, rectangle.Width - step);
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_RIGHT))
        {
            rectangle.Width += step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_UP))
        {
            rectangle.Height = std::max(1, rectangle.Height - step);
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_DOWN))
        {
            rectangle.Height += step;
            changed = true;
        }
    }
    else
    {
        if (SEASON3B::IsPress(VK_LEFT))
        {
            moveX = -step;
        }
        else if (SEASON3B::IsPress(VK_RIGHT))
        {
            moveX = step;
        }
        else if (SEASON3B::IsPress(VK_UP))
        {
            moveY = -step;
        }
        else if (SEASON3B::IsPress(VK_DOWN))
        {
            moveY = step;
        }

        if (moveX != 0 || moveY != 0)
        {
            rectangle.X += moveX;
            rectangle.Y += moveY;
            changed = true;

            if (m_TextureDebugIndex == TextureQuestTracker)
            {
                m_TextureLayout[TextureQuestTrackerExpanded].X += moveX;
                m_TextureLayout[TextureQuestTrackerExpanded].Y += moveY;
            }
        }
    }

    return changed;
}

void CNewUICustomQuestCenter::RenderTextureDebugOverlay()
{
    for (int index = 0; index < TextureCount; ++index)
    {
        const CustomQuestUiRect& rectangle = m_TextureLayout[index];
        const bool selected = index == m_TextureDebugIndex;

        UI::Paris::Color(
            selected ? 1.f : 0.25f,
            selected ? 0.08f : 0.70f,
            selected ? 0.08f : 1.f,
            selected ? 0.28f : 0.10f);
        RenderColor(
            rectangle.X,
            rectangle.Y,
            rectangle.Width,
            rectangle.Height);
        EndRenderColor();

        UI::Paris::Color(
            selected ? 1.f : 0.30f,
            selected ? 0.10f : 0.75f,
            selected ? 0.10f : 1.f,
            0.95f);
        RenderColor(rectangle.X, rectangle.Y, rectangle.Width, selected ? 2.f : 1.f);
        RenderColor(rectangle.X, rectangle.Y + rectangle.Height - 1, rectangle.Width, selected ? 2.f : 1.f);
        RenderColor(rectangle.X, rectangle.Y, selected ? 2.f : 1.f, rectangle.Height);
        RenderColor(rectangle.X + rectangle.Width - 1, rectangle.Y, selected ? 2.f : 1.f, rectangle.Height);
        EndRenderColor();
    }

    const CustomQuestUiRect& selected = m_TextureLayout[m_TextureDebugIndex];
    const int overlayX = 8;
    const int overlayY = 355;
    const int overlayW = 460;
    const int overlayH = 82;

    UI::Paris::Color(0.f, 0.f, 0.f, 0.88f);
    RenderColor(overlayX, overlayY, overlayW, overlayH);
    EndRenderColor();

    wchar_t line1[256] = {};
    wchar_t line2[256] = {};
    wchar_t line3[320] = {};
    mu_swprintf(
        line1,
        L"QUEST TEXTURE DEBUG  [%d/4] %ls  X=%d Y=%d W=%d H=%d",
        m_TextureDebugIndex + 1,
        selected.Name,
        selected.X,
        selected.Y,
        selected.Width,
        selected.Height);
    mu_swprintf(
        line2,
        L"1=Button  2=Panel  3=Tracker  4=TrackerExpanded | Flechas=mover | Ctrl+Flechas=tamano");
    mu_swprintf(
        line3,
        L"Shift=paso 5 | R=reset seleccionado | F6=exportar | Ctrl+F9=cerrar debug");

    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetTextColor(255, 225, 105, 255);
    g_pRenderText->RenderText(overlayX + 6, overlayY + 7, line1, overlayW - 12, 0, RT3_SORT_LEFT);
    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(235, 235, 235, 255);
    g_pRenderText->RenderText(overlayX + 6, overlayY + 29, line2, overlayW - 12, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(overlayX + 6, overlayY + 50, line3, overlayW - 12, 0, RT3_SORT_LEFT);

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::ExportTextureDebugLayout()
{
    FILE* file = nullptr;
    if (_wfopen_s(&file, L"QuestTextureLayout_Debug.txt", L"wt") != 0 || file == nullptr)
    {
        SetStatus(L"No se pudo exportar QuestTextureLayout_Debug.txt");
        return;
    }

    fwprintf(file, L"ETRO QUEST TEXTURE LAYOUT\\n");
    fwprintf(file, L"QuestButton.tga | QuestPanel.tga | QuestTracker.tga | QuestTrackerExpanded.tga\\n\\n");

    for (int index = 0; index < TextureCount; ++index)
    {
        const CustomQuestUiRect& rectangle = m_TextureLayout[index];
        fwprintf(
            file,
            L"%ls X=%d Y=%d W=%d H=%d\\n",
            rectangle.Name,
            rectangle.X,
            rectangle.Y,
            rectangle.Width,
            rectangle.Height);
    }

    fclose(file);
    SetStatus(L"QuestTextureLayout_Debug.txt exportado");
}

void CNewUICustomQuestCenter::InitializeInteractionDebugLayout()
{
    const CustomQuestUiRect rewards = GetTemporaryRewardsHeaderRect();

    // Final production values exported by the user in QuestInteractionLayout_Debug(1).txt.
    CustomQuestUiRect& rowHover =
        m_InteractionDebugLayout[InteractionQuestRowHover];
    rowHover.Name = L"QuestRowHover";
    rowHover.ScreenSpace = false;
    rowHover.X = 0;
    rowHover.Y = 5;
    rowHover.Width = 129;
    rowHover.Height = 14;

    CustomQuestUiRect& buttonHover =
        m_InteractionDebugLayout[InteractionQuestButtonHover];
    buttonHover.Name = L"QuestButtonHover";
    buttonHover.ScreenSpace = false;
    buttonHover.X = 0;
    buttonHover.Y = 0;
    buttonHover.Width = 31;
    buttonHover.Height = 23;

    // One shared tuning rectangle is applied to every reward row.
    // X/Y are relative to each reward line; W/H are independent of the text.
    CustomQuestUiRect& rewardHover =
        m_InteractionDebugLayout[InteractionRewardHover];
    rewardHover.Name = L"RewardHover";
    rewardHover.ScreenSpace = false;
    rewardHover.X = 0;
    rewardHover.Y = 0;
    rewardHover.Width = 50;
    rewardHover.Height = 14;

    for (int index = 0; index < InteractionCount; ++index)
    {
        m_DefaultInteractionDebugLayout[index] =
            m_InteractionDebugLayout[index];
    }
}

CustomQuestUiRect CNewUICustomQuestCenter::GetInteractionDebugScreenRect(
    int index) const
{
    index = std::clamp(index, 0, InteractionCount - 1);
    const CustomQuestUiRect& tuning = m_InteractionDebugLayout[index];

    CustomQuestUiRect rectangle;
    rectangle.Name = tuning.Name;
    rectangle.ScreenSpace = true;
    rectangle.Width = std::max(1, tuning.Width);
    rectangle.Height = std::max(1, tuning.Height);

    if (index == InteractionQuestButtonHover)
    {
        const CustomQuestUiRect button = GetTemporaryLauncherRect();
        rectangle.X = button.X + tuning.X;
        rectangle.Y = button.Y + tuning.Y;
        return rectangle;
    }

    if (index == InteractionRewardHover)
    {
        CustomQuestUiRect base = GetTemporaryRewardLineRect(0);
        if (base.Width <= 0 || base.Height <= 0)
        {
            const CustomQuestUiRect rewards = GetTemporaryRewardsHeaderRect();
            base.X = rewards.X;
            base.Y = rewards.Y + rewards.Height;
            base.Width = rewards.Width;
            base.Height = TempRewardLineHeight;
        }

        rectangle.X = base.X + tuning.X;
        rectangle.Y = base.Y + tuning.Y;
        return rectangle;
    }

    int selectedRow = GetSelectedRowOnPage();
    if (selectedRow < 0)
    {
        selectedRow = 0;
    }

    const CustomQuestUiRect row = GetTemporaryQuestRowRect(selectedRow);
    rectangle.X = row.X + tuning.X;
    rectangle.Y = row.Y + tuning.Y;
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetQuestRowHoverRect(int row) const
{
    const CustomQuestUiRect& tuning =
        m_InteractionDebugLayout[InteractionQuestRowHover];
    const CustomQuestUiRect base = GetTemporaryQuestRowRect(row);

    CustomQuestUiRect rectangle;
    rectangle.Name = L"QuestRowHover";
    rectangle.ScreenSpace = true;
    rectangle.X = base.X + tuning.X;
    rectangle.Y = base.Y + tuning.Y;
    rectangle.Width = std::max(1, tuning.Width);
    rectangle.Height = std::max(1, tuning.Height);
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetQuestButtonHoverRect() const
{
    return GetInteractionDebugScreenRect(InteractionQuestButtonHover);
}

void CNewUICustomQuestCenter::ResetInteractionDebugElement(int index)
{
    index = std::clamp(index, 0, InteractionCount - 1);
    m_InteractionDebugLayout[index] =
        m_DefaultInteractionDebugLayout[index];
}

bool CNewUICustomQuestCenter::ProcessInteractionDebugKeyEvent()
{
    const bool control =
        (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift =
        (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (control && SEASON3B::IsPress(VK_F10))
    {
        m_InteractionDebugEnabled = !m_InteractionDebugEnabled;
        if (m_InteractionDebugEnabled)
        {
            m_TextureDebugEnabled = false;
            m_TextDebugEnabled = false;
            m_WindowOpen = true;
            if (m_SelectedIndex < 0)
            {
                SelectFirstQuestOnPage();
            }

            SetStatus(L"Debug hover/hitbox Quest activado");
        }
        else
        {
            SetStatus(L"Debug hover/hitbox Quest desactivado");
        }

        return true;
    }

    if (!m_InteractionDebugEnabled)
    {
        return false;
    }

    if (SEASON3B::IsPress('1'))
    {
        m_InteractionDebugIndex = InteractionQuestRowHover;
        return true;
    }

    if (SEASON3B::IsPress('2'))
    {
        m_InteractionDebugIndex = InteractionQuestButtonHover;
        return true;
    }

    if (SEASON3B::IsPress('3'))
    {
        m_InteractionDebugIndex = InteractionRewardHover;
        m_RewardsExpanded = true;
        return true;
    }

    if (SEASON3B::IsPress('R'))
    {
        ResetInteractionDebugElement(m_InteractionDebugIndex);
        return true;
    }

    if (SEASON3B::IsPress(VK_F6))
    {
        ExportInteractionDebugLayout();
        return true;
    }

    CustomQuestUiRect& rectangle =
        m_InteractionDebugLayout[m_InteractionDebugIndex];
    const int step = shift ? 5 : 1;
    bool changed = false;

    if (control)
    {
        if (SEASON3B::IsPress(VK_LEFT))
        {
            rectangle.Width = std::max(1, rectangle.Width - step);
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_RIGHT))
        {
            rectangle.Width += step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_UP))
        {
            rectangle.Height = std::max(1, rectangle.Height - step);
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_DOWN))
        {
            rectangle.Height += step;
            changed = true;
        }
    }
    else
    {
        if (SEASON3B::IsPress(VK_LEFT))
        {
            rectangle.X -= step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_RIGHT))
        {
            rectangle.X += step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_UP))
        {
            rectangle.Y -= step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_DOWN))
        {
            rectangle.Y += step;
            changed = true;
        }
    }

    return changed;
}

void CNewUICustomQuestCenter::RenderInteractionDebugOverlay()
{
    for (int index = 0; index < InteractionCount; ++index)
    {
        const bool selected = index == m_InteractionDebugIndex;

        if (index == InteractionRewardHover && selected)
        {
            const CustomQuestDefinitionView* quest = GetSelectedQuest();
            const int rewardCount = quest == nullptr
                ? 0
                : std::min<int>(quest->RewardCount, MAX_REWARDS);

            for (int rewardIndex = 0; rewardIndex < rewardCount; ++rewardIndex)
            {
                const CustomQuestUiRect rewardRectangle =
                    GetRewardHoverRect(rewardIndex);
                UI::Paris::Color(1.f, 0.10f, 0.10f, 0.23f);
                RenderColor(
                    rewardRectangle.X,
                    rewardRectangle.Y,
                    rewardRectangle.Width,
                    rewardRectangle.Height);
                EndRenderColor();

                UI::Paris::Color(1.f, 0.15f, 0.15f, 0.95f);
                RenderColor(rewardRectangle.X, rewardRectangle.Y, rewardRectangle.Width, 2.f);
                RenderColor(rewardRectangle.X, rewardRectangle.Y + rewardRectangle.Height - 1, rewardRectangle.Width, 2.f);
                RenderColor(rewardRectangle.X, rewardRectangle.Y, 2.f, rewardRectangle.Height);
                RenderColor(rewardRectangle.X + rewardRectangle.Width - 1, rewardRectangle.Y, 2.f, rewardRectangle.Height);
                EndRenderColor();
            }

            continue;
        }

        const CustomQuestUiRect rectangle =
            GetInteractionDebugScreenRect(index);

        UI::Paris::Color(
            selected ? 1.f : 0.20f,
            selected ? 0.10f : 0.75f,
            selected ? 0.10f : 1.f,
            selected ? 0.23f : 0.10f);
        RenderColor(
            rectangle.X,
            rectangle.Y,
            rectangle.Width,
            rectangle.Height);
        EndRenderColor();

        UI::Paris::Color(
            selected ? 1.f : 0.25f,
            selected ? 0.15f : 0.80f,
            selected ? 0.15f : 1.f,
            0.95f);
        RenderColor(
            rectangle.X,
            rectangle.Y,
            rectangle.Width,
            selected ? 2.f : 1.f);
        RenderColor(
            rectangle.X,
            rectangle.Y + rectangle.Height - 1,
            rectangle.Width,
            selected ? 2.f : 1.f);
        RenderColor(
            rectangle.X,
            rectangle.Y,
            selected ? 2.f : 1.f,
            rectangle.Height);
        RenderColor(
            rectangle.X + rectangle.Width - 1,
            rectangle.Y,
            selected ? 2.f : 1.f,
            rectangle.Height);
        EndRenderColor();
    }

    const CustomQuestUiRect& tuning =
        m_InteractionDebugLayout[m_InteractionDebugIndex];
    const int overlayX = 8;
    const int overlayY = 350;
    const int overlayW = 520;
    const int overlayH = 86;

    UI::Paris::Color(0.f, 0.f, 0.f, 0.90f);
    RenderColor(overlayX, overlayY, overlayW, overlayH);
    EndRenderColor();

    wchar_t line1[320] = {};
    wchar_t line2[360] = {};
    wchar_t line3[360] = {};

    mu_swprintf(
        line1,
        L"QUEST HOVER DEBUG [%d/3] %ls XOFF=%d YOFF=%d W=%d H=%d",
        m_InteractionDebugIndex + 1,
        tuning.Name,
        tuning.X,
        tuning.Y,
        tuning.Width,
        tuning.Height);
    mu_swprintf(
        line2,
        L"1=Hover mision | 2=Hover/Click Q | 3=Hover recompensas | Flechas=mover | Ctrl+Flechas=tamano");
    mu_swprintf(
        line3,
        L"Shift=paso 5 | R=reset | F6=exportar | Ctrl+F10=cerrar");

    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetTextColor(255, 225, 105, 255);
    g_pRenderText->RenderText(
        overlayX + 6,
        overlayY + 7,
        line1,
        overlayW - 12,
        0,
        RT3_SORT_LEFT);
    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(235, 235, 235, 255);
    g_pRenderText->RenderText(
        overlayX + 6,
        overlayY + 31,
        line2,
        overlayW - 12,
        0,
        RT3_SORT_LEFT);
    g_pRenderText->RenderText(
        overlayX + 6,
        overlayY + 54,
        line3,
        overlayW - 12,
        0,
        RT3_SORT_LEFT);

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::ExportInteractionDebugLayout()
{
    FILE* file = nullptr;
    if (_wfopen_s(
            &file,
            L"QuestInteractionLayout_Debug.txt",
            L"wt") != 0
        || file == nullptr)
    {
        SetStatus(
            L"No se pudo exportar QuestInteractionLayout_Debug.txt");
        return;
    }

    fwprintf(file, L"ETRO QUEST HOVER / HITBOX LAYOUT\n");
    fwprintf(
        file,
        L"Valores relativos a QuestPanel / QuestButton.\n\n");

    for (int index = 0; index < InteractionCount; ++index)
    {
        const CustomQuestUiRect& rectangle =
            m_InteractionDebugLayout[index];
        fwprintf(
            file,
            L"%ls OFFSET_X=%d OFFSET_Y=%d W=%d H=%d\n",
            rectangle.Name,
            rectangle.X,
            rectangle.Y,
            rectangle.Width,
            rectangle.Height);
    }

    fclose(file);
    SetStatus(L"QuestInteractionLayout_Debug.txt exportado");
}

void CNewUICustomQuestCenter::InitializeTextDebugLayout()
{
    const wchar_t* names[TextCount] =
    {
        L"PanelTitle",
        L"PanelSubtitle",
        L"CloseX",
        L"QuestName",
        L"QuestExpandSymbol",
        L"RequirementsTitle",
        L"Objective",
        L"RewardsTitle",
        L"Reward1",
        L"Reward2",
        L"Reward3",
        L"Reward4",
        L"Reward5",
        L"ActionButtonText",
        L"StatusText",
        L"PageCounter",
        L"TrackerLabel",
        L"TrackerProgress",
    };

    for (int index = 0; index < TextCount; ++index)
    {
        CustomQuestUiRect& offset = m_TextDebugLayout[index];
        offset.Name = names[index];
        offset.X = 0;
        offset.Y = 0;
        offset.Width = 0;
        offset.Height = 0;
        offset.ScreenSpace = true;
    }

    // The clean panel uses its layout rectangles directly for text and hitboxes.
    m_TextDebugLayout[TextTrackerLabel].X = 8;
    m_TextDebugLayout[TextTrackerLabel].Y = 17;
    m_TextDebugLayout[TextTrackerProgress].X = 0;
    m_TextDebugLayout[TextTrackerProgress].Y = -1;

    for (int index = 0; index < TextCount; ++index)
    {
        m_DefaultTextDebugLayout[index] = m_TextDebugLayout[index];
    }
}

const CustomQuestUiRect& CNewUICustomQuestCenter::GetTextDebugOffset(int index) const
{
    return m_TextDebugLayout[std::clamp(index, 0, TextCount - 1)];
}

int CNewUICustomQuestCenter::GetTextOffsetX(int index) const
{
    return GetTextDebugOffset(index).X;
}

int CNewUICustomQuestCenter::GetTextOffsetY(int index) const
{
    return GetTextDebugOffset(index).Y;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTextDebugScreenRect(int index) const
{
    index = std::clamp(index, 0, TextCount - 1);
    const CustomQuestUiRect& offset = m_TextDebugLayout[index];
    const CustomQuestUiRect panel = GetTemporaryPanelRect();
    const CustomQuestUiRect tracker = GetTemporaryTrackerRect();
    const CustomQuestUiRect expanded = GetTemporaryTrackerExpandedRect();

    CustomQuestUiRect rectangle;
    rectangle.Name = offset.Name;
    rectangle.ScreenSpace = true;
    rectangle.Width = 80;
    rectangle.Height = 14;

    int selectedRow = GetSelectedRowOnPage();
    if (selectedRow < 0)
    {
        selectedRow = 0;
    }

    const CustomQuestUiRect row = GetTemporaryQuestRowRect(selectedRow);
    const CustomQuestUiRect objective = GetTemporaryObjectiveRect();
    const CustomQuestUiRect rewards = GetTemporaryRewardsHeaderRect();
    const CustomQuestUiRect action = GetTemporaryActionRect();
    const CustomQuestUiRect close = GetTemporaryCloseRect();

    switch (index)
    {
    case TextPanelTitle:
        rectangle.X = panel.X + 8;
        rectangle.Y = panel.Y + 8;
        rectangle.Width = std::max(1, panel.Width - 32);
        break;
    case TextPanelSubtitle:
        rectangle.X = panel.X + 8;
        rectangle.Y = panel.Y + 29;
        rectangle.Width = std::max(1, panel.Width - 16);
        break;
    case TextCloseButton:
        rectangle.X = close.X;
        rectangle.Y = close.Y + 2;
        rectangle.Width = close.Width;
        rectangle.Height = close.Height;
        break;
    case TextQuestName:
        rectangle.X = row.X + 8;
        rectangle.Y = row.Y + 6;
        rectangle.Width = std::max(1, row.Width - 34);
        break;
    case TextQuestExpandSymbol:
        rectangle.X = row.X + row.Width - 24;
        rectangle.Y = row.Y + 5;
        rectangle.Width = 18;
        break;
    case TextRequirementsTitle:
        rectangle.X = row.X + 10;
        rectangle.Y = row.Y + TempQuestRowHeight + 7;
        rectangle.Width = std::max(1, row.Width - 20);
        break;
    case TextObjective:
        rectangle.X = objective.X;
        rectangle.Y = objective.Y;
        rectangle.Width = std::max(1, objective.Width);
        rectangle.Height = std::max(1, objective.Height);
        break;
    case TextRewardsTitle:
        rectangle.X = rewards.X;
        rectangle.Y = rewards.Y;
        rectangle.Width = std::max(1, rewards.Width);
        rectangle.Height = std::max(1, rewards.Height);
        break;
    case TextReward1:
    case TextReward2:
    case TextReward3:
    case TextReward4:
    case TextReward5:
        {
            const int rewardIndex = index - TextReward1;
            rectangle.X = rewards.X + 7;
            rectangle.Y = rewards.Y + rewards.Height
                + (rewardIndex * TempRewardLineHeight) + 2;
            rectangle.Width = std::max(1, rewards.Width - 14);
            rectangle.Height = TempRewardLineHeight;
        }
        break;
    case TextActionButton:
        rectangle.X = action.X;
        rectangle.Y = action.Y + 5;
        rectangle.Width = std::max(1, action.Width);
        rectangle.Height = std::max(1, action.Height);
        break;
    case TextStatus:
        rectangle.X = panel.X + 10;
        rectangle.Y = panel.Y + panel.Height - 43;
        rectangle.Width = std::max(1, panel.Width - 20);
        break;
    case TextPageCounter:
        rectangle.X = panel.X + 74;
        rectangle.Y = panel.Y + panel.Height - 25;
        rectangle.Width = std::max(1, panel.Width - 148);
        break;
    case TextTrackerLabel:
        rectangle.X = tracker.X + 3;
        rectangle.Y = tracker.Y + 2;
        rectangle.Width = std::max(1, tracker.Width - 6);
        break;
    case TextTrackerProgress:
        rectangle.X = expanded.X + 5;
        rectangle.Y = expanded.Y + 10;
        rectangle.Width = std::max(1, expanded.Width - 10);
        break;
    default:
        rectangle.X = panel.X;
        rectangle.Y = panel.Y;
        break;
    }

    rectangle.X += offset.X;
    rectangle.Y += offset.Y;
    return rectangle;
}

void CNewUICustomQuestCenter::ResetTextDebugElement(int index)
{
    index = std::clamp(index, 0, TextCount - 1);
    m_TextDebugLayout[index] = m_DefaultTextDebugLayout[index];
}

bool CNewUICustomQuestCenter::ProcessTextDebugKeyEvent()
{
    const bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (control && SEASON3B::IsPress(VK_F8))
    {
        m_TextDebugEnabled = !m_TextDebugEnabled;
        if (m_TextDebugEnabled)
        {
            m_TextureDebugEnabled = false;
            m_InteractionDebugEnabled = false;
            m_WindowOpen = true;
            m_TrackerExpanded = true;
            m_RewardsExpanded = true;
            if (m_SelectedIndex < 0)
            {
                SelectFirstQuestOnPage();
            }
            SetStatus(L"Debug de textos Quest activado");
        }
        else
        {
            SetStatus(L"Debug de textos Quest desactivado");
        }
        return true;
    }

    if (!m_TextDebugEnabled)
    {
        return false;
    }

    if (SEASON3B::IsPress(VK_TAB))
    {
        if (shift)
        {
            --m_TextDebugIndex;
            if (m_TextDebugIndex < 0)
            {
                m_TextDebugIndex = TextCount - 1;
            }
        }
        else
        {
            m_TextDebugIndex = (m_TextDebugIndex + 1) % TextCount;
        }
        return true;
    }

    if (SEASON3B::IsPress('R'))
    {
        ResetTextDebugElement(m_TextDebugIndex);
        return true;
    }

    if (SEASON3B::IsPress(VK_F6))
    {
        ExportTextDebugLayout();
        return true;
    }

    CustomQuestUiRect& offset = m_TextDebugLayout[m_TextDebugIndex];
    const int step = shift ? 5 : 1;
    bool changed = false;

    if (SEASON3B::IsPress(VK_LEFT))
    {
        offset.X -= step;
        changed = true;
    }
    else if (SEASON3B::IsPress(VK_RIGHT))
    {
        offset.X += step;
        changed = true;
    }
    else if (SEASON3B::IsPress(VK_UP))
    {
        offset.Y -= step;
        changed = true;
    }
    else if (SEASON3B::IsPress(VK_DOWN))
    {
        offset.Y += step;
        changed = true;
    }

    return changed;
}

void CNewUICustomQuestCenter::RenderTextDebugOverlay()
{
    const CustomQuestUiRect rectangle =
        GetTextDebugScreenRect(m_TextDebugIndex);

    UI::Paris::Color(1.f, 0.08f, 0.08f, 0.18f);
    RenderColor(
        rectangle.X,
        rectangle.Y,
        std::max(1, rectangle.Width),
        std::max(1, rectangle.Height));
    EndRenderColor();

    UI::Paris::Color(1.f, 0.10f, 0.10f, 0.95f);
    RenderColor(rectangle.X, rectangle.Y, std::max(1, rectangle.Width), 1.0f);
    RenderColor(rectangle.X, rectangle.Y + std::max(1, rectangle.Height) - 1, std::max(1, rectangle.Width), 1.0f);
    RenderColor(rectangle.X, rectangle.Y, 1.0f, std::max(1, rectangle.Height));
    RenderColor(rectangle.X + std::max(1, rectangle.Width) - 1, rectangle.Y, 1.0f, std::max(1, rectangle.Height));
    EndRenderColor();

    const CustomQuestUiRect& offset = m_TextDebugLayout[m_TextDebugIndex];
    const int overlayX = 8;
    const int overlayY = 350;
    const int overlayW = 500;
    const int overlayH = 86;

    UI::Paris::Color(0.f, 0.f, 0.f, 0.90f);
    RenderColor(overlayX, overlayY, overlayW, overlayH);
    EndRenderColor();

    wchar_t line1[300] = {};
    wchar_t line2[340] = {};
    wchar_t line3[340] = {};
    mu_swprintf(
        line1,
        L"QUEST TEXT DEBUG [%d/%d] %ls  OFFSET X=%d Y=%d",
        m_TextDebugIndex + 1,
        TextCount,
        offset.Name,
        offset.X,
        offset.Y);
    mu_swprintf(
        line2,
        L"TAB=siguiente | Shift+TAB=anterior | Flechas=mover | Shift+Flechas=paso 5");
    mu_swprintf(
        line3,
        L"R=reset seleccionado | F6=exportar QuestTextLayout_Debug.txt | Ctrl+F8=cerrar");

    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetTextColor(255, 225, 105, 255);
    g_pRenderText->RenderText(overlayX + 6, overlayY + 7, line1, overlayW - 12, 0, RT3_SORT_LEFT);
    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(235, 235, 235, 255);
    g_pRenderText->RenderText(overlayX + 6, overlayY + 31, line2, overlayW - 12, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(overlayX + 6, overlayY + 54, line3, overlayW - 12, 0, RT3_SORT_LEFT);

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::ExportTextDebugLayout()
{
    FILE* file = nullptr;
    if (_wfopen_s(&file, L"QuestTextLayout_Debug.txt", L"wt") != 0 || file == nullptr)
    {
        SetStatus(L"No se pudo exportar QuestTextLayout_Debug.txt");
        return;
    }

    fwprintf(file, L"ETRO QUEST TEXT OFFSETS\n");
    fwprintf(file, L"Offsets relativos: mover QuestPanel no rompe estas posiciones.\n\n");

    for (int index = 0; index < TextCount; ++index)
    {
        const CustomQuestUiRect& offset = m_TextDebugLayout[index];
        fwprintf(
            file,
            L"%ls OFFSET_X=%d OFFSET_Y=%d\n",
            offset.Name,
            offset.X,
            offset.Y);
    }

    fclose(file);
    SetStatus(L"QuestTextLayout_Debug.txt exportado");
}

void CNewUICustomQuestCenter::InitializeDebugLayout()
{
    auto set = [this](
        CustomQuestUiElement element,
        const wchar_t* name,
        int x,
        int y,
        int width,
        int height,
        bool screenSpace)
    {
        CustomQuestUiRect& rectangle = m_Layout[static_cast<int>(element)];
        rectangle.Name = name;
        rectangle.X = x;
        rectangle.Y = y;
        rectangle.Width = width;
        rectangle.Height = height;
        rectangle.ScreenSpace = screenSpace;
    };


    set(CustomQuestUiElement::LauncherButton, L"MiniMapButton", 17, 105, 29, 8, false);
    set(CustomQuestUiElement::DragArea, L"DragArea", -1, -1, 130, 133, false);
    set(CustomQuestUiElement::CloseButton, L"CloseButton", 121, 3, 7, 7, false);
    set(CustomQuestUiElement::QuestTitle, L"QuestTitle", 42, 4, 46, 12, false);
    set(CustomQuestUiElement::MiniMapLabel, L"MiniMapLabel", 3, 107, 55, 10, false);
    set(CustomQuestUiElement::QuestMissionHover, L"QuestMissionHover", 4, 22, 55, 16, false);
    set(CustomQuestUiElement::QuestRow1, L"QuestRow1", 3, 21, 56, 15, false);
    set(CustomQuestUiElement::QuestRow2, L"QuestRow2", 3, 37, 55, 15, false);
    set(CustomQuestUiElement::QuestRow3, L"QuestRow3", 3, 52, 55, 15, false);
    set(CustomQuestUiElement::QuestRow4, L"QuestRow4", 3, 68, 55, 20, false);
    set(CustomQuestUiElement::QuestText1, L"QuestText1", 15, 26, 42, 12, false);
    set(CustomQuestUiElement::QuestText2, L"QuestText2", 15, 42, 37, 12, false);
    set(CustomQuestUiElement::QuestText3, L"QuestText3", 15, 58, 25, 7, false);
    set(CustomQuestUiElement::QuestText4, L"QuestText4", 15, 74, 25, 12, false);
    set(CustomQuestUiElement::MissionTitle, L"MissionTitle", 75, 24, 40, 14, false);
    set(CustomQuestUiElement::MissionState, L"MissionState", 82, 104, 32, 9, false);
    set(CustomQuestUiElement::MissionDescription, L"MissionDescription", 79, 42, 152, 1, false);
    set(CustomQuestUiElement::MissionObjective, L"MissionObjective", 68, 60, 50, 9, false);
    set(CustomQuestUiElement::RewardSlot1, L"RewardSlot1", 63, 82, 9, 15, false);
    set(CustomQuestUiElement::RewardSlot2, L"RewardSlot2", 75, 82, 9, 15, false);
    set(CustomQuestUiElement::RewardSlot3, L"RewardSlot3", 87, 82, 10, 15, false);
    set(CustomQuestUiElement::RewardSlot4, L"RewardSlot4", 99, 82, 10, 15, false);
    set(CustomQuestUiElement::RewardSlot5, L"RewardSlot5", 111, 82, 10, 15, false);
    set(CustomQuestUiElement::MinimumLevel, L"MinimumLevel", 83, 32, 31, 13, false);
    set(CustomQuestUiElement::ActionButton, L"ActionButton", 65, 117, 52, 14, false);
    set(CustomQuestUiElement::PagePrevious, L"PagePrevious", -2, 118, 28, 19, false);
    set(CustomQuestUiElement::PageNext, L"PageNext", 43, 119, 14, 14, false);
    set(CustomQuestUiElement::PageText, L"PageText", 4, 119, 52, 14, false);
    set(CustomQuestUiElement::StatusText, L"StatusText", 4, 88, 51, 4, false);



    for (int i = 0; i < LayoutElementCount; ++i)
    {
        m_DefaultLayout[i] = m_Layout[i];
    }
}

bool CNewUICustomQuestCenter::ProcessDebugKeyEvent()
{
    const bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (control && SEASON3B::IsPress(VK_F10))
    {
        m_DebugEnabled = !m_DebugEnabled;
        if (m_DebugEnabled)
        {
            m_WindowOpen = true;

            if (MiniMapButtonFocusedDebug)
            {
                m_DebugElementIndex =
                    static_cast<int>(CustomQuestUiElement::LauncherButton);
                SetStatus(L"Debug del boton de minimapa activado");
            }
            else
            {
                m_DebugElementIndex =
                    static_cast<int>(CustomQuestUiElement::QuestTitle);
                SetStatus(L"Quest UI Debug activado");
            }
        }
        else
        {
            SetStatus(L"Quest UI Debug desactivado");
        }

        return true;
    }

    if (!m_DebugEnabled)
    {
        return false;
    }

    if (MiniMapButtonFocusedDebug)
    {
        m_DebugElementIndex =
            static_cast<int>(CustomQuestUiElement::LauncherButton);
    }
    else
    {
        if (SEASON3B::IsPress(VK_PRIOR))
        {
            --m_DebugElementIndex;
            if (m_DebugElementIndex < 0)
            {
                m_DebugElementIndex = LayoutElementCount - 1;
            }

            return true;
        }

        if (SEASON3B::IsPress(VK_NEXT))
        {
            m_DebugElementIndex =
                (m_DebugElementIndex + 1) % LayoutElementCount;
            return true;
        }

        if (control && shift)
        {
            if (SEASON3B::IsPress(VK_RIGHT)
                || SEASON3B::IsPress(VK_UP))
            {
                SetPanelScalePercent(
                    m_PanelScalePercent + PanelScaleStepPercent);
                return true;
            }

            if (SEASON3B::IsPress(VK_LEFT)
                || SEASON3B::IsPress(VK_DOWN))
            {
                SetPanelScalePercent(
                    m_PanelScalePercent - PanelScaleStepPercent);
                return true;
            }

            if (SEASON3B::IsPress(VK_HOME))
            {
                SetPanelScalePercent(DefaultPanelScalePercent);
                return true;
            }
        }
    }

    if (SEASON3B::IsPress(VK_HOME))
    {
        ResetSelectedDebugElement();
        return true;
    }

    if (SEASON3B::IsPress(VK_F6))
    {
        ExportDebugLayout();
        return true;
    }

    CustomQuestUiRect& rectangle = m_Layout[m_DebugElementIndex];
    const int step = shift ? 5 : 1;
    bool changed = false;

    if (control && !shift)
    {
        if (SEASON3B::IsPress(VK_LEFT))
        {
            rectangle.Width = std::max(1, rectangle.Width - step);
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_RIGHT))
        {
            rectangle.Width += step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_UP))
        {
            rectangle.Height = std::max(1, rectangle.Height - step);
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_DOWN))
        {
            rectangle.Height += step;
            changed = true;
        }
    }
    else
    {
        if (SEASON3B::IsPress(VK_LEFT))
        {
            rectangle.X -= step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_RIGHT))
        {
            rectangle.X += step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_UP))
        {
            rectangle.Y -= step;
            changed = true;
        }
        else if (SEASON3B::IsPress(VK_DOWN))
        {
            rectangle.Y += step;
            changed = true;
        }
    }

    return changed;
}

void CNewUICustomQuestCenter::RenderDebugOverlay()
{
    if (MiniMapButtonFocusedDebug)
    {
        RenderDebugRectangle(
            m_Layout[m_DebugElementIndex],
            true);
    }
    else
    {
        for (int i = 0; i < LayoutElementCount; ++i)
        {
            RenderDebugRectangle(
                m_Layout[i],
                i == m_DebugElementIndex);
        }
    }

    const CustomQuestUiRect& selected = m_Layout[m_DebugElementIndex];

    // Dedicated close-button guide. It uses a strong fill, white outer border,
    // red inner border and a center crosshair, so the clickable area is clear
    // even when it overlaps the close-button artwork in the panel texture.
    if (m_DebugElementIndex
        == static_cast<int>(CustomQuestUiElement::CloseButton))
    {
        const int closeX = GetLayoutScreenX(selected);
        const int closeY = GetLayoutScreenY(selected);
        const int closeW = GetLayoutScreenWidth(selected);
        const int closeH = GetLayoutScreenHeight(selected);
        const int centerX = closeX + (closeW / 2);
        const int centerY = closeY + (closeH / 2);

        UI::Paris::Color(1.f, 0.f, 0.f, 0.34f);
        RenderColor(closeX, closeY, closeW, closeH);
        EndRenderColor();

        UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
        RenderColor(closeX - 1, closeY - 1, closeW + 2, 2.f);
        EndRenderColor();
        RenderColor(closeX - 1, closeY + closeH - 1, closeW + 2, 2.f);
        EndRenderColor();
        RenderColor(closeX - 1, closeY - 1, 2.f, closeH + 2);
        EndRenderColor();
        RenderColor(closeX + closeW - 1, closeY - 1, 2.f, closeH + 2);
        EndRenderColor();

        UI::Paris::Color(1.f, 0.f, 0.f, 1.f);
        RenderColor(closeX, closeY, closeW, 2.f);
        EndRenderColor();
        RenderColor(closeX, closeY + closeH - 2, closeW, 2.f);
        EndRenderColor();
        RenderColor(closeX, closeY, 2.f, closeH);
        EndRenderColor();
        RenderColor(closeX + closeW - 2, closeY, 2.f, closeH);
        EndRenderColor();
        RenderColor(centerX - 1, closeY, 2.f, closeH);
        EndRenderColor();
        RenderColor(closeX, centerY - 1, closeW, 2.f);
        EndRenderColor();

        UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
    }

    UI::Paris::Color(0.f, 0.f, 0.f, 0.86f);
    RenderColor(DebugOverlayX, DebugOverlayY, DebugOverlayWidth, DebugOverlayHeight);
    EndRenderColor();
    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);

    wchar_t line1[192];
    wchar_t line2[192];
    wchar_t line3[256];
    mu_swprintf(
        line1,
        L"QUEST UI DEBUG  %d/%d  %ls  |  ESCALA %d%%  PANEL %dx%d",
        m_DebugElementIndex + 1,
        LayoutElementCount,
        selected.Name,
        m_PanelScalePercent,
        GetPanelWidth(),
        GetPanelHeight());
    mu_swprintf(
        line2,
        L"BASE X=%d Y=%d W=%d H=%d  |  HITBOX %dx%d  |  %ls",
        selected.X,
        selected.Y,
        selected.Width,
        selected.Height,
        GetLayoutScreenWidth(selected),
        GetLayoutScreenHeight(selected),
        selected.ScreenSpace ? L"SCREEN" : L"PANEL");
    mu_swprintf(
        line3,
        MiniMapButtonFocusedDebug
            ? L"MINIMAP BUTTON | Flechas mover | Ctrl+Flechas tamaño | Shift=5 | Home reset | F6 exportar"
            : L"PgUp/PgDn: QuestTitle, MiniMapLabel, QuestMissionHover, ActionButton | Flechas mover | Ctrl+Flechas tamaño | Shift=5 | F6 exportar");

    g_pRenderText->SetFont(GetQuestHeadingFont());
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 230, 110, 255);
    g_pRenderText->RenderText(DebugOverlayX + 6, DebugOverlayY + 6, line1, DebugOverlayWidth - 12, 0, RT3_SORT_LEFT);

    g_pRenderText->SetFont(GetQuestBodyFont());
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->RenderText(DebugOverlayX + 6, DebugOverlayY + 25, line2, DebugOverlayWidth - 12, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(DebugOverlayX + 6, DebugOverlayY + 44, line3, DebugOverlayWidth - 12, 0, RT3_SORT_LEFT);
    g_pRenderText->SetTextColor(255, 90, 90, 255);
    g_pRenderText->RenderText(
        DebugOverlayX + 6,
        DebugOverlayY + 63,
        MiniMapButtonFocusedDebug
            ? L"Solo se muestra el hitbox MiniMapButton; no altera el resto del layout."
            : L"El hitbox seleccionado se muestra con relleno rojo y borde rojo grueso.",
        DebugOverlayWidth - 12,
        0,
        RT3_SORT_LEFT);
}

void CNewUICustomQuestCenter::RenderDebugRectangle(const CustomQuestUiRect& rectangle, bool selected)
{
    const int x = GetLayoutScreenX(rectangle);
    const int y = GetLayoutScreenY(rectangle);
    const int width = GetLayoutScreenWidth(rectangle);
    const int height = GetLayoutScreenHeight(rectangle);

    if (selected)
    {
        UI::Paris::Color(1.f, 0.f, 0.f, 0.28f);
        RenderColor(x, y, width, height);
        EndRenderColor();

        UI::Paris::Color(1.f, 0.05f, 0.05f, 1.f);
        RenderColor(x, y, width, 2.f);
        EndRenderColor();
        RenderColor(x, y + height - 2, width, 2.f);
        EndRenderColor();
        RenderColor(x, y, 2.f, height);
        EndRenderColor();
        RenderColor(x + width - 2, y, 2.f, height);
        EndRenderColor();

        g_pRenderText->SetFont(GetQuestHeadingFont());
        g_pRenderText->SetBgColor(0, 0, 0, 180);
        g_pRenderText->SetTextColor(255, 70, 70, 255);
        g_pRenderText->RenderText(x + 3, y + 3, rectangle.Name, std::max(1, width - 6), 0, RT3_SORT_LEFT);
    }
    else
    {
        UI::Paris::Color(0.2f, 0.65f, 1.f, 0.04f);
        RenderColor(x, y, width, height);
        EndRenderColor();

        UI::Paris::Color(0.35f, 0.75f, 1.f, 0.24f);
        RenderColor(x, y, width, 1.f);
        EndRenderColor();
        RenderColor(x, y + height - 1, width, 1.f);
        EndRenderColor();
        RenderColor(x, y, 1.f, height);
        EndRenderColor();
        RenderColor(x + width - 1, y, 1.f, height);
        EndRenderColor();
    }

    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
}

void CNewUICustomQuestCenter::ExportDebugLayout()
{
    FILE* file = nullptr;
    if (_wfopen_s(&file, L"QuestUILayout_Debug.txt", L"wt") != 0 || file == nullptr)
    {
        SetStatus(L"No se pudo exportar QuestUILayout_Debug.txt");
        return;
    }

    fwprintf(file, L"QUEST UI LAYOUT DEBUG\\n");
    fwprintf(file, L"PanelBaseWidth=%d PanelBaseHeight=%d PanelScalePercent=%d RenderWidth=%d RenderHeight=%d\\n\\n", WINDOW_WIDTH, WINDOW_HEIGHT, m_PanelScalePercent, GetPanelWidth(), GetPanelHeight());

    for (int i = 0; i < LayoutElementCount; ++i)
    {
        const CustomQuestUiRect& rectangle = m_Layout[i];
        fwprintf(
            file,
            L"%ls X=%d Y=%d W=%d H=%d Space=%ls\\n",
            rectangle.Name,
            rectangle.X,
            rectangle.Y,
            rectangle.Width,
            rectangle.Height,
            rectangle.ScreenSpace ? L"Screen" : L"Panel");
    }

    fclose(file);
    SetStatus(L"Layout exportado: QuestUILayout_Debug.txt");
}

void CNewUICustomQuestCenter::ResetSelectedDebugElement()
{
    m_Layout[m_DebugElementIndex] = m_DefaultLayout[m_DebugElementIndex];
}

void CNewUICustomQuestCenter::SetPanelScalePercent(int percent)
{
    const int clamped = std::clamp(percent, MinimumPanelScalePercent, MaximumPanelScalePercent);
    if (clamped == m_PanelScalePercent)
    {
        return;
    }

    const int oldWidth = GetPanelWidth();
    const int oldHeight = GetPanelHeight();
    const int centerX = m_Pos.x + (oldWidth / 2);
    const int centerY = m_Pos.y + (oldHeight / 2);

    m_PanelScalePercent = clamped;

    SetPos(
        centerX - (GetPanelWidth() / 2),
        centerY - (GetPanelHeight() / 2));

    wchar_t text[96];
    mu_swprintf(text, L"Escala del panel: %d%%", m_PanelScalePercent);
    SetStatus(text);
}

int CNewUICustomQuestCenter::GetPanelWidth() const
{
    return std::max(1, ScalePanelValue(WINDOW_WIDTH));
}

int CNewUICustomQuestCenter::GetPanelHeight() const
{
    return std::max(1, ScalePanelValue(WINDOW_HEIGHT));
}

int CNewUICustomQuestCenter::ScalePanelValue(int value) const
{
    return static_cast<int>((static_cast<float>(value) * static_cast<float>(m_PanelScalePercent)) / 100.0f);
}

CustomQuestUiRect& CNewUICustomQuestCenter::GetLayout(CustomQuestUiElement element)
{
    return m_Layout[static_cast<int>(element)];
}

const CustomQuestUiRect& CNewUICustomQuestCenter::GetLayout(CustomQuestUiElement element) const
{
    return m_Layout[static_cast<int>(element)];
}

int CNewUICustomQuestCenter::GetLayoutScreenX(const CustomQuestUiRect& rectangle) const
{
    return rectangle.ScreenSpace ? rectangle.X : m_Pos.x + ScalePanelValue(rectangle.X);
}

int CNewUICustomQuestCenter::GetLayoutScreenY(const CustomQuestUiRect& rectangle) const
{
    return rectangle.ScreenSpace ? rectangle.Y : m_Pos.y + ScalePanelValue(rectangle.Y);
}

int CNewUICustomQuestCenter::GetLayoutScreenWidth(const CustomQuestUiRect& rectangle) const
{
    return rectangle.ScreenSpace ? rectangle.Width : std::max(1, ScalePanelValue(rectangle.Width));
}

int CNewUICustomQuestCenter::GetLayoutScreenHeight(const CustomQuestUiRect& rectangle) const
{
    return rectangle.ScreenSpace ? rectangle.Height : std::max(1, ScalePanelValue(rectangle.Height));
}

bool CNewUICustomQuestCenter::CheckLayoutMouse(CustomQuestUiElement element) const
{
    const CustomQuestUiRect& rectangle = GetLayout(element);
    return QuestCheckMouseIn(
        GetLayoutScreenX(rectangle),
        GetLayoutScreenY(rectangle),
        GetLayoutScreenWidth(rectangle),
        GetLayoutScreenHeight(rectangle));
}

CustomQuestUiElement CNewUICustomQuestCenter::GetQuestRowElement(int row) const
{
    return static_cast<CustomQuestUiElement>(QuestRowFirstElement + std::clamp(row, 0, QUESTS_PER_PAGE - 1));
}

CustomQuestUiElement CNewUICustomQuestCenter::GetQuestTextElement(int row) const
{
    return static_cast<CustomQuestUiElement>(QuestTextFirstElement + std::clamp(row, 0, QUESTS_PER_PAGE - 1));
}

CustomQuestUiElement CNewUICustomQuestCenter::GetRewardSlotElement(int rewardIndex) const
{
    return static_cast<CustomQuestUiElement>(RewardSlotFirstElement + std::clamp(rewardIndex, 0, MAX_REWARDS - 1));
}

int CNewUICustomQuestCenter::GetSelectedRowOnPage() const
{
    if (m_SelectedIndex < 0)
    {
        return -1;
    }

    for (int row = 0; row < QUESTS_PER_PAGE; ++row)
    {
        if (GetQuestIndexForRow(row) == m_SelectedIndex)
        {
            return row;
        }
    }

    return -1;
}

const CustomQuestDefinitionView* CNewUICustomQuestCenter::GetTrackedQuest() const
{
    if (m_TrackedQuestId != 0)
    {
        for (const auto& quest : m_Quests)
        {
            if (quest.Id == m_TrackedQuestId
                && (quest.State == CustomQuestClientState::Active
                    || quest.State == CustomQuestClientState::ReadyToClaim))
            {
                return &quest;
            }
        }
    }

    for (const auto& quest : m_Quests)
    {
        if (quest.State == CustomQuestClientState::Active
            || quest.State == CustomQuestClientState::ReadyToClaim)
        {
            return &quest;
        }
    }

    return nullptr;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryPanelRect() const
{
    auto panel = GetTextureDebugRect(TextureQuestPanel);
    int height = 48 + 58;
    int previousCategory = -1;
    for (int row = 0; row < QUESTS_PER_PAGE; ++row)
    {
        const int index = GetQuestIndexForRow(row);
        if (index < 0) break;
        const int category = m_Quests[index].Category == ProgressiveQuestCategory ? 1 : 0;
        if (previousCategory >= 0 && previousCategory != category) height += 18;
        previousCategory = category;
        height += TempQuestRowHeight + TempQuestRowGap;
        if (index == m_SelectedIndex)
        {
            height += TempExpandedBaseHeight;
            if (m_RewardsExpanded) height += std::min<int>(m_Quests[index].RewardCount, MAX_REWARDS) * TempRewardLineHeight;
        }
    }
    panel.Height = std::max(128, height);
    panel.Y = std::max(12, std::min(panel.Y, GetQuestLogicalHeight() - panel.Height - 50));
    return panel;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryLauncherRect() const
{
    return GetTextureDebugRect(TextureQuestButton);
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryTrackerRect() const
{
    return GetTextureDebugRect(TextureQuestTracker);
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryTrackerExpandedRect() const
{
    return GetTextureDebugRect(TextureQuestTrackerExpanded);
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryQuestRowRect(
    int row) const
{
    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryQuestRow";
    rectangle.ScreenSpace = true;

    const CustomQuestUiRect panel = GetTemporaryPanelRect();
    rectangle.X = panel.X + 8;
    rectangle.Y = panel.Y + 48;
    rectangle.Width = panel.Width - 16;
    rectangle.Height = TempQuestRowHeight;

    const int selectedRow = GetSelectedRowOnPage();

    for (int index = 0; index < row; ++index)
    {
        rectangle.Y += TempQuestRowHeight + TempQuestRowGap;
        const int current = GetQuestIndexForRow(index);
        const int next = GetQuestIndexForRow(index + 1);
        if (current >= 0 && next >= 0
            && (m_Quests[current].Category == ProgressiveQuestCategory)
                != (m_Quests[next].Category == ProgressiveQuestCategory))
            rectangle.Y += 18;

        if (index == selectedRow)
        {
            const CustomQuestDefinitionView* quest = GetSelectedQuest();
            const int rewardCount = quest == nullptr
                ? 0
                : std::min<int>(quest->RewardCount, MAX_REWARDS);

            rectangle.Y += TempExpandedBaseHeight;
            if (m_RewardsExpanded)
            {
                rectangle.Y += rewardCount * TempRewardLineHeight;
            }
        }
    }

    if (row == selectedRow)
    {
        const CustomQuestDefinitionView* quest = GetSelectedQuest();
        const int rewardCount = quest == nullptr
            ? 0
            : std::min<int>(quest->RewardCount, MAX_REWARDS);

        rectangle.Height += TempExpandedBaseHeight;
        if (m_RewardsExpanded)
        {
            rectangle.Height += rewardCount * TempRewardLineHeight;
        }
    }

    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryObjectiveRect() const
{
    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryQuestObjective";
    rectangle.ScreenSpace = true;

    const int selectedRow = GetSelectedRowOnPage();
    if (selectedRow < 0)
    {
        return rectangle;
    }

    const CustomQuestUiRect row = GetTemporaryQuestRowRect(selectedRow);
    rectangle.X = row.X + 10;
    rectangle.Y = row.Y + TempQuestRowHeight + 25;
    rectangle.Width = row.Width - 20;
    rectangle.Height = 16;
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryRewardsHeaderRect() const
{
    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryRewardsHeader";
    rectangle.ScreenSpace = true;

    const int selectedRow = GetSelectedRowOnPage();
    if (selectedRow < 0)
    {
        return rectangle;
    }

    const CustomQuestUiRect row = GetTemporaryQuestRowRect(selectedRow);
    rectangle.X = row.X + 10;
    rectangle.Y = row.Y + TempQuestRowHeight + 7;
    rectangle.Width = row.Width - 20;
    rectangle.Height = 17;
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryRewardLineRect(
    int rewardIndex) const
{
    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryRewardLine";
    rectangle.ScreenSpace = true;

    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    if (!m_RewardsExpanded
        || quest == nullptr
        || rewardIndex < 0
        || rewardIndex >= std::min<int>(quest->RewardCount, MAX_REWARDS))
    {
        return rectangle;
    }

    const CustomQuestUiRect rewards = GetTemporaryRewardsHeaderRect();
    rectangle.X = rewards.X;
    rectangle.Y =
        rewards.Y
        + rewards.Height
        + (rewardIndex * TempRewardLineHeight);
    rectangle.Width = rewards.Width;
    rectangle.Height = TempRewardLineHeight;
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryActionRect() const
{
    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryQuestAction";
    rectangle.ScreenSpace = true;

    const CustomQuestDefinitionView* quest = GetSelectedQuest();
    const int selectedRow = GetSelectedRowOnPage();
    if (quest == nullptr || selectedRow < 0)
    {
        return rectangle;
    }

    const CustomQuestUiRect rewards = GetTemporaryRewardsHeaderRect();
    const int rewardCount = std::min<int>(
        quest->RewardCount,
        MAX_REWARDS);

    rectangle.X = rewards.X + 8;
    // Place the action below whichever view is currently visible.
    rectangle.Y =
        rewards.Y
        + rewards.Height
        + 5
        + (m_RewardsExpanded ? rewardCount * TempRewardLineHeight : 20);

    rectangle.Width = std::max(1, rewards.Width - 16);
    rectangle.Height = 22;
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryCloseRect() const
{
    const CustomQuestUiRect panel = GetTemporaryPanelRect();

    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryQuestClose";
    rectangle.ScreenSpace = true;
    rectangle.X = panel.X + panel.Width - 25;
    rectangle.Y = panel.Y + 5;
    rectangle.Width = 19;
    rectangle.Height = 19;
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryPagePreviousRect() const
{
    const CustomQuestUiRect panel = GetTemporaryPanelRect();

    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryPagePrevious";
    rectangle.ScreenSpace = true;
    rectangle.X = panel.X + 8;
    rectangle.Y = panel.Y + panel.Height - 29;
    rectangle.Width = 24;
    rectangle.Height = 20;
    return rectangle;
}

CustomQuestUiRect CNewUICustomQuestCenter::GetTemporaryPageNextRect() const
{
    const CustomQuestUiRect panel = GetTemporaryPanelRect();

    CustomQuestUiRect rectangle;
    rectangle.Name = L"TemporaryPageNext";
    rectangle.ScreenSpace = true;
    rectangle.X = panel.X + panel.Width - 32;
    rectangle.Y = panel.Y + panel.Height - 29;
    rectangle.Width = 24;
    rectangle.Height = 20;
    return rectangle;
}

int CNewUICustomQuestCenter::GetCurrentProgressiveQuestIndex() const
{
    for (int index = 0; index < static_cast<int>(m_Quests.size()); ++index)
    {
        const CustomQuestDefinitionView& quest = m_Quests[index];
        if (quest.Category != ProgressiveQuestCategory)
        {
            continue;
        }

        // Claimed progressive steps are historical; the first unresolved step
        // is the only one exposed to the player. This also means a Locked step
        // correctly blocks later steps instead of letting the chain skip ahead.
        if (quest.State != CustomQuestClientState::Claimed)
        {
            return index;
        }
    }

    return -1;
}

int CNewUICustomQuestCenter::GetProgressiveSequenceNumber(int actualIndex) const
{
    int sequence = 0;
    for (int index = 0; index < static_cast<int>(m_Quests.size()); ++index)
    {
        if (m_Quests[index].Category != ProgressiveQuestCategory)
        {
            continue;
        }

        ++sequence;
        if (index == actualIndex)
        {
            return sequence;
        }
    }

    return 0;
}

void CNewUICustomQuestCenter::BuildVisibleQuestIndices(
    std::vector<int>& indices) const
{
    indices.clear();

    const int progressiveIndex = GetCurrentProgressiveQuestIndex();
    if (progressiveIndex >= 0)
    {
        indices.push_back(progressiveIndex);
    }

    // Keep other categories available for the later Daily/other tabs. Only
    // Category 0 is collapsed into a one-at-a-time progressive chain.
    for (int index = 0; index < static_cast<int>(m_Quests.size()); ++index)
    {
        if (m_Quests[index].Category != ProgressiveQuestCategory)
        {
            indices.push_back(index);
        }
    }
}

void CNewUICustomQuestCenter::AutoAcceptCurrentProgressiveQuest()
{
    const int index = GetCurrentProgressiveQuestIndex();
    if (index < 0 || index >= static_cast<int>(m_Quests.size()))
    {
        return;
    }

    CustomQuestDefinitionView& quest = m_Quests[index];
    if (quest.State != CustomQuestClientState::Available)
    {
        return;
    }

    // Mark as tracked immediately so the compact tracker can take over as
    // soon as the server confirms the automatic acceptance.
    m_TrackedQuestId = quest.Id;
    SendAcceptRequest(quest.Id);
}

int CNewUICustomQuestCenter::GetPageCount() const
{
    std::vector<int> visibleQuestIndices;
    BuildVisibleQuestIndices(visibleQuestIndices);
    if (visibleQuestIndices.empty())
    {
        return 1;
    }

    return (static_cast<int>(visibleQuestIndices.size()) + QUESTS_PER_PAGE - 1) / QUESTS_PER_PAGE;
}

int CNewUICustomQuestCenter::GetQuestIndexForRow(int row) const
{
    std::vector<int> visibleQuestIndices;
    BuildVisibleQuestIndices(visibleQuestIndices);

    const int visibleIndex = (m_Page * QUESTS_PER_PAGE) + row;
    return visibleIndex >= 0
        && visibleIndex < static_cast<int>(visibleQuestIndices.size())
            ? visibleQuestIndices[visibleIndex]
            : -1;
}

CustomQuestDefinitionView* CNewUICustomQuestCenter::GetSelectedQuest()
{
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Quests.size()))
    {
        return nullptr;
    }

    return &m_Quests[m_SelectedIndex];
}

const CustomQuestDefinitionView* CNewUICustomQuestCenter::GetSelectedQuest() const
{
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Quests.size()))
    {
        return nullptr;
    }

    return &m_Quests[m_SelectedIndex];
}

CustomQuestDefinitionView* CNewUICustomQuestCenter::FindQuest(std::int32_t questId)
{
    auto found = std::find_if(
        m_Quests.begin(),
        m_Quests.end(),
        [questId](const CustomQuestDefinitionView& quest)
        {
            return quest.Id == questId;
        });

    return found == m_Quests.end() ? nullptr : &(*found);
}

void CNewUICustomQuestCenter::SelectFirstQuestOnPage()
{
    m_SelectedIndex = GetQuestIndexForRow(0);
}

void CNewUICustomQuestCenter::SetStatus(const wchar_t* text)
{
    if (text == nullptr)
    {
        m_StatusText[0] = L'\0';
        return;
    }

    wcsncpy_s(m_StatusText, text, _TRUNCATE);
}

const wchar_t* CNewUICustomQuestCenter::GetStateText(CustomQuestClientState state) const
{
    switch (state)
    {
    case CustomQuestClientState::Available:
        return EtroCustomQuestText(L"DISPONIBLE", L"AVAILABLE");
    case CustomQuestClientState::Active:
        return EtroCustomQuestText(L"EN PROGRESO", L"IN PROGRESS");
    case CustomQuestClientState::ReadyToClaim:
        return EtroCustomQuestText(L"LISTA PARA RECLAMAR", L"READY TO CLAIM");
    case CustomQuestClientState::Claimed:
        return EtroCustomQuestText(L"COMPLETADA", L"COMPLETED");
    case CustomQuestClientState::Cooldown:
        return EtroCustomQuestText(L"EN ESPERA", L"COOLDOWN");
    default:
        return EtroCustomQuestText(L"BLOQUEADA", L"LOCKED");
    }
}

const wchar_t* CNewUICustomQuestCenter::GetActionResultText(BYTE result) const
{
    switch (result)
    {
    case 1:
        return EtroCustomQuestText(L"mision inexistente", L"quest not found");
    case 2:
        return EtroCustomQuestText(L"mision desactivada", L"quest disabled");
    case 3:
        return EtroCustomQuestText(L"nivel insuficiente", L"level too low");
    case 4:
        return EtroCustomQuestText(L"mision ya activa", L"quest already active");
    case 5:
        return EtroCustomQuestText(L"mision ya completada", L"quest already completed");
    case 6:
        return EtroCustomQuestText(L"cooldown activo", L"cooldown active");
    case 7:
        return EtroCustomQuestText(L"mision no activa", L"quest not active");
    case 8:
        return EtroCustomQuestText(L"objetivo incompleto", L"objective incomplete");
    case 9:
        return EtroCustomQuestText(L"inventario lleno", L"inventory full");
    case 10:
        return EtroCustomQuestText(L"recompensa invalida", L"invalid reward");
    default:
        return EtroCustomQuestText(L"error interno", L"internal error");
    }
}

std::uint16_t CNewUICustomQuestCenter::ReadUInt16(const BYTE* data)
{
    return static_cast<std::uint16_t>(data[0])
        | (static_cast<std::uint16_t>(data[1]) << 8);
}

std::uint32_t CNewUICustomQuestCenter::ReadUInt32(const BYTE* data)
{
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
}

std::int32_t CNewUICustomQuestCenter::ReadInt32(const BYTE* data)
{
    return static_cast<std::int32_t>(ReadUInt32(data));
}

void CNewUICustomQuestCenter::WriteInt32(BYTE* data, std::int32_t value)
{
    const auto unsignedValue = static_cast<std::uint32_t>(value);
    data[0] = static_cast<BYTE>(unsignedValue & 0xFF);
    data[1] = static_cast<BYTE>((unsignedValue >> 8) & 0xFF);
    data[2] = static_cast<BYTE>((unsignedValue >> 16) & 0xFF);
    data[3] = static_cast<BYTE>((unsignedValue >> 24) & 0xFF);
}

void CNewUICustomQuestCenter::CopyFixedUtf8(
    wchar_t* destination,
    std::size_t destinationCount,
    const BYTE* source,
    std::size_t sourceLength)
{
    if (destination == nullptr || destinationCount == 0)
    {
        return;
    }

    destination[0] = L'\0';

    std::string value(
        reinterpret_cast<const char*>(source),
        reinterpret_cast<const char*>(source) + sourceLength);

    const auto terminator = value.find('\0');
    if (terminator != std::string::npos)
    {
        value.resize(terminator);
    }

    if (value.empty())
    {
        return;
    }

    CMultiLanguage::ConvertFromUtf8(destination, value.c_str(), static_cast<int>(value.size()));
    destination[destinationCount - 1] = L'\0';
}

