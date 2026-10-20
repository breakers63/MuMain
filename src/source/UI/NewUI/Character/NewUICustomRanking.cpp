#include "stdafx.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "ParisFlatStyle.h"
#include "ParisControls.h"
#include "ParisRankUser.h"
#include "Render/UI/CharacterPreviewCamera.h"
#include "UI/NewUI/Dialogs/ParisCustomMenuWindow.h"
#include "GuildRankingView.h"
#include "MissionRankingView.h"
#include "RankingHoverCard.h"
#include "PanelAspectScale.h"
#include "UI/NewUI/Character/RankingEquipmentView.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/ParisMarketAssets.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/Widgets/NewUIChatInputBox.h"
namespace {
    UI::Theme::ParisFrameAssets g_ParisRankingFrame;
    UI::Theme::ParisMarketAssets g_ParisRankingExact;
    std::array<GLuint, 3> g_PodiumImages{BITMAP_UNKNOWN, BITMAP_UNKNOWN, BITMAP_UNKNOWN};
    HFONT g_RankingFont = nullptr, g_RankingBold = nullptr, g_RankingTitle = nullptr, g_RankingNameFont = nullptr;
    GLuint g_RankingReferenceSkin = BITMAP_UNKNOWN;
    GLuint g_RankingWidgets = BITMAP_UNKNOWN;
    GLuint g_RankingStatus = BITMAP_UNKNOWN;
    GLuint g_RankingColoredWidgets = BITMAP_UNKNOWN;
    GLuint g_RankingFooter = BITMAP_UNKNOWN;
    HFONT g_RankingValueFont = nullptr;
    float RankingPhysicalScale(float zoom)
    {
        constexpr float DefaultZoom = .816f;
        return UI::Character::PanelAspect::Fit(static_cast<float>(WindowWidth), std::max(1.f,static_cast<float>(WindowHeight)-40.f*g_fScreenRate_y), 750.f, 560.f) * zoom / DefaultZoom;
    }
    constexpr int RankingTextureSize = 1024;
    constexpr int RankingTextureHeight = 800;
    GLuint g_RankingShell = BITMAP_UNKNOWN;
    std::wstring g_RankingCacheOwner;
}
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Character/NewUICustomRanking.h"

#include "Audio/DSPlaySound.h"
#include "Camera/CameraProjection.h"
#include "Character/CharacterManager.h"
#include "Character/CSParts.h"
#include "Core/Globals/_enum.h"
#include "Engine/AI/GOBoid.h"
#include "GameLogic/Pets/GIPetManager.h"
#include "GameLogic/Pets/w_PetProcess.h"
#include "Guild/GuildCache.h"
#include "Network/Server/WSclient.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Textures/ZzzTexture.h"
#include "Scenes/SceneCore.h"
#include "UI/Legacy/UIManager.h"
#include "UI/NewUI/NewUISystem.h"
#include "World/MapInfra/MapManager.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwctype>

using namespace SEASON3B;

extern DWORD g_dwKeyFocusUIID;
extern void MoveCharacter(CHARACTER* character, OBJECT* object);
extern void MoveCharacterVisual(CHARACTER* character, OBJECT* object);

SEASON3B::CNewUICustomRanking* g_pCustomRankingWindow = nullptr;

// MUETRO_GUILD_RANKING_TABS_V1
// Reuses Top Reset UI/3D preview for a Guild Ranking tab.
// DEKOHUT_QUEST_RANKING_V1
// Adds a third ranking tab for persisted progressive quest completions.

// MUETRO_TOP_RESET_LAYOUT_DEBUG_V1_2_1_BLACK_RECT_FIX

namespace
{
    constexpr BYTE RankingHeadCode = 0xED;
    constexpr BYTE RankingListRequest = 0x50;
    constexpr BYTE RankingListResponse = 0x51;
    constexpr BYTE RankingGuildListRequest = 0x52;
    constexpr BYTE RankingGuildListResponse = 0x53;
    constexpr BYTE RankingCharacterRequest = 0x54;
    constexpr BYTE RankingCharacterResponse = 0x55;
    constexpr BYTE RankingGuildLeaderRequest = 0x56;
    constexpr BYTE RankingGuildLeaderResponse = 0x57;
    constexpr BYTE RankingQuestListRequest = 0x58;
    constexpr BYTE RankingQuestListResponse = 0x59;
    constexpr BYTE RankingQuestCharacterRequest = 0x5A;
    constexpr BYTE RankingQuestCharacterResponse = 0x5B;
    constexpr BYTE RankingProtocolVersion = 1;

    constexpr DWORD ListCacheMilliseconds = 300000;
    constexpr DWORD ListTimeoutMilliseconds = 6000;
    constexpr DWORD ProfileTimeoutMilliseconds = 5000;
    constexpr int CharacterNameLength = 10;
    constexpr int GuildNameLength = 10;
    constexpr int AppearanceLength = 27;
    constexpr int RankingEntryLength = CharacterNameLength + 4;
    constexpr int GuildRankingEntryLength = GuildNameLength + CharacterNameLength + 4 + 4;
    constexpr int QuestRankingEntryLength = CharacterNameLength + 4 + 4;

    constexpr int ListX = 25;
    constexpr int ListY = 139;
    constexpr int ListWidth = 194;
    constexpr int RowHeight = 33;
    constexpr int ProfileX = 239;
    constexpr int ProfileY = 134;
    constexpr int ProfileWidth = 167;
    constexpr int ProfileHeight = 329;

    void DrawRankingStatus(int x, int y, int width, int height, const wchar_t* text, HFONT font, bool active)
    {
        if(g_RankingStatus!=BITMAP_UNKNOWN)
        {
            const bool own=wcsncmp(text,L"MI EQUIPO",9)==0;
            const int row=own?(active?0:1):2;
            EnableAlphaTest();
            const float brightness=own && !UI::Character::RankingEquipment().PreferenceReady()?.65f:1.f;
            UI::Paris::Color(brightness,brightness,brightness,1);
            RenderBitmap(g_RankingStatus,x,y,width,height,0.f,row*64.f/256.f,(own?400.f:256.f)/512.f,60.f/256.f);
            UI::Paris::Color(1,1,1,1);
            return;
        }
        UI::Character::ParisStyle::Solid(x, y, width, height, active ? .22f : .32f, active ? .47f : .26f, .27f);
        UI::Character::ParisStyle::Solid(x + 1, y + 1, width - 2, height - 2, .035f, .07f, .055f);
        g_pRenderText->SetFont(font ? font : g_hFont);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->SetTextColor(active ? 148 : 210, active ? 228 : 200, active ? 184 : 170, 255);
        g_pRenderText->RenderText(x, y + (height - 10) / 2, text, width, 0, RT3_SORT_CENTER);
    }

    void DrawRankingClassBadge(int x, int y, int width, int height)
    {
        const float radiusX = width * .5f, radiusY = height * .5f;
        for (int row = 0; row < height; ++row)
        {
            const float normalizedY = (row - radiusY) / radiusY;
            const int half = static_cast<int>(radiusX * std::sqrt(std::max(0.f, 1.f - normalizedY * normalizedY)));
            if (half <= 0) continue;
            UI::Character::ParisStyle::Solid(x + width / 2 - half, y + row, half * 2, 1, .7f, .51f, .17f);
            if (row > 0 && row < height - 1 && half > 1)
                UI::Character::ParisStyle::Solid(x + width / 2 - half + 1, y + row, half * 2 - 2, 1, .065f, .05f, .025f);
        }
    }


    int ResetRankingRowHeight(int row, int spacingAdjustment)
    {
        (void)row;
        return RowHeight + spacingAdjustment;
    }

    constexpr int SyntheticGuildKey = 0x6F520001;

    bool IsFloatingPetModel(int modelType)
    {
        switch (modelType)
        {
        case MODEL_DEMON:
        case MODEL_SPIRIT_OF_GUARDIAN:
        case MODEL_PET_RUDOLF:
        case MODEL_PET_PANDA:
        case MODEL_PET_UNICORN:
        case MODEL_PET_SKELETON:
            return true;
        default:
            return false;
        }
    }

    bool IsDarkSpirit(const BYTE* item)
    {
        if (item[0] == 0xFF || item[1] == 0xFF)
        {
            return false;
        }

        const BYTE group = (item[0] & 0xF0) >> 4;
        const short number = MAKEWORD(item[1], item[0] & 0x0F);
        return group == ITEM_GROUP_HELPER && number == ITEM_NUMBER_DARK_SPIRIT;
    }

    void RenderPodiumMedal(int x, int y, int width, int height, int rank)
{
        if (rank < 1 || rank > static_cast<int>(g_PodiumImages.size())) return;
        const GLuint image = g_PodiumImages[rank - 1];
        if (image == BITMAP_UNKNOWN) return;
        EnableAlphaTest();
        UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
        RenderBitmap(image, x, y, width, height, 0.f, 0.f, 1.f, 1.f);
}

}

// MUETRO_TOP_RESET_LAYOUT_FINAL_V1_3_NO_DEBUG
CNewUICustomRanking* CNewUICustomRanking::NewWindow()
{
    // Keep allocation and construction together when the window layout changes.
    return new CNewUICustomRanking;
}

CNewUICustomRanking::CNewUICustomRanking()
    : m_Manager(nullptr),
      m_SearchBox(nullptr),
      m_SearchEntryCount(-1),
      m_EscapeHeld(false),
      m_Position{ 60, 35 },
      m_ImagesLoaded(false),
      m_WindowOpen(false),
      m_ListPending(false),
      m_ProfilePending(false),
      m_ListRequestTick(0),
      m_ProfileRequestTick(0),
      m_Tab(TAB_RESET),
      m_Page(0),
      m_SelectedIndex(-1),
      m_GuildNameUtf8{},
      m_GuildMark{},
      m_LastProfilePacketSize(0),
      m_LastProfilePayloadOffset(0),
      m_GuildMarkNonZeroBytes(0),
      m_GuildMarkNonZeroPixels(0),
      m_GuildMarkByteSum(0),
      m_Level(0),
      m_Resets(0),
      m_TotalStats(0),
      m_GuildScore(0),
      m_GuildMemberCount(0),
      m_QuestCompleted(0),
      m_QuestTotal(0),
      m_HasPreview(false),
      m_PreviewHelperScale(0.0f),
      m_PreviewAngle(90.0f),
      m_PreviewZoom(0.50f),
      m_GuildMarkIndex(-1),
      m_PanelScale(0.816f),
      m_PanelOffsetX(0),
      m_PanelOffsetY(0),
      m_DebugEnabled(false),
      m_DebugElement(DEBUG_PREVIEW),
      m_CharacterColumnOffsetX(0),
      m_CharacterColumnOffsetY(0),
      m_CharacterNameOffsetX(0),
      m_CharacterNameOffsetY(0),
      m_ResetColumnOffsetX(0),
      m_ResetColumnOffsetY(0),
      m_ResetValueOffsetX(-7),
      m_ResetValueOffsetY(0),
      m_RowSpacingAdjustment(0),
      m_TitleRect{ 270, 12, 210, 27 },
      m_ResetTabRect{ 239, 44, 90, 25 },
      m_GuildTabRect{ 332, 44, 90, 25 },
      m_QuestTabRect{ 425, 44, 90, 25 },
      m_ProfileNameRect{ 292, 90, 220, 19 },
      m_GuildRect{ 292, 85, 174, 26 },
      m_GuildMarkRect{ 294, 102, 12, 12 },
      m_GuildNameRect{ 310, 102, 394, 12 },
      m_CloseRect{ WINDOW_WIDTH - 36, 14, 20, 20 },
      m_RefreshRect{ 435, 61, 59, 23 },
      m_PreviousRect{ 26, 507, 72, 26 },
      m_NextRect{ 148, 507, 72, 26 },
      m_StatsRect{ 239, 472, 495, 28 },
      // Expanded 3D viewport. The old 120x139 viewport clipped large wings
      // and weapons. 240x240 keeps the same center while giving the model
      // almost the whole profile area; the lower default zoom preserves the
      // previous apparent character size.
      m_PreviewRect{ 244, 220, 157, 210 }
{
}

CNewUICustomRanking::~CNewUICustomRanking()
{
    Release();
}

bool CNewUICustomRanking::Create(CNewUIManager* manager, CNewUI3DRenderMng* renderManager)
{
    if (manager == nullptr)
    {
        return false;
    }

    UI::Character::RankingEquipment().Initialize(renderManager);
    UI::Character::RankUser::Load();
    SetLayoutMode(UI::Scaling::LayoutMode::WorldOverlay);
    m_Manager = manager;
    m_Manager->AddUIObj(TOP_RESET_RANKING_INTERFACE_KEY, this);
    LoadImages();
    m_SearchBox = new CUITextInputBox;
    m_SearchBox->Init(g_hWnd, 170, 14, 32);
    m_SearchBox->SetTextColor(245, 245, 245, 255);
    m_SearchBox->SetBackColor(0, 0, 0, 0);
    m_SearchBox->SetFont(g_RankingFont);
    m_SearchBox->SetState(UISTATE_HIDE);
    Show(true);
    Enable(true);
    g_pCustomRankingWindow = this;
    return true;
}

void CNewUICustomRanking::Release()
{
    UI::Character::RankUser::Unload();
    UI::Character::MissionRanking().Release();
    UI::Character::RankingEquipment().Release();
    ReleasePreview();
    SAFE_DELETE(m_SearchBox);
    UnloadImages();

    if (g_pCustomRankingWindow == this)
    {
        g_pCustomRankingWindow = nullptr;
    }

    if (m_Manager != nullptr)
    {
        m_Manager->RemoveUIObj(this);
        m_Manager = nullptr;
    }
}

namespace UI::Character::RankingStyle
{
    inline GLuint Background = BITMAP_UNKNOWN;
}

void CNewUICustomRanking::LoadImages()
{
    UI::Character::GuildRanking().Load();
    UI::Character::MissionRanking().Load();
    g_RankingFooter = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingFooterV17.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    g_RankingColoredWidgets = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingColorsV14.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    g_RankingReferenceSkin = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingReferenceV5.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    g_RankingStatus = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingStatusV11.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
    g_RankingWidgets = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingWidgetsV5.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    g_ParisRankingFrame.Load();
    g_ParisRankingExact.Load();

    for (int i = 0; i < 3; ++i)
    {
        wchar_t path[128];
        swprintf_s(path, L"Data\\Interface\\Paris_RankMedalV17_%d.tga", i + 1);
        g_PodiumImages[i] = Bitmaps.LoadImage(path, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }
    auto font = [&](int size, int weight, const wchar_t* family = L"Georgia") {
        return CreateFontW((std::max)(8, static_cast<int>(size * RankingPhysicalScale(m_PanelScale))),
            0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_ROMAN, family);
    };
    g_RankingValueFont = font(13, FW_BOLD, L"Segoe UI");
    g_RankingFont = font(11, FW_NORMAL, L"Segoe UI");
    g_RankingBold = font(11, FW_BOLD);
    g_RankingTitle = font(26, FW_BOLD);
    g_RankingNameFont = font(18, FW_BOLD);
    UI::Character::RankingStyle::Background = Bitmaps.LoadImage(
        L"Data\\Interface\\Paris_RankingScene.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    bool loaded = true;
    loaded = LoadBitmap(
                 L"Interface\\MuEtroTopReset\\bg500buttom.tga",
                 IMAGE_TOP_RESET_BACKGROUND_BOTTOM,
                 GL_LINEAR,
                 GL_CLAMP_TO_EDGE,
                 false)
        && loaded;
    loaded = LoadBitmap(
                 L"Interface\\MuEtroTopReset\\bg500top.tga",
                 IMAGE_TOP_RESET_BACKGROUND_TOP,
                 GL_LINEAR,
                 GL_CLAMP_TO_EDGE,
                 false)
        && loaded;
    loaded = LoadBitmap(
                 L"Interface\\MuEtroTopReset\\Button.tga",
                 IMAGE_TOP_RESET_BUTTON,
                 GL_LINEAR,
                 GL_CLAMP_TO_EDGE,
                 false)
        && loaded;
    loaded = LoadBitmap(
                 L"Interface\\MuEtroTopReset\\Button_Close.tga",
                 IMAGE_TOP_RESET_CLOSE,
                 GL_LINEAR,
                 GL_CLAMP_TO_EDGE,
                 false)
        && loaded;
    m_ImagesLoaded = loaded;
}

void CNewUICustomRanking::UnloadImages()
{
    UI::Character::GuildRanking().Release();
    if (g_RankingColoredWidgets != BITMAP_UNKNOWN) DeleteBitmap(g_RankingColoredWidgets);
    g_RankingColoredWidgets = BITMAP_UNKNOWN;
    if (g_RankingFooter != BITMAP_UNKNOWN) DeleteBitmap(g_RankingFooter);
    g_RankingFooter = BITMAP_UNKNOWN;
    if (g_RankingReferenceSkin != BITMAP_UNKNOWN) DeleteBitmap(g_RankingReferenceSkin);
    if (g_RankingWidgets != BITMAP_UNKNOWN) DeleteBitmap(g_RankingWidgets);
    if (g_RankingStatus != BITMAP_UNKNOWN) DeleteBitmap(g_RankingStatus);
    g_RankingStatus = BITMAP_UNKNOWN;
    g_RankingReferenceSkin = g_RankingWidgets = BITMAP_UNKNOWN;
    g_ParisRankingFrame.Unload();
    g_ParisRankingExact.Unload();
    if (g_RankingShell != BITMAP_UNKNOWN) DeleteBitmap(g_RankingShell);
    g_RankingShell = BITMAP_UNKNOWN;
    for (auto& image : g_PodiumImages) {if (image != BITMAP_UNKNOWN) DeleteBitmap(image); image = BITMAP_UNKNOWN;}
    g_pRenderText->SetFont(g_hFont);
    for (auto font : {g_RankingFont, g_RankingBold, g_RankingTitle, g_RankingNameFont, g_RankingValueFont}) if (font) DeleteObject(font);
    g_RankingFont = g_RankingBold = g_RankingTitle = g_RankingNameFont = g_RankingValueFont = nullptr;
    if (UI::Character::RankingStyle::Background != BITMAP_UNKNOWN)
        DeleteBitmap(UI::Character::RankingStyle::Background);
    UI::Character::RankingStyle::Background = BITMAP_UNKNOWN;
    DeleteBitmap(IMAGE_TOP_RESET_CLOSE);
    DeleteBitmap(IMAGE_TOP_RESET_BUTTON);
    DeleteBitmap(IMAGE_TOP_RESET_BACKGROUND_TOP);
    DeleteBitmap(IMAGE_TOP_RESET_BACKGROUND_BOTTOM);
    m_ImagesLoaded = false;
}

void CNewUICustomRanking::PositionWindow()
{
    m_PreviousRect.Y = m_NextRect.Y = m_Tab == TAB_GUILD ? 489 : 453;
    m_NextRect.X = m_Tab == TAB_GUILD ? 114 : 148;
    const int width = GetWindowWidth();
    const int height = GetWindowHeight();
    m_Position.x = std::max<LONG>(0, ((640 - width) / 2) + m_PanelOffsetX);
    m_Position.y = std::max<LONG>(0, ((440 - height) / 2) + m_PanelOffsetY);
    m_Position.x = std::min<LONG>(m_Position.x, 640 - width);
    m_Position.y = std::min<LONG>(m_Position.y, 440 - height);
}

int CNewUICustomRanking::ScaleValue(int value) const
{
    return UI::Character::PanelAspect::Axis(value, RankingPhysicalScale(m_PanelScale), g_fScreenRate_y);
}

int CNewUICustomRanking::GetWindowWidth() const
{
    return std::max(1, UI::Character::PanelAspect::Axis(WINDOW_WIDTH, RankingPhysicalScale(m_PanelScale), g_fScreenRate_x));
}

int CNewUICustomRanking::GetWindowHeight() const
{
    return std::max(1, ScaleValue(WINDOW_HEIGHT));
}

CNewUICustomRanking::UiRect CNewUICustomRanking::GetListLayoutRect(const UiRect& rect) const
{
    UiRect layout = rect;
    if (m_Tab == TAB_GUILD && rect.X < 230 && rect.Y < 440)
    {
        constexpr float GuildListWidthRatio = 174.f / 212.f;
        layout.X = 19 + static_cast<int>((rect.X - 16) * GuildListWidthRatio);
        layout.Width = rect.Width == 17 ? 17 : std::max(1, static_cast<int>(rect.Width * GuildListWidthRatio));
    }
    return layout;
}

CNewUICustomRanking::UiRect CNewUICustomRanking::GetAbsoluteRect(const UiRect& rect) const
{
    const UiRect layout = GetListLayoutRect(rect);
    return {
        m_Position.x + UI::Character::PanelAspect::Axis(layout.X, RankingPhysicalScale(m_PanelScale), g_fScreenRate_x),
        m_Position.y + ScaleValue(rect.Y),
        std::max(1, UI::Character::PanelAspect::Axis(layout.Width, RankingPhysicalScale(m_PanelScale), g_fScreenRate_x)),
        std::max(1, ScaleValue(rect.Height)),
    };
}

void CNewUICustomRanking::OpenWindow()
{
    if (SceneFlag != MAIN_SCENE || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        return;
    }



    PositionWindow();
    SEASON3B::CParisCustomMenuWindow::CloseAllOverlays(this);
    m_WindowOpen = true;
    UI::Character::RankingEquipment().RefreshOwnPreference();
    m_SearchBox->SetText(L"");
    m_SearchBox->SetState(UISTATE_NORMAL);
    m_SearchEntryCount = -1;
    m_Tab = TAB_RESET;
    m_Page = 0;
    m_SelectedIndex = -1;
    const std::wstring owner = Hero ? Hero->ID : L"";
    if (g_RankingCacheOwner != owner)
    {
    m_Entries.clear();
    m_GuildEntries.clear();
    m_QuestEntries.clear();
    m_ListCacheTick.fill(0);
    m_ListCacheReady.fill(false);
    UI::Character::GuildRanking().Clear();
    UI::Character::MissionRanking().Clear();
        UI::Character::RankingHover::Clear();
        g_RankingCacheOwner = owner;
    }
    ClearProfileState();
    if (!m_ListCacheReady[TAB_RESET] || GetTickCount() - m_ListCacheTick[TAB_RESET] >= ListCacheMilliseconds) RequestList();
    else if (!m_Entries.empty()) RequestCharacter(0);
    PlayBuffer(SOUND_CLICK01);
}

void CNewUICustomRanking::CloseWindow(bool playSound)
{
    if (!m_WindowOpen)
    {
        return;
    }

    m_WindowOpen = false;
    UI::Character::RankingEquipment().Clear();
    if (m_SearchBox) m_SearchBox->SetState(UISTATE_HIDE);
    m_ListPending = false;
    m_ProfilePending = false;
    ReleasePreview();

    if (playSound)
    {
        PlayBuffer(SOUND_CLICK01);
    }
}

bool CNewUICustomRanking::IsWindowOpen() const
{
    return m_WindowOpen;
}

bool CNewUICustomRanking::IsVisible() const
{
    return CNewUIObj::IsVisible() && m_WindowOpen;
}

float CNewUICustomRanking::GetLayerDepth()
{
    return 5.0f;
}

float CNewUICustomRanking::GetKeyEventOrder()
{
    return 5.0f;
}

void CNewUICustomRanking::RequestList()
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        return;
    }

    BYTE subCode = RankingListRequest;
    if (m_Tab == TAB_GUILD)
    {
        subCode = RankingGuildListRequest;
    }
    else if (m_Tab == TAB_QUEST)
    {
        subCode = RankingQuestListRequest;
    }
    BYTE packet[5] = { 0xC1, 5, RankingHeadCode, subCode, RankingProtocolVersion };
    SocketClient->Send(packet, sizeof(packet));
    m_SearchEntryCount = -1;
    m_ListPending = true;
    m_ListRequestTick = GetTickCount();
}

void CNewUICustomRanking::RequestCharacter(int index)
{
    if (index < 0
        || index >= GetEntryCount()
        || SocketClient == nullptr
        || !SocketClient->IsConnected())
    {
        return;
    }

    BYTE subCode = RankingCharacterRequest;
    if (m_Tab == TAB_GUILD)
    {
        subCode = RankingGuildLeaderRequest;
    }
    else if (m_Tab == TAB_QUEST)
    {
        subCode = RankingQuestCharacterRequest;
    }

    BYTE packet[15] = {
        0xC1,
        15,
        RankingHeadCode,
        subCode,
        RankingProtocolVersion,
    };

    if (m_Tab == TAB_GUILD)
    {
        std::memcpy(packet + 5, m_GuildEntries[index].GuildName.data(), GuildNameLength);
    }
    else if (m_Tab == TAB_RESET)
    {
        std::memcpy(packet + 5, m_Entries[index].Name.data(), CharacterNameLength);
    }
    else
    {
        std::memcpy(packet + 5, m_QuestEntries[index].Name.data(), CharacterNameLength);
    }

    SocketClient->Send(packet, sizeof(packet));
    m_SelectedIndex = index;
    ClearProfileState();
    m_ProfilePending = true;
    m_ProfileRequestTick = GetTickCount();
}


void CNewUICustomRanking::ClearProfileState()
{
    UI::Character::RankingEquipment().Clear();
    m_ProfileName.clear();
    m_GuildName.clear();
    m_GuildNameUtf8.fill(0);
    m_GuildMark.fill(0);
    m_LastProfilePacketSize = 0;
    m_LastProfilePayloadOffset = 0;
    m_GuildMarkNonZeroBytes = 0;
    m_GuildMarkNonZeroPixels = 0;
    m_GuildMarkByteSum = 0;
    m_Level = 0;
    m_Resets = 0;
    m_TotalStats = 0;
    m_GuildScore = 0;
    m_GuildMemberCount = 0;
    m_QuestCompleted = 0;
    m_QuestTotal = 0;
    ReleasePreview();
}

void CNewUICustomRanking::SwitchTab(RankingTab tab)
{
    if (m_Tab == tab)
    {
        if (!m_ListPending) RequestList();
        return;
    }

    m_Tab = tab;
    m_SearchEntryCount = -1;
    m_Page = 0;
    m_SelectedIndex = -1;
    m_ListPending = false;
    m_ProfilePending = false;
    ClearProfileState();

    RestoreTabList();
    PlayBuffer(SOUND_CLICK01);
}

void CNewUICustomRanking::RestoreTabList()
{
    if (m_Tab == TAB_QUEST)
    {
        UI::Character::MissionRanking().Open();
        if (!m_ListCacheReady[TAB_QUEST] || GetTickCount() - m_ListCacheTick[TAB_QUEST] >= ListCacheMilliseconds) RequestList();
        return;
    }
    const auto tab = static_cast<std::size_t>(m_Tab);
    const bool fresh = m_ListCacheReady[tab] && GetTickCount() - m_ListCacheTick[tab] < ListCacheMilliseconds;
    if (GetEntryCount() > 0) RequestCharacter(0);
    if (!fresh) RequestList();
}

int CNewUICustomRanking::GetEntryCount() const
{
    if (m_Tab == TAB_GUILD)
    {
        return static_cast<int>(m_GuildEntries.size());
    }

    if (m_Tab == TAB_QUEST)
    {
        return static_cast<int>(m_QuestEntries.size());
    }

    return static_cast<int>(m_Entries.size());
}

void CNewUICustomRanking::ReceivePacket(const BYTE* packet, int size)
{
    if (packet == nullptr || size < 5)
    {
        return;
    }

    if (UI::Character::RankingHover::Receive(packet, size)) return;
    if (UI::Character::MissionRanking().Receive(packet, size)) return;
    if (UI::Character::RankingEquipment().Receive(packet, size)) return;
    if (UI::Character::GuildRanking().Receive(packet,size)) return;

    int headOffset = 2;
    int subOffset = 3;
    int payloadOffset = 4;
    int declaredSize = packet[1];

    if (packet[0] == 0xC2 || packet[0] == 0xC4)
    {
        if (size < 6)
        {
            return;
        }

        headOffset = 3;
        subOffset = 4;
        payloadOffset = 5;
        declaredSize = (static_cast<int>(packet[1]) << 8) | packet[2];
    }

    if (packet[headOffset] != RankingHeadCode || declaredSize > size || declaredSize <= payloadOffset)
    {
        return;
    }

    switch (packet[subOffset])
    {
    case RankingListResponse:
        ReceiveList(packet, declaredSize, payloadOffset);
        break;
    case RankingCharacterResponse:
        ReceiveCharacter(packet, declaredSize, payloadOffset);
        break;
    case RankingGuildListResponse:
        ReceiveGuildList(packet, declaredSize, payloadOffset);
        break;
    case RankingGuildLeaderResponse:
        ReceiveGuildLeader(packet, declaredSize, payloadOffset);
        break;
    case RankingQuestListResponse:
        ReceiveQuestList(packet, declaredSize, payloadOffset);
        break;
    case RankingQuestCharacterResponse:
        ReceiveQuestCharacter(packet, declaredSize, payloadOffset);
        break;
    default:
        break;
    }
}

void CNewUICustomRanking::ReceiveList(const BYTE* packet, int size, int payloadOffset)
{
    m_SearchEntryCount = -1;
    if (!m_WindowOpen
        || m_Tab != TAB_RESET
        || size < payloadOffset + 2
        || packet[payloadOffset] != RankingProtocolVersion)
    {
        return;
    }

    const int count = std::min<int>(packet[payloadOffset + 1], MAXIMUM_ENTRIES);
    const int entriesOffset = payloadOffset + 2;
    if (size < entriesOffset + (count * RankingEntryLength))
    {
        return;
    }

    m_Entries.clear();
    m_Entries.reserve(count);
    for (int index = 0; index < count; ++index)
    {
        const int offset = entriesOffset + (index * RankingEntryLength);
        RankingEntry entry;
        std::memcpy(entry.Name.data(), packet + offset, CharacterNameLength);
        entry.Name[CharacterNameLength] = '\0';
        entry.Resets = ReadInt32(packet + offset + CharacterNameLength);
        m_Entries.push_back(entry);
    }

    m_ListCacheTick[TAB_RESET] = GetTickCount();
    m_ListCacheReady[TAB_RESET] = true;
    m_ListPending = false;
    m_Page = 0;
    if (!m_Entries.empty())
    {
        RequestCharacter(0);
    }
}

void CNewUICustomRanking::ReceiveCharacter(const BYTE* packet, int size, int payloadOffset)
{
    constexpr int PayloadLength = 93;
    if (!m_WindowOpen
        || m_Tab != TAB_RESET
        || !m_ProfilePending
        || size < payloadOffset + PayloadLength
        || packet[payloadOffset] != RankingProtocolVersion)
    {
        return;
    }

    m_ProfilePending = false;
    m_LastProfilePacketSize = size;
    m_LastProfilePayloadOffset = payloadOffset;
    if (packet[payloadOffset + 1] == 0)
    {
        return;
    }

    const int base = payloadOffset + 2;
    m_ProfileName = Utf8ToWide(reinterpret_cast<const char*>(packet + base), CharacterNameLength);
    m_GuildName = Utf8ToWide(reinterpret_cast<const char*>(packet + base + 10), GuildNameLength);
    std::memcpy(m_GuildNameUtf8.data(), packet + base + 10, GuildNameLength);
    m_GuildNameUtf8[GuildNameLength] = '\0';
    std::memcpy(m_GuildMark.data(), packet + base + 20, m_GuildMark.size());

    // MUETRO_TOP_RESET_GUILD_MARK_DIAGNOSTIC_V1_17
    // Keep the diagnostic entirely client-side: it reports exactly what the
    // ED:55 packet delivered before GuildCache or the 2D renderer touches it.
    m_GuildMarkNonZeroBytes = 0;
    m_GuildMarkNonZeroPixels = 0;
    m_GuildMarkByteSum = 0;
    for (const BYTE value : m_GuildMark)
    {
        if (value != 0)
        {
            ++m_GuildMarkNonZeroBytes;
        }

        if ((value & 0xF0) != 0)
        {
            ++m_GuildMarkNonZeroPixels;
        }
        if ((value & 0x0F) != 0)
        {
            ++m_GuildMarkNonZeroPixels;
        }

        m_GuildMarkByteSum += value;
    }

    m_Level = ReadInt32(packet + base + 52);
    m_Resets = ReadInt32(packet + base + 56);
    m_TotalStats = ReadInt32(packet + base + 60);
    BuildPreview(packet + base + 64);
    UpdateGuildMark();
    UI::Character::RankingEquipment().RequestProfile(m_ProfileName);
}


void CNewUICustomRanking::ReceiveGuildList(const BYTE* packet, int size, int payloadOffset)
{
    m_SearchEntryCount = -1;
    if (!m_WindowOpen
        || m_Tab != TAB_GUILD
        || size < payloadOffset + 2
        || packet[payloadOffset] != RankingProtocolVersion)
    {
        return;
    }

    const int count = std::min<int>(packet[payloadOffset + 1], MAXIMUM_ENTRIES);
    const int entriesOffset = payloadOffset + 2;
    if (size < entriesOffset + (count * GuildRankingEntryLength))
    {
        return;
    }

    m_GuildEntries.clear();
    m_GuildEntries.reserve(count);
    for (int index = 0; index < count; ++index)
    {
        const int offset = entriesOffset + (index * GuildRankingEntryLength);
        GuildRankingEntry entry;
        std::memcpy(entry.GuildName.data(), packet + offset, GuildNameLength);
        entry.GuildName[GuildNameLength] = '\0';
        std::memcpy(entry.LeaderName.data(), packet + offset + GuildNameLength, CharacterNameLength);
        entry.LeaderName[CharacterNameLength] = '\0';
        entry.Score = ReadInt32(packet + offset + GuildNameLength + CharacterNameLength);
        entry.MemberCount = ReadInt32(packet + offset + GuildNameLength + CharacterNameLength + 4);
        m_GuildEntries.push_back(entry);
    }

    m_ListCacheTick[TAB_GUILD] = GetTickCount();
    m_ListCacheReady[TAB_GUILD] = true;
    m_ListPending = false;
    m_Page = 0;
    if (!m_GuildEntries.empty())
    {
        RequestCharacter(0);
    }
}

void CNewUICustomRanking::ReceiveGuildLeader(const BYTE* packet, int size, int payloadOffset)
{
    constexpr int PayloadLength = 89;
    if (!m_WindowOpen
        || m_Tab != TAB_GUILD
        || !m_ProfilePending
        || size < payloadOffset + PayloadLength
        || packet[payloadOffset] != RankingProtocolVersion)
    {
        return;
    }

    if(m_SelectedIndex<0 || m_SelectedIndex>=static_cast<int>(m_GuildEntries.size())
        || std::memcmp(packet+payloadOffset+2,m_GuildEntries[m_SelectedIndex].GuildName.data(),GuildNameLength)!=0)return;
    m_ProfilePending = false;
    m_LastProfilePacketSize = size;
    m_LastProfilePayloadOffset = payloadOffset;
    if (packet[payloadOffset + 1] == 0)
    {
        return;
    }

    const int base = payloadOffset + 2;
    m_GuildName = Utf8ToWide(reinterpret_cast<const char*>(packet + base), GuildNameLength);
    m_ProfileName = Utf8ToWide(reinterpret_cast<const char*>(packet + base + 10), CharacterNameLength);
    std::memcpy(m_GuildNameUtf8.data(), packet + base, GuildNameLength);
    m_GuildNameUtf8[GuildNameLength] = '\0';
    std::memcpy(m_GuildMark.data(), packet + base + 20, m_GuildMark.size());

    m_GuildMarkNonZeroBytes = 0;
    m_GuildMarkNonZeroPixels = 0;
    m_GuildMarkByteSum = 0;
    for (const BYTE value : m_GuildMark)
    {
        if (value != 0)
        {
            ++m_GuildMarkNonZeroBytes;
        }

        if ((value & 0xF0) != 0)
        {
            ++m_GuildMarkNonZeroPixels;
        }
        if ((value & 0x0F) != 0)
        {
            ++m_GuildMarkNonZeroPixels;
        }

        m_GuildMarkByteSum += value;
    }

    m_GuildScore = ReadInt32(packet + base + 52);
    m_GuildMemberCount = ReadInt32(packet + base + 56);
    UI::Character::GuildRanking().Request(m_GuildName);
}

void CNewUICustomRanking::ReceiveQuestList(const BYTE* packet, int size, int payloadOffset)
{
    m_SearchEntryCount = -1;
    if (!m_WindowOpen
        || m_Tab != TAB_QUEST
        || size < payloadOffset + 2
        || packet[payloadOffset] != RankingProtocolVersion)
    {
        return;
    }

    const int count = std::min<int>(packet[payloadOffset + 1], MAXIMUM_ENTRIES);
    const int entriesOffset = payloadOffset + 2;
    if (size < entriesOffset + (count * QuestRankingEntryLength))
    {
        return;
    }

    m_QuestEntries.clear();
    m_QuestEntries.reserve(count);
    for (int index = 0; index < count; ++index)
    {
        const int offset = entriesOffset + (index * QuestRankingEntryLength);
        QuestRankingEntry entry;
        std::memcpy(entry.Name.data(), packet + offset, CharacterNameLength);
        entry.Name[CharacterNameLength] = '\0';
        entry.CompletedQuests = ReadInt32(packet + offset + CharacterNameLength);
        entry.TotalQuests = ReadInt32(packet + offset + CharacterNameLength + 4);
        m_QuestEntries.push_back(entry);
    }

    m_ListCacheTick[TAB_QUEST] = GetTickCount();
    m_ListCacheReady[TAB_QUEST] = true;
    m_ListPending = false;
    m_Page = 0;
    int rank = 0;
    if (Hero) for (int i = 0; i < static_cast<int>(m_QuestEntries.size()); ++i)
        if (Utf8ToWide(m_QuestEntries[i].Name.data(), CharacterNameLength) == Hero->ID) { rank = i + 1; break; }
    UI::Character::MissionRanking().SetGlobalRank(rank);
    UI::Character::MissionRanking().ReceiveGlobal(packet, size, payloadOffset);
}

void CNewUICustomRanking::ReceiveQuestCharacter(const BYTE* packet, int size, int payloadOffset)
{
    constexpr int PayloadLength = 93;
    if (!m_WindowOpen
        || m_Tab != TAB_QUEST
        || !m_ProfilePending
        || size < payloadOffset + PayloadLength
        || packet[payloadOffset] != RankingProtocolVersion)
    {
        return;
    }

    m_ProfilePending = false;
    m_LastProfilePacketSize = size;
    m_LastProfilePayloadOffset = payloadOffset;
    if (packet[payloadOffset + 1] == 0)
    {
        return;
    }

    const int base = payloadOffset + 2;
    m_ProfileName = Utf8ToWide(reinterpret_cast<const char*>(packet + base), CharacterNameLength);
    m_GuildName = Utf8ToWide(reinterpret_cast<const char*>(packet + base + 10), GuildNameLength);
    std::memcpy(m_GuildNameUtf8.data(), packet + base + 10, GuildNameLength);
    m_GuildNameUtf8[GuildNameLength] = '\0';
    std::memcpy(m_GuildMark.data(), packet + base + 20, m_GuildMark.size());

    m_GuildMarkNonZeroBytes = 0;
    m_GuildMarkNonZeroPixels = 0;
    m_GuildMarkByteSum = 0;
    for (const BYTE value : m_GuildMark)
    {
        if (value != 0)
        {
            ++m_GuildMarkNonZeroBytes;
        }

        if ((value & 0xF0) != 0)
        {
            ++m_GuildMarkNonZeroPixels;
        }
        if ((value & 0x0F) != 0)
        {
            ++m_GuildMarkNonZeroPixels;
        }

        m_GuildMarkByteSum += value;
    }

    m_QuestCompleted = ReadInt32(packet + base + 52);
    m_QuestTotal = ReadInt32(packet + base + 56);
    m_Level = ReadInt32(packet + base + 60);
    BuildPreview(packet + base + 64);
    UpdateGuildMark();
    UI::Character::RankingEquipment().RequestProfile(m_ProfileName);
}

void CNewUICustomRanking::BuildPreview(const BYTE* appearance)
{
    ReleasePreview();
    if (appearance == nullptr || Hero == nullptr)
    {
        return;
    }

    BYTE equipment[25] = {};
    std::memcpy(equipment, appearance + 2, sizeof(equipment));

    // Dark Spirit's legacy decoder reads CharacterMachine (the local hero)
    // and would attach the wrong pet data to this detached preview.
    for (int hand = 0; hand < 2; ++hand)
    {
        BYTE* item = equipment + (hand * 3);
        if (IsDarkSpirit(item))
        {
            item[0] = item[1] = item[2] = 0xFF;
        }
    }

    // Floating pets are owned by a world-global process. Keep mounts private,
    // but suppress floating pets so opening Ranking never leaks a world object.
    if (equipment[23] != 0xFF && equipment[24] != 0xFF)
    {
        const short number = equipment[24] + ((equipment[23] & 0x0F) << 8);
        const BYTE group = (equipment[23] & 0xF0) >> 4;
        const int modelType = MODEL_ITEM + (group * MAX_ITEM_INDEX) + (number & (MAX_ITEM_INDEX - 1));
        if (IsFloatingPetModel(modelType))
        {
            equipment[23] = equipment[24] = 0xFF;
        }
    }

    m_Preview.Initialize();
    m_PreviewHelper.Initialize();
    CreateCharacterPointer(
        &m_Preview,
        MODEL_PLAYER,
        Hero->PositionX,
        Hero->PositionY,
        0.0f);
    if (m_Preview.Object.BoneTransform == nullptr)
    {
        return;
    }

    const auto serverClass = static_cast<SERVER_CLASS_TYPE>(appearance[0]);
    const CLASS_TYPE clientClass = gCharacterManager.ChangeServerClassTypeToClientClassType(serverClass);
    m_Preview.Key = -29001;
    m_Preview.Class = clientClass;
    m_Preview.SkinIndex = gCharacterManager.GetSkinModelIndex(clientClass);
    m_Preview.Skin = 0;
    m_Preview.HideShadow = true;
    m_Preview.SafeZone = false;
    m_Preview.Dead = 0;
    m_Preview.Movement = false;
    m_Preview.GuildMarkIndex = -1;
    m_Preview.Object.Live = true;
    m_Preview.Object.Visible = true;
    m_Preview.Object.EnableShadow = false;
    m_Preview.Object.m_bRenderShadow = false;
    Vector(-300.0f, -300.0f, -300.0f, m_Preview.Object.Position);
    Vector(1.0f, 1.0f, 1.0f, m_Preview.Object.Light);
    Vector(1.0f, 1.0f, 1.0f, m_Preview.Light);

    // SetChangeClass in this MuMain revision does not calculate SkinIndex, so
    // it must be assigned above before the base body models are rebuilt.
    SetChangeClass(&m_Preview);
    ReadEquipmentExtended(0, appearance[1], equipment, &m_Preview, &m_PreviewHelper);
    SetPlayerStop(&m_Preview);

    m_PreviewHelperScale = 0.0f;
    if (m_PreviewHelper.Live && m_Preview.Object.Scale > 0.0f)
    {
        m_PreviewHelperScale = m_PreviewHelper.Scale * 0.7f / m_Preview.Object.Scale;
    }

    mu_swprintf(m_Preview.ID, L"%ls", m_ProfileName.c_str());
    m_HasPreview = true;
}

void CNewUICustomRanking::ReleasePreview()
{
    m_InspectionSignature = 0;
    if (m_Preview.Object.BoneTransform != nullptr)
    {
        DeleteCloth(&m_Preview, &m_Preview.Object);
        DeleteParts(&m_Preview);
        giPetManager::DeletePet(&m_Preview);
        delete[] m_Preview.Object.BoneTransform;
        m_Preview.Object.BoneTransform = nullptr;
    }

    if (m_PreviewHelper.BoneTransform != nullptr)
    {
        delete[] m_PreviewHelper.BoneTransform;
        m_PreviewHelper.BoneTransform = nullptr;
    }

    m_Preview.Object.Live = false;
    m_PreviewHelper.Live = false;
    m_HasPreview = false;
    m_PreviewHelperScale = 0.0f;
    m_GuildMarkIndex = -1;
}

void CNewUICustomRanking::BuildInspectionCharacter(const CHARACTER& source)
{
    ReleasePreview();
    m_Preview.Initialize();
    m_PreviewHelper.Initialize();
    CreateCharacterPointer(&m_Preview, MODEL_PLAYER, source.PositionX, source.PositionY, 0.f);
    if (!m_Preview.Object.BoneTransform) return;
    m_Preview.Class = source.Class;
    m_Preview.SkinIndex = gCharacterManager.GetSkinModelIndex(source.Class);
    m_Preview.Key = -29001;
    SetChangeClass(&m_Preview);
    const PART_t* parts[]{&source.BodyPart[0], &source.BodyPart[1], &source.BodyPart[2],
        &source.BodyPart[3], &source.BodyPart[4], &source.Weapon[0], &source.Weapon[1], &source.Wing};
    PART_t* copies[]{&m_Preview.BodyPart[0], &m_Preview.BodyPart[1], &m_Preview.BodyPart[2],
        &m_Preview.BodyPart[3], &m_Preview.BodyPart[4], &m_Preview.Weapon[0], &m_Preview.Weapon[1], &m_Preview.Wing};
    for (int i = 0; i < 8; ++i)
    {
        copies[i]->Type = parts[i]->Type;
        copies[i]->Level = parts[i]->Level;
        copies[i]->ExcellentFlags = parts[i]->ExcellentFlags;
        copies[i]->AncientDiscriminator = parts[i]->AncientDiscriminator;
        copies[i]->LinkBone = parts[i]->LinkBone;
    }
    m_Preview.Helper.Type = -1;
    m_Preview.GuildMarkIndex = -1;
    m_Preview.Object.Live = m_Preview.Object.Visible = true;
    m_Preview.Object.EnableShadow = m_Preview.Object.m_bRenderShadow = false;
    Vector(-300.f, -300.f, -300.f, m_Preview.Object.Position);
    SetPlayerStop(&m_Preview);
    m_HasPreview = true;
}

void CNewUICustomRanking::RenderInspectionCharacter(const CHARACTER& source, int x, int y, int width, int height)
{
    if (m_WindowOpen || !source.Object.Live) return;
    const PART_t* parts[]{&source.BodyPart[0], &source.BodyPart[1], &source.BodyPart[2],
        &source.BodyPart[3], &source.BodyPart[4], &source.Weapon[0], &source.Weapon[1], &source.Wing};
    std::uint64_t signature = static_cast<unsigned short>(source.Key) + 1;
    signature = signature * 16777619u + source.Class;
    for (const auto* part : parts)
        signature = signature * 16777619u + static_cast<unsigned short>(part->Type)
            + (static_cast<std::uint64_t>(part->Level) << 16) + (static_cast<std::uint64_t>(part->ExcellentFlags) << 24)
            + (static_cast<std::uint64_t>(part->AncientDiscriminator) << 32);
    if (!m_HasPreview || m_InspectionSignature != signature)
    {
        BuildInspectionCharacter(source);
        m_InspectionSignature = signature;
    }
    UI::Paris::Clip(x, y, width, height);
    const float previousZoom = m_PreviewZoom;
    m_PreviewZoom = .45f;
    constexpr float FeetMarginOffset = 24.f;
    RenderPreview(x, y, width, height, FeetMarginOffset);
    m_PreviewZoom = previousZoom;
    UI::Paris::ResetClip();
}

void CNewUICustomRanking::UpdateGuildMark()
{
    if (m_GuildName.empty()
        || std::all_of(m_GuildMark.begin(), m_GuildMark.end(), [](BYTE value) { return value == 0; }))
    {
        m_GuildMarkIndex = -1;
        m_Preview.GuildMarkIndex = -1;
        return;
    }

    m_GuildMarkIndex = g_GuildCache.GetGuildMarkIndex(SyntheticGuildKey);
    if (m_GuildMarkIndex < 0)
    {
        m_GuildMarkIndex = g_GuildCache.MakeGuildMarkIndex(SyntheticGuildKey);
    }

    if (m_GuildMarkIndex >= 0)
    {
        char emptyUnion[GuildNameLength + 1] = {};
        g_GuildCache.SetGuildMark(
            SyntheticGuildKey,
            emptyUnion,
            m_GuildNameUtf8.data(),
            m_GuildMark.data());
        m_Preview.GuildMarkIndex = static_cast<short>(m_GuildMarkIndex);
        ::CreateGuildMark(m_GuildMarkIndex, false);
    }
}

bool CNewUICustomRanking::Update()
{
    if (SceneFlag != MAIN_SCENE || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        CloseWindow(false);
        return true;
    }

    if (!m_WindowOpen)
    {
        return true;
    }

    HandleEscape();
    if (!m_WindowOpen) return true;
    PositionWindow();
    UpdateSearch();
    const DWORD now = GetTickCount();
    if (!m_ListPending && m_ListCacheReady[m_Tab] && now - m_ListCacheTick[m_Tab] >= ListCacheMilliseconds) RequestList();
    if (m_Tab == TAB_QUEST) UI::Character::MissionRanking().Open();
    if (m_ListPending && now - m_ListRequestTick >= ListTimeoutMilliseconds)
    {
        m_ListPending = false;
    }

    if (m_ProfilePending && now - m_ProfileRequestTick >= ProfileTimeoutMilliseconds)
    {
        m_ProfilePending = false;
    }

    return true;
}

bool CNewUICustomRanking::UpdateKeyEvent()
{
    if (HandleEscape()) return false;
    if (CloseForMenuHotKey()) return true;
    return true;
}

bool CNewUICustomRanking::HandleEscape()
{
    // Native edit controls swallow Escape, so read its state while they own focus.
    const bool pressed = SEASON3B::IsPress(VK_ESCAPE) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (!pressed) {m_EscapeHeld = false; return false;}
    if (!m_WindowOpen) return false;
    if (m_EscapeHeld) return true;
    m_EscapeHeld = true;
    if (m_SearchBox && m_SearchBox->HaveFocus())
    {
        SetFocus(g_hWnd);
        g_dwKeyFocusUIID = 0;
    }
    else CloseWindow();
    return true;
}

bool CNewUICustomRanking::CloseForMenuHotKey()
{
    if (!m_WindowOpen || (m_SearchBox && m_SearchBox->HaveFocus())
        || (g_pChatInputBox && g_pChatInputBox->HaveFocus())
        || SEASON3B::IsRepeat(VK_CONTROL) || SEASON3B::IsRepeat(VK_MENU)) return false;
    constexpr int menuKeys[] = {'C','V','I','F','T','P','G','U','D','M','X','B','Z','Y','L','O',VK_F1,VK_HOME,VK_TAB};
    for (const int key : menuKeys)
    {
        if (!SEASON3B::IsPress(key)) continue;
        CloseWindow();
        return true;
    }
    return false;
}

void CNewUICustomRanking::UpdateSearch()
{
    const UiRect input = GetAbsoluteRect(m_Tab == TAB_QUEST ? UiRect{26,124,244,24} : UiRect{26,106,192,24});
    m_SearchBox->SetPosition(input.X + ScaleValue(28), input.Y + ScaleValue(2));
    m_SearchBox->SetSize(input.Width - ScaleValue(35), ScaleValue(18));
    wchar_t text[33]{}; m_SearchBox->GetText(text, 33);
    std::wstring query(text);
    std::transform(query.begin(), query.end(), query.begin(), towlower);
    if (query == m_SearchText && m_SearchEntryCount == GetEntryCount()) return;
    m_SearchText = query; m_SearchEntryCount = GetEntryCount(); m_Page = 0;
    m_FilteredEntries.clear();
    for (int i = 0; i < GetEntryCount(); ++i)
    {
        const char* name = m_Tab == TAB_RESET ? m_Entries[i].Name.data()
            : m_Tab == TAB_GUILD ? m_GuildEntries[i].GuildName.data() : m_QuestEntries[i].Name.data();
        auto wide = Utf8ToWide(name, CharacterNameLength);
        std::transform(wide.begin(), wide.end(), wide.begin(), towlower);
        if (wide.find(query) != std::wstring::npos) m_FilteredEntries.push_back(i);
    }
}

void CNewUICustomRanking::UpdateDebugElement()
{
    if (SEASON3B::IsPress(VK_TAB))
    {
        m_DebugElement = (m_DebugElement + 1) % DEBUG_ELEMENT_COUNT;
        PlayBuffer(SOUND_CLICK01);
        return;
    }

    if (SEASON3B::IsPress(VK_HOME))
    {
        ResetDebugElement();
        return;
    }

    // Whole-panel calibration. The panel remains centered according to its
    // current scale, then OFFSET X/Y is applied by PositionWindow().
    if (m_DebugElement == DEBUG_PANEL)
    {
        if (SEASON3B::IsPress(VK_LEFT)) --m_PanelOffsetX;
        else if (SEASON3B::IsPress(VK_RIGHT)) ++m_PanelOffsetX;
        else if (SEASON3B::IsPress(VK_UP)) --m_PanelOffsetY;
        else if (SEASON3B::IsPress(VK_DOWN)) ++m_PanelOffsetY;
        else if (SEASON3B::IsPress(VK_PRIOR))
            m_PanelScale = std::min(1.20f, m_PanelScale + 0.01f);
        else if (SEASON3B::IsPress(VK_NEXT))
            m_PanelScale = std::max(0.35f, m_PanelScale - 0.01f);

        PositionWindow();
        return;
    }

    // Existing complete-column movement is preserved.
    if (m_DebugElement == DEBUG_CHARACTERS || m_DebugElement == DEBUG_RESETS)
    {
        int* offsetX = m_DebugElement == DEBUG_CHARACTERS
            ? &m_CharacterColumnOffsetX
            : &m_ResetColumnOffsetX;
        int* offsetY = m_DebugElement == DEBUG_CHARACTERS
            ? &m_CharacterColumnOffsetY
            : &m_ResetColumnOffsetY;

        if (SEASON3B::IsPress(VK_LEFT)) --(*offsetX);
        else if (SEASON3B::IsPress(VK_RIGHT)) ++(*offsetX);
        else if (SEASON3B::IsPress(VK_UP)) --(*offsetY);
        else if (SEASON3B::IsPress(VK_DOWN)) ++(*offsetY);
        return;
    }

    // Fine calibration for the row values only. This is intentionally
    // independent from the headers and ranking-number column.
    if (m_DebugElement == DEBUG_CHARACTER_NAMES || m_DebugElement == DEBUG_RESET_VALUES)
    {
        int* offsetX = m_DebugElement == DEBUG_CHARACTER_NAMES
            ? &m_CharacterNameOffsetX
            : &m_ResetValueOffsetX;
        int* offsetY = m_DebugElement == DEBUG_CHARACTER_NAMES
            ? &m_CharacterNameOffsetY
            : &m_ResetValueOffsetY;

        if (SEASON3B::IsPress(VK_LEFT)) --(*offsetX);
        else if (SEASON3B::IsPress(VK_RIGHT)) ++(*offsetX);
        else if (SEASON3B::IsPress(VK_UP)) --(*offsetY);
        else if (SEASON3B::IsPress(VK_DOWN)) ++(*offsetY);
        return;
    }

    // If the first rows align but the lower rows drift, adjust only the
    // vertical distance between rows instead of moving every label.
    if (m_DebugElement == DEBUG_ROW_SPACING)
    {
        if (SEASON3B::IsPress(VK_UP) || SEASON3B::IsPress(VK_LEFT))
            m_RowSpacingAdjustment = std::max(-10, m_RowSpacingAdjustment - 1);
        else if (SEASON3B::IsPress(VK_DOWN) || SEASON3B::IsPress(VK_RIGHT))
            m_RowSpacingAdjustment = std::min(10, m_RowSpacingAdjustment + 1);
        return;
    }

    UiRect* rectangle = nullptr;
    switch (m_DebugElement)
    {
    case DEBUG_PREVIEW:
        rectangle = &m_PreviewRect;
        break;
    case DEBUG_PREVIOUS:
        rectangle = &m_PreviousRect;
        break;
    case DEBUG_NEXT:
        rectangle = &m_NextRect;
        break;
    case DEBUG_TITLE:
        rectangle = &m_TitleRect;
        break;
    case DEBUG_STATS:
        rectangle = &m_StatsRect;
        break;
    default:
        return;
    }

    if (m_DebugElement == DEBUG_PREVIEW)
    {
        if (SEASON3B::IsPress(VK_PRIOR))
        {
            m_PreviewZoom = std::min(1.50f, m_PreviewZoom + 0.02f);
            return;
        }
        if (SEASON3B::IsPress(VK_NEXT))
        {
            m_PreviewZoom = std::max(0.20f, m_PreviewZoom - 0.02f);
            return;
        }
    }

    const bool resize = SEASON3B::IsRepeat(VK_SHIFT)
        && (m_DebugElement == DEBUG_PREVIEW
            || m_DebugElement == DEBUG_PREVIOUS
            || m_DebugElement == DEBUG_NEXT
            || m_DebugElement == DEBUG_STATS);

    int dx = 0;
    int dy = 0;
    if (SEASON3B::IsPress(VK_LEFT)) dx = -1;
    else if (SEASON3B::IsPress(VK_RIGHT)) dx = 1;
    else if (SEASON3B::IsPress(VK_UP)) dy = -1;
    else if (SEASON3B::IsPress(VK_DOWN)) dy = 1;

    if (resize)
    {
        int minWidth = 20;
        int minHeight = 8;
        if (m_DebugElement == DEBUG_PREVIEW)
        {
            minWidth = 80;
            minHeight = 80;
        }
        else if (m_DebugElement == DEBUG_STATS)
        {
            minWidth = 80;
            minHeight = 24;
        }

        rectangle->Width = std::max(minWidth, rectangle->Width + dx);
        rectangle->Height = std::max(minHeight, rectangle->Height + dy);
    }
    else
    {
        rectangle->X += dx;
        rectangle->Y += dy;
    }
}

void CNewUICustomRanking::ResetDebugElement()
{
    switch (m_DebugElement)
    {
    case DEBUG_PREVIEW:
        m_PreviewRect = { 294, 144, 162, 280 };
        m_PreviewZoom = 0.50f;
        break;
    case DEBUG_PANEL:
        m_PanelScale = 0.82f;
        m_PanelOffsetX = 0;
        m_PanelOffsetY = -4;
        PositionWindow();
        break;
    case DEBUG_CHARACTERS:
        m_CharacterColumnOffsetX = 11;
        m_CharacterColumnOffsetY = 12;
        break;
    case DEBUG_CHARACTER_NAMES:
        m_CharacterNameOffsetX = 0;
        m_CharacterNameOffsetY = -2;
        break;
    case DEBUG_RESETS:
        m_ResetColumnOffsetX = 0;
        m_ResetColumnOffsetY = 14;
        break;
    case DEBUG_RESET_VALUES:
        m_ResetValueOffsetX = -7;
        m_ResetValueOffsetY = -2;
        break;
    case DEBUG_ROW_SPACING:
        m_RowSpacingAdjustment = -1;
        break;
    case DEBUG_PREVIOUS:
        m_PreviousRect = { 43, 474, 70, 20 };
        break;
    case DEBUG_NEXT:
        m_NextRect = { 169, 474, 73, 20 };
        break;
    case DEBUG_TITLE:
        m_TitleRect = { 280, 12, 190, 22 };
        break;
    case DEBUG_STATS:
        m_StatsRect = { 290, 438, 440, 30 };
        break;
    default:
        break;
    }
}

bool CNewUICustomRanking::HandleEquipmentPrivacyClick()
{
    if(!CheckRect(m_EquipmentPrivacyRect))return false;
    auto& equipment=UI::Character::RankingEquipment();
    equipment.RefreshOwnPreference();
    if(equipment.PreferenceReady())equipment.SetOwnDetailsPublic(!equipment.OwnDetailsPublic());
    UI::Paris::ConsumeMouse();
    return true;
}

bool CNewUICustomRanking::UpdateMouseEvent()
{
    if (!m_WindowOpen)
    {
        return true;
    }

    PositionWindow();
    UpdateSearch();
    m_SearchBox->DoAction();
    if (SEASON3B::IsPress(VK_LBUTTON))
    {
        if(HandleEquipmentPrivacyClick())return false;
        if(m_Tab==TAB_GUILD && UI::Character::GuildRanking().HandleClick())
        {UI::Paris::ConsumeMouse();return false;}
        if (CheckRect(m_CloseRect))
        {
            UI::Paris::ConsumeMouse();
            CloseWindow();
            return false;
        }

        if (CheckRect(m_ResetTabRect))
        {
            SwitchTab(TAB_RESET);
            return false;
        }

        if (CheckRect(m_GuildTabRect))
        {
            SwitchTab(TAB_GUILD);
            return false;
        }

        if (CheckRect(m_QuestTabRect))
        {
            SwitchTab(TAB_QUEST);
            return false;
        }

        if (m_Tab == TAB_QUEST && UI::Character::MissionRanking().Mouse()) return false;
        const int row = GetClickedRow();
        if (row >= 0)
        {
            if (m_Tab == TAB_GUILD && row == m_SelectedIndex) UI::Character::GuildRanking().Retry();
            RequestCharacter(row);
            return false;
        }

        if (CheckRect(m_PreviousRect) && m_Page > 0)
        {
            --m_Page;
            return false;
        }

        if (CheckRect(m_NextRect) && m_Page < GetMaximumPage())
        {
            ++m_Page;
            return false;
        }
    }

    return !CheckWindow();
}

bool CNewUICustomRanking::Render()
{
    if (!m_WindowOpen)
    {
        return true;
    }

    PositionWindow();
    EnableAlphaTest();
    UI::Paris::Color(1.0f, 1.0f, 1.0f, 1.0f);
    UI::Character::RankingHover::BeginFrame();
    RenderWindow();
    UI::Character::RankingHover::Render();
    DisableAlphaBlend();
    return true;
}

void CNewUICustomRanking::RenderWindow()
{
    if (m_Tab == TAB_QUEST)
    {
        auto& missions = UI::Character::MissionRanking();
        missions.SetLayout(m_Position.x, m_Position.y, RankingPhysicalScale(m_PanelScale));
        if(g_RankingReferenceSkin!=BITMAP_UNKNOWN)RenderBitmap(g_RankingReferenceSkin,m_Position.x,m_Position.y,GetWindowWidth(),GetWindowHeight());
        missions.RenderBackground();
        RenderButton(m_ResetTabRect, L"RESET", true); RenderButton(m_GuildTabRect, L"CLANES", true); RenderButton(m_QuestTabRect, L"MISIONES", true);
        RenderCloseButton(); missions.Render(m_SearchText);
        if (m_SearchText.empty() && !m_SearchBox->HaveFocus()) RankingText({57, 132, 212, 14}, L"Buscar mision...", g_RankingFont, false);
        m_SearchBox->Render(); return;
    }
    UI::Character::GuildRanking().SetLayout(m_Position.x,m_Position.y,RankingPhysicalScale(m_PanelScale));
    if(m_Tab==TAB_GUILD)UI::Character::GuildRanking().RenderBackground();
    else if (g_RankingReferenceSkin != BITMAP_UNKNOWN)
    {
        EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(g_RankingReferenceSkin, m_Position.x, m_Position.y, GetWindowWidth(), GetWindowHeight());
    }
    else UI::Character::ParisFlat::Frame(m_Position.x, m_Position.y, GetWindowWidth(), GetWindowHeight());
    RenderLayoutPanels();
    if (g_RankingReferenceSkin == BITMAP_UNKNOWN) RankingText(m_TitleRect, L"RANKING", g_RankingTitle, true, RT3_SORT_CENTER);
    RenderButton(m_ResetTabRect, L"RESET", true);
    RenderButton(m_GuildTabRect, L"CLANES", true);
    RenderButton(m_QuestTabRect, L"MISIONES", true);
    const UiRect search = GetAbsoluteRect({26, 106, 192, 24});
    {
        if (g_RankingReferenceSkin == BITMAP_UNKNOWN) UI::Character::ParisFlat::Box(search.X, search.Y, search.Width, search.Height);
        g_ParisRankingExact.SearchIcon(search.X + ScaleValue(8), search.Y + ScaleValue(6), ScaleValue(12));
    }
    if (m_SearchText.empty() && !m_SearchBox->HaveFocus())
        RankingText({53, 112, 162, 14}, m_Tab == TAB_GUILD ? L"Buscar clan..." : L"Buscar personaje...", g_RankingFont, false);
    m_SearchBox->Render();
    RenderCloseButton();
    RenderList();
    if(m_Tab==TAB_GUILD)
    {
        wchar_t count[32]{};swprintf_s(count,L"%zu GUILDS",m_GuildEntries.size());
        RankingText({173,93,46,12},count,g_RankingFont,false,RT3_SORT_CENTER);
    }
    if(m_Tab==TAB_GUILD)
        UI::Character::GuildRanking().Render(m_GuildName,m_ProfileName,m_GuildMark,m_SelectedIndex+1,m_GuildScore,m_GuildMemberCount);
    else RenderProfile();
    RenderEquipment();

}

void CNewUICustomRanking::RenderLayoutPanels()
{
    if (g_RankingReferenceSkin == BITMAP_UNKNOWN)
    {
        for (const UiRect& panel : {UiRect{16, 84, 212, 453}, UiRect{239, 84, 495, 42}, UiRect{239, 134, 167, 329}, UiRect{415, 134, 319, 329}, UiRect{239, 472, 495, 28}, UiRect{239, 507, 495, 25}})
        {
            const auto rect = GetAbsoluteRect(panel);
            UI::Character::ParisFlat::Box(rect.X, rect.Y, rect.Width, rect.Height);
        }
        const UiRect line = GetAbsoluteRect({16, 74, 718, 1});
        UI::Character::ParisStyle::Solid(line.X, line.Y, line.Width, 1, .67f, .49f, .15f);
        RankingText({23, 21, 180, 16}, L"PARIS PROJECT", g_RankingBold, true);
        RankingText({28, 91, 160, 16}, L"TOP JUGADORES", g_RankingBold, true);
        RankingText({178, 93, 40, 12}, L"ONLINE", g_RankingFont, false, RT3_SORT_CENTER);
        RankingText({248, 139, 110, 16}, L"PERSONAJE", g_RankingBold, true);
        RankingText({424, 139, 175, 16}, L"EQUIPAMIENTO", g_RankingBold, true);
        RankingText({636, 141, 90, 12}, L"12 ESPACIOS", g_RankingFont, true, RT3_SORT_CENTER);
        RankingText({249, 513, 475, 15}, L"El jugador decide los detalles que comparte de su equipo.", g_RankingFont, false);
        RankingText({239, 540, 495, 12}, L"PARIS PROJECT - RANKING GLOBAL", g_RankingFont, true, RT3_SORT_CENTER);
    }
    const auto ownBadge = GetAbsoluteRect(m_EquipmentPrivacyRect);
    DrawRankingStatus(ownBadge.X, ownBadge.Y, ownBadge.Width, ownBadge.Height, UI::Character::RankingEquipment().OwnDetailsPublic() ? L"MI EQUIPO: PUBLICO" : L"MI EQUIPO: PRIVADO", g_RankingFont, UI::Character::RankingEquipment().OwnDetailsPublic());
}

void CNewUICustomRanking::RenderList()
{
    const UiRect header = GetAbsoluteRect({ListX, ListY, ListWidth, 20});
    if (g_RankingReferenceSkin == BITMAP_UNKNOWN) UI::Character::ParisFlat::Box(header.X, header.Y, header.Width, header.Height);
    RankingText({ListX + 4, ListY, 24, 20}, L"#", g_RankingValueFont, true, RT3_SORT_CENTER, true);
    RankingText({ListX + 36, ListY, 109, 20}, m_Tab == TAB_GUILD ? L"CLAN" : L"PERSONAJE",
        g_RankingValueFont, true, RT3_SORT_LEFT, true);
    const wchar_t* value = m_Tab == TAB_RESET ? L"RESETS" : m_Tab == TAB_GUILD ? L"PUNTOS" : L"MISIONES";
    RankingText({ListX + 146, ListY, 48, 20}, value, g_RankingValueFont, true, RT3_SORT_CENTER, true);
    const int first = m_Page * PAGE_SIZE;
    for (int row = 0; row < PAGE_SIZE && first + row < static_cast<int>(m_FilteredEntries.size()); ++row)
        RenderRankingRow(row, m_FilteredEntries[first + row]);
    RenderRankingPager();
    if (m_FilteredEntries.empty())
        RankingText({ListX, ListY + 140, ListWidth, 16}, m_ListPending ? L"Cargando ranking..." : L"Sin resultados", g_RankingFont, false, RT3_SORT_CENTER);
}


void CNewUICustomRanking::RenderGuildList()
{
    RenderList();
}

void CNewUICustomRanking::RenderQuestList()
{
    RenderList();
}

void CNewUICustomRanking::RankingText(const UiRect& rect, const wchar_t* text, HFONT font, bool gold,
    int align, bool centerVertically) const
{
    const UiRect absolute = GetAbsoluteRect(rect);
    g_pRenderText->SetFont(font ? font : g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    // Match the gold used by the reference skin's RANKING title (#eed38b).
    g_pRenderText->SetTextColor(gold ? 238 : 255, gold ? 211 : 255, gold ? 139 : 255, 255);
    int top = absolute.Y;
    if (centerVertically)
    {
        SIZE textSize{};
        textSize = g_pRenderText->MeasureText(text, lstrlen(text));
        const float textHeight = static_cast<float>(textSize.cy) / g_fScreenRate_y;
        top += static_cast<int>(std::lround((absolute.Height - textHeight) * .5f));
    }
    g_pRenderText->RenderText(absolute.X, top, text, absolute.Width, 0, align);
}

void CNewUICustomRanking::RenderRankingRow(int row, int index)
{
    const int top = ListY + 20 + row * RowHeight;
    const UiRect rect = GetAbsoluteRect({ListX, top, ListWidth, RowHeight});
    const bool selected = index == m_SelectedIndex;
    const bool hovered = CheckMouseIn(rect.X, rect.Y, rect.Width, rect.Height);
    if (selected || hovered)
    {
        if (g_RankingWidgets != BITMAP_UNKNOWN)
        {
            EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
            RenderBitmap(g_RankingWidgets, rect.X, rect.Y, rect.Width, rect.Height, 0.f, 40.f / 128.f, 200.f / 256.f, 44.f / 128.f);
        }
        else UI::Character::ParisFlat::Box(rect.X, rect.Y, rect.Width, rect.Height, selected);
    }
    else UI::Character::ParisStyle::Solid(rect.X + 4, rect.Y + rect.Height - 1, rect.Width - 8, 1, .42f, .31f, .11f);
    wchar_t position[16]{}; swprintf_s(position, L"%d", index + 1);
    if (index < 3)
    {
        const auto medal = GetAbsoluteRect({ListX + 6, top + 7, 17, 17});
        RenderPodiumMedal(medal.X, medal.Y, medal.Width, medal.Height, index + 1);
    }
    else RankingText({ListX + 4, top + 10, 24, 14}, position, g_RankingFont, false, RT3_SORT_CENTER);
    const char* name = m_Tab == TAB_RESET ? m_Entries[index].Name.data()
        : m_Tab == TAB_GUILD ? m_GuildEntries[index].GuildName.data() : m_QuestEntries[index].Name.data();
    const auto label = Utf8ToWide(name, CharacterNameLength);
    if (hovered && m_Tab != TAB_GUILD) UI::Character::RankingHover::Hover(label.c_str());
    RankingText({ListX + 36, top + 10, 109, 14}, label.c_str(), g_RankingFont, selected || index == 0);
    const int value = m_Tab == TAB_RESET ? m_Entries[index].Resets
        : m_Tab == TAB_GUILD ? m_GuildEntries[index].Score : m_QuestEntries[index].CompletedQuests;
    wchar_t number[32]{}; swprintf_s(number, L"%d", value);
    RankingText({ListX + 146, top + 10, 48, 14}, number, g_RankingFont, selected || index == 0, RT3_SORT_CENTER);
}

void CNewUICustomRanking::RenderRankingPager()
{
    RenderButton(m_PreviousRect, L"‹ ANTERIOR", m_Page > 0);
    RenderButton(m_NextRect, L"SIGUIENTE ›", m_Page < GetMaximumPage());
    wchar_t page[32]{}; swprintf_s(page, m_Tab == TAB_GUILD ? L"%d/%d" : L"%d / %d", m_Page + 1, GetMaximumPage() + 1);
    const int gapStart = m_PreviousRect.X + m_PreviousRect.Width;
    RankingText({gapStart, m_PreviousRect.Y, m_NextRect.X - gapStart, m_PreviousRect.Height},
        page, g_RankingFont, false, RT3_SORT_CENTER, true);
}

void CNewUICustomRanking::RenderProfile()
{
    const auto scene = GetAbsoluteRect({241, 156, 163, 305});
    if (g_RankingReferenceSkin == BITMAP_UNKNOWN && UI::Character::RankingStyle::Background != BITMAP_UNKNOWN)
    {
        EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(UI::Character::RankingStyle::Background, scene.X, scene.Y, scene.Width, scene.Height);
    }
    if (!m_ProfilePending && m_HasPreview)
    {
        const auto preview = GetAbsoluteRect(m_PreviewRect);
        RenderPreview(preview.X, preview.Y, preview.Width, preview.Height);
    }
    else
    {
        RankingText({245, 280, 155, 20}, m_ProfilePending ? L"Cargando personaje..." : L"Selecciona un personaje", g_RankingFont, false, RT3_SORT_CENTER);
    }
    DisableDepthTest(); DisableAlphaBlend(); EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    RenderProfileIdentity();
    RenderProfileStats();
}

void CNewUICustomRanking::RenderProfileIdentity()
{
    if (m_ProfileName.empty()) return;
    wchar_t name[64]{}; swprintf_s(name, L"%ls", m_ProfileName.c_str());
    CharUpperBuffW(name, static_cast<DWORD>(wcslen(name)));
    RankingText(m_ProfileNameRect, name, g_RankingNameFont, true);
    const wchar_t* classNames[] = {L"Dark Wizard", L"Dark Knight", L"Fairy Elf", L"Magic Gladiator", L"Dark Lord", L"Summoner", L"Rage Fighter"};
    const wchar_t* shortNames[] = {L"DW", L"DK", L"ELF", L"MG", L"DL", L"SUM", L"RF"};
    const int baseClass = gCharacterManager.GetBaseClass(m_Preview.Class);
    const auto className = baseClass >= 0 && baseClass < 7 ? classNames[baseClass] : L"Personaje";
    const auto classShort = baseClass >= 0 && baseClass < 7 ? shortNames[baseClass] : L"--";
    const auto badge = GetAbsoluteRect({250, 94, 32, 24});
    DrawRankingClassBadge(badge.X, badge.Y, badge.Width, badge.Height);
    RankingText({250, 94, 32, 24}, classShort, g_RankingBold, true, RT3_SORT_CENTER, true);
    wchar_t subtitle[128]{}; swprintf_s(subtitle, L"%ls - %ls", className, m_GuildName.empty() ? L"Sin clan" : m_GuildName.c_str());
    RankingText({292, 112, 310, 12}, subtitle, g_RankingFont, false);
    const auto visibleBadge = GetAbsoluteRect({627, 94, 94, 22});
    DrawRankingStatus(visibleBadge.X, visibleBadge.Y, visibleBadge.Width, visibleBadge.Height, L"EQUIPO VISIBLE", g_RankingFont, true);
    wchar_t portraitCaption[80]{}; swprintf_s(portraitCaption, L"%ls - LV %d", className, m_Level);
    const auto caption = GetAbsoluteRect({253, 430, 141, 22});
    if (g_RankingReferenceSkin == BITMAP_UNKNOWN) UI::Character::ParisFlat::Box(caption.X, caption.Y, caption.Width, caption.Height);
    RankingText({253, 430, 141, 22}, portraitCaption, g_RankingBold, true, RT3_SORT_CENTER, true);
}

void CNewUICustomRanking::RenderProfileStats()
{
    const wchar_t* labels[] = {L"NIVEL", L"RESET", L"TOTAL STATS", L"CLASE"};
    const wchar_t* classNames[] = {L"Dark Wizard", L"Dark Knight", L"Fairy Elf", L"Magic Gladiator", L"Dark Lord", L"Summoner", L"Rage Fighter"};
    const int baseClass = gCharacterManager.GetBaseClass(m_Preview.Class);
    wchar_t values[4][64]{};
    swprintf_s(values[0], L"%d", m_Level); swprintf_s(values[1], L"%d", m_Resets); swprintf_s(values[2], L"%d", m_TotalStats);
    if (m_Tab == TAB_GUILD)
    {
        labels[1] = L"PUNTOS"; labels[2] = L"MIEMBROS";
        swprintf_s(values[1], L"%d", m_GuildScore); swprintf_s(values[2], L"%d", m_GuildMemberCount);
    }
    else if (m_Tab == TAB_QUEST)
    {
        labels[1] = L"MISIONES"; labels[2] = L"TOTAL";
        swprintf_s(values[1], L"%d", m_QuestCompleted); swprintf_s(values[2], L"%d", m_QuestTotal);
    }
    swprintf_s(values[3], L"%ls", m_HasPreview && baseClass >= 0 && baseClass < 7 ? classNames[baseClass] : L"--");
    const UiRect footer = GetAbsoluteRect({239, 472, 495, 62});
    if (g_RankingFooter != BITMAP_UNKNOWN)
    {
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(g_RankingFooter, footer.X, footer.Y, footer.Width, footer.Height);
    }
    for (int column = 0; column < 4; ++column)
    {
        const int left = 270 + column * 124;
        RankingText({left, 475, 91, 14}, labels[column], g_RankingValueFont, true, RT3_SORT_LEFT, true);
        RankingText({left, 489, 91, 16}, values[column], g_RankingValueFont, false, RT3_SORT_LEFT, true);
    }
    RankingText({263, 516, 465, 15}, L"El jugador decide qué detalles de su equipo comparte en el ranking.",
        g_RankingFont, false, RT3_SORT_LEFT, true);

}

void CNewUICustomRanking::RenderPreview(int x, int y, int width, int height, float verticalOffset)
{
    if (!m_HasPreview
        || !m_Preview.Object.Live
        || m_Preview.Object.BoneTransform == nullptr
        || width <= 0
        || height <= 0)
    {
        return;
    }

    CHARACTER* character = &m_Preview;
    OBJECT* object = &m_Preview.Object;

    const int worldBackup = gMapManager.WorldActive;
    gMapManager.WorldActive = WD_0LORENCIA;
    MoveCharacter(character, object);
    MoveCharacterVisual(character, object);
    MoveMount(&m_PreviewHelper, TRUE);
    gMapManager.WorldActive = worldBackup;

    Render::UI::BeginCharacterPreview(x, y, width, height, verticalOffset);
    if (character->Helper.Type == MODEL_DARK_HORSE_ITEM)
    {
        mu::GetRenderer().Translate(-object->Position[0], -object->Position[1], -object->Position[2] - 50.0f);
    }
    else
    {
        mu::GetRenderer().Translate(-object->Position[0], -object->Position[1], -object->Position[2]);
    }

    Vector(0.0f, 0.0f, m_PreviewAngle, object->Angle);

    mu::GetRenderer().SetAlphaTest(false);
    mu::GetRenderer().SetTexture2D(true);
    EnableDepthTest();
    EnableCullFace();
    EnableDepthMask();
    TextureEnable = true;
    DepthTestEnable = true;
    CullFaceEnable = true;
    DepthMaskEnable = true;
    mu::GetRenderer().SetDepthFunc(GL_LEQUAL);
    mu::GetRenderer().SetAlphaFunc(GL_GREATER, 0.25f);
    mu::GetRenderer().SetFogEnabled(false);
    mu::GetRenderer().ClearDepthBuffer();

    object->Scale = 0.7f * m_PreviewZoom;
    m_PreviewHelper.Scale = m_PreviewHelperScale * m_PreviewZoom;
    Vector(1.0f, 1.0f, 1.0f, object->Light);
    Vector(1.0f, 1.0f, 1.0f, m_PreviewHelper.Light);

    character->HideShadow = true;
    character->SafeZone = false;
    if (character->Helper.Type == MODEL_HORN_OF_UNIRIA)
    {
        m_PreviewHelper.Position[2] += 10.0f;
    }
    else if (character->Helper.Type == MODEL_HORN_OF_DINORANT)
    {
        m_PreviewHelper.Position[2] += 25.0f;
    }

    RenderMount(&m_PreviewHelper, TRUE);

    if (character->Helper.Type == MODEL_HORN_OF_UNIRIA)
    {
        m_PreviewHelper.Position[2] -= 10.0f;
    }
    else if (character->Helper.Type == MODEL_HORN_OF_DINORANT)
    {
        m_PreviewHelper.Position[2] -= 25.0f;
    }

    RenderCharacter(character, object, 0);

    mu::GetRenderer().SetMatrixMode(GL_MODELVIEW);
    mu::GetRenderer().PopMatrix();
    mu::GetRenderer().SetMatrixMode(GL_PROJECTION);
    mu::GetRenderer().PopMatrix();
    SetRenderViewport(0, 0, WindowWidth, WindowHeight);
    RestoreCameraPerspective();
    // Return the fixed-function matrix selector to the mode expected by the
    // surrounding NewUI pass. The old code returned while GL_PROJECTION was
    // still selected.
    mu::GetRenderer().SetMatrixMode(GL_MODELVIEW);
}

void CNewUICustomRanking::RenderButton(const UiRect& rect, const wchar_t* text, bool enabled) const
{
    const UiRect absolute = GetAbsoluteRect(rect);
    const bool selected = (&rect == &m_ResetTabRect && m_Tab == TAB_RESET)
        || (&rect == &m_GuildTabRect && m_Tab == TAB_GUILD) || (&rect == &m_QuestTabRect && m_Tab == TAB_QUEST);
    const int colorTile = &rect == &m_ResetTabRect || &rect == &m_PreviousRect ? 0
        : &rect == &m_GuildTabRect ? 1
        : &rect == &m_QuestTabRect || &rect == &m_NextRect ? 2 : -1;
    if (colorTile >= 0 && g_RankingColoredWidgets != BITMAP_UNKNOWN)
    {
        EnableAlphaTest();
        const float brightness = enabled ? (CheckRect(rect) ? 1.15f : 1.f) : .65f;
        UI::Paris::Color(brightness, brightness, brightness, 1);
        RenderBitmap(g_RankingColoredWidgets, absolute.X, absolute.Y, absolute.Width, absolute.Height,
            0.f, colorTile * 64.f / 256.f, 200.f / 256.f, 56.f / 256.f);
        if (selected)
            UI::Character::ParisStyle::Solid(absolute.X + absolute.Width / 8, absolute.Y + absolute.Height - 3,
                absolute.Width * 3 / 4, 1, .92f, .72f, .25f);
        UI::Paris::Color(1, 1, 1, 1);
    }
    else if (g_RankingWidgets != BITMAP_UNKNOWN)
    {
        EnableAlphaTest(); UI::Paris::Color(enabled ? 1.f : .5f, enabled ? 1.f : .5f, enabled ? 1.f : .5f, 1);
        RenderBitmap(g_RankingWidgets, absolute.X, absolute.Y, absolute.Width, absolute.Height,
            selected ? 0.f : 100.f / 256.f, 0.f, 96.f / 256.f, 32.f / 128.f);
        UI::Paris::Color(1, 1, 1, 1);
    }
    else UI::Character::ParisFlat::Box(absolute.X, absolute.Y, absolute.Width, absolute.Height, selected);

    RankingText(rect, text, g_RankingBold, selected || enabled, RT3_SORT_CENTER, true);
}

void CNewUICustomRanking::RenderPaginationButton(const UiRect& rect, bool next, bool enabled) const
{
    const bool hovered = enabled && CheckMouseIn(rect.X, rect.Y, rect.Width, rect.Height);
    const float red = next ? .025f : .36f;
    const float green = next ? .25f : .025f;
    const float brightness = enabled ? (hovered ? 1.35f : 1.f) : .6f;
    UI::Theme::Bevel(rect.X, rect.Y, rect.Width, rect.Height, L"", false, true);
    UI::Character::ParisStyle::Solid(rect.X + 2, rect.Y + 2, rect.Width - 4, rect.Height - 4,
        red * brightness, green * brightness, .025f);
}

void CNewUICustomRanking::RenderCloseButton() const
{
    RenderButton(m_CloseRect, L"X", true);
}


void CNewUICustomRanking::RenderPanel(int x, int y, int width, int height, BYTE alpha) const
{
    SetRenderColor(10, 12, 17, alpha);
    RenderColor(x, y, width, height);
    EndRenderColor();

    SetRenderColor(142, 99, 35, 180);
    RenderColor(x, y, width, 1);
    RenderColor(x, y + height - 1, width, 1);
    RenderColor(x, y, 1, height);
    RenderColor(x + width - 1, y, 1, height);
    EndRenderColor();
}

void CNewUICustomRanking::RenderDebug() const
{
    UiRect highlight{};
    const wchar_t* elementName = L"MODELO 3D";

    switch (m_DebugElement)
    {
    case DEBUG_PREVIEW:
        highlight = GetAbsoluteRect(m_PreviewRect);
        break;
    case DEBUG_PANEL:
        highlight = { m_Position.x, m_Position.y, GetWindowWidth(), GetWindowHeight() };
        elementName = L"PANEL COMPLETO";
        break;
    case DEBUG_CHARACTERS:
        highlight = GetAbsoluteRect({
            ListX + 5 + m_CharacterColumnOffsetX,
            ListY + 8 + m_CharacterColumnOffsetY,
            145,
            282,
        });
        elementName = L"PERSONAJES COLUMNA";
        break;
    case DEBUG_CHARACTER_NAMES:
        highlight = GetAbsoluteRect({
            ListX + 35 + m_CharacterColumnOffsetX + m_CharacterNameOffsetX,
            ListY + 31 + 6 + m_CharacterColumnOffsetY + m_CharacterNameOffsetY,
            115,
            (PAGE_SIZE - 1) * (RowHeight + m_RowSpacingAdjustment) + RowHeight,
        });
        elementName = L"NOMBRES PERSONAJES";
        break;
    case DEBUG_RESETS:
        highlight = GetAbsoluteRect({
            ListX + 152 + m_ResetColumnOffsetX,
            ListY + 8 + m_ResetColumnOffsetY,
            67,
            282,
        });
        elementName = L"RESETS COLUMNA";
        break;
    case DEBUG_RESET_VALUES:
        highlight = GetAbsoluteRect({
            ListX + 152 + m_ResetColumnOffsetX + m_ResetValueOffsetX,
            ListY + 31 + 6 + m_ResetColumnOffsetY + m_ResetValueOffsetY,
            67,
            (PAGE_SIZE - 1) * (RowHeight + m_RowSpacingAdjustment) + RowHeight,
        });
        elementName = L"RESET NUMEROS";
        break;
    case DEBUG_ROW_SPACING:
        highlight = GetAbsoluteRect({
            ListX + 4,
            ListY + 31,
            ListWidth - 8,
            PAGE_SIZE * (RowHeight + m_RowSpacingAdjustment),
        });
        elementName = L"ESPACIADO FILAS";
        break;
    case DEBUG_PREVIOUS:
        highlight = GetAbsoluteRect(m_PreviousRect);
        elementName = L"ANTERIOR";
        break;
    case DEBUG_NEXT:
        highlight = GetAbsoluteRect(m_NextRect);
        elementName = L"SIGUIENTE";
        break;
    case DEBUG_TITLE:
        highlight = GetAbsoluteRect(m_TitleRect);
        elementName = L"TITULO TOP RESET";
        break;
    case DEBUG_STATS:
        highlight = GetAbsoluteRect(m_StatsRect);
        elementName = L"STATS BOX";
        break;
    default:
        break;
    }

    SetRenderColor(255, 45, 45, 230);
    RenderColor(highlight.X, highlight.Y, highlight.Width, 1);
    RenderColor(highlight.X, highlight.Y + highlight.Height - 1, highlight.Width, 1);
    RenderColor(highlight.X, highlight.Y, 1, highlight.Height);
    RenderColor(highlight.X + highlight.Width - 1, highlight.Y, 1, highlight.Height);
    EndRenderColor();

    wchar_t values[256] = {};
    switch (m_DebugElement)
    {
    case DEBUG_PREVIEW:
        mu_swprintf(
            values,
            L"DEBUG %ls X=%d Y=%d W=%d H=%d ZOOM=%d%%",
            elementName,
            m_PreviewRect.X,
            m_PreviewRect.Y,
            m_PreviewRect.Width,
            m_PreviewRect.Height,
            static_cast<int>(m_PreviewZoom * 100.0f + 0.5f));
        break;
    case DEBUG_PANEL:
        mu_swprintf(
            values,
            L"DEBUG %ls SCALE=%d%% OFFSET_X=%d OFFSET_Y=%d W=%d H=%d",
            elementName,
            static_cast<int>(m_PanelScale * 100.0f + 0.5f),
            m_PanelOffsetX,
            m_PanelOffsetY,
            GetWindowWidth(),
            GetWindowHeight());
        break;
    case DEBUG_CHARACTERS:
        mu_swprintf(
            values,
            L"DEBUG %ls X=%d Y=%d OFFSET_X=%d OFFSET_Y=%d",
            elementName,
            ListX + 5 + m_CharacterColumnOffsetX,
            ListY + 8 + m_CharacterColumnOffsetY,
            m_CharacterColumnOffsetX,
            m_CharacterColumnOffsetY);
        break;
    case DEBUG_CHARACTER_NAMES:
        mu_swprintf(
            values,
            L"DEBUG %ls X=%d Y=%d OFFSET_X=%d OFFSET_Y=%d",
            elementName,
            ListX + 35 + m_CharacterColumnOffsetX + m_CharacterNameOffsetX,
            ListY + 37 + m_CharacterColumnOffsetY + m_CharacterNameOffsetY,
            m_CharacterNameOffsetX,
            m_CharacterNameOffsetY);
        break;
    case DEBUG_RESETS:
        mu_swprintf(
            values,
            L"DEBUG %ls X=%d Y=%d OFFSET_X=%d OFFSET_Y=%d",
            elementName,
            ListX + 152 + m_ResetColumnOffsetX,
            ListY + 8 + m_ResetColumnOffsetY,
            m_ResetColumnOffsetX,
            m_ResetColumnOffsetY);
        break;
    case DEBUG_RESET_VALUES:
        mu_swprintf(
            values,
            L"DEBUG %ls X=%d Y=%d OFFSET_X=%d OFFSET_Y=%d",
            elementName,
            ListX + 152 + m_ResetColumnOffsetX + m_ResetValueOffsetX,
            ListY + 37 + m_ResetColumnOffsetY + m_ResetValueOffsetY,
            m_ResetValueOffsetX,
            m_ResetValueOffsetY);
        break;
    case DEBUG_ROW_SPACING:
        mu_swprintf(
            values,
            L"DEBUG %ls BASE=%d AJUSTE=%d RESULTADO=%d",
            elementName,
            RowHeight,
            m_RowSpacingAdjustment,
            RowHeight + m_RowSpacingAdjustment);
        break;
    case DEBUG_PREVIOUS:
        mu_swprintf(values, L"DEBUG %ls X=%d Y=%d W=%d H=%d", elementName,
            m_PreviousRect.X, m_PreviousRect.Y, m_PreviousRect.Width, m_PreviousRect.Height);
        break;
    case DEBUG_NEXT:
        mu_swprintf(values, L"DEBUG %ls X=%d Y=%d W=%d H=%d", elementName,
            m_NextRect.X, m_NextRect.Y, m_NextRect.Width, m_NextRect.Height);
        break;
    case DEBUG_TITLE:
        mu_swprintf(values, L"DEBUG %ls X=%d Y=%d", elementName, m_TitleRect.X, m_TitleRect.Y);
        break;
    case DEBUG_STATS:
        mu_swprintf(values, L"DEBUG %ls X=%d Y=%d W=%d H=%d", elementName,
            m_StatsRect.X, m_StatsRect.Y, m_StatsRect.Width, m_StatsRect.Height);
        break;
    default:
        break;
    }

    g_pRenderText->SetFont(g_RankingBold ? g_RankingBold : g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 215);
    g_pRenderText->SetTextColor(255, 105, 105, 255);
    g_pRenderText->RenderText(5, 5, values, 630, 0, RT3_SORT_LEFT);

    g_pRenderText->SetFont(g_RankingFont ? g_RankingFont : g_hFont);
    g_pRenderText->SetTextColor(235, 210, 170, 255);
    const wchar_t* help = L"TAB: siguiente | Flechas: mover | HOME: restaurar | Ctrl+F12: salir";
    if (m_DebugElement == DEBUG_PREVIEW)
    {
        help = L"TAB | Flechas: mover | SHIFT+Flechas: W/H | PgUp/PgDn: zoom | HOME | Ctrl+F12";
    }
    else if (m_DebugElement == DEBUG_PANEL)
    {
        help = L"TAB | Flechas: mover panel X/Y | PgUp/PgDn: SCALE +/-1% | HOME | Ctrl+F12";
    }
    else if (m_DebugElement == DEBUG_PREVIOUS || m_DebugElement == DEBUG_NEXT || m_DebugElement == DEBUG_STATS)
    {
        help = L"TAB | Flechas: mover | SHIFT+Flechas: W/H | HOME | Ctrl+F12";
    }
    else if (m_DebugElement == DEBUG_CHARACTERS || m_DebugElement == DEBUG_RESETS)
    {
        help = L"TAB | Flechas: mover columna completa X/Y | HOME | Ctrl+F12";
    }
    else if (m_DebugElement == DEBUG_CHARACTER_NAMES || m_DebugElement == DEBUG_RESET_VALUES)
    {
        help = L"TAB | Flechas: mover SOLO valores de filas X/Y | HOME | Ctrl+F12";
    }
    else if (m_DebugElement == DEBUG_ROW_SPACING)
    {
        help = L"TAB | Flechas: ajustar separacion vertical entre filas | HOME | Ctrl+F12";
    }
    g_pRenderText->RenderText(5, 18, help, 630, 0, RT3_SORT_LEFT);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
}

bool CNewUICustomRanking::CheckRect(const UiRect& rect) const
{
    const UiRect absolute = GetAbsoluteRect(rect);
    return CheckMouseIn(absolute.X, absolute.Y, absolute.Width, absolute.Height);
}

bool CNewUICustomRanking::CheckWindow() const
{
    return CheckMouseIn(m_Position.x, m_Position.y, GetWindowWidth(), GetWindowHeight());
}

int CNewUICustomRanking::GetClickedRow() const
{
    const int first = m_Page * PAGE_SIZE;
    for (int row = 0; row < PAGE_SIZE && first + row < static_cast<int>(m_FilteredEntries.size()); ++row)
    {
        if (CheckRect({ListX, ListY + 20 + row * RowHeight, ListWidth, RowHeight}))
            return m_FilteredEntries[first + row];
    }
    return -1;
}

int CNewUICustomRanking::GetMaximumPage() const
{
    const int count = static_cast<int>(m_FilteredEntries.size());
    if (count <= 0)
    {
        return 0;
    }

    return (count - 1) / PAGE_SIZE;
}

std::int32_t CNewUICustomRanking::ReadInt32(const BYTE* data)
{
    const std::uint32_t value = static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
    return static_cast<std::int32_t>(value);
}

std::wstring CNewUICustomRanking::Utf8ToWide(const char* value, int maximumBytes)
{
    if (value == nullptr || maximumBytes <= 0)
    {
        return {};
    }

    int length = 0;
    while (length < maximumBytes && value[length] != '\0')
    {
        ++length;
    }

    if (length == 0)
    {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, value, length, nullptr, 0);
    if (required <= 0)
    {
        return {};
    }

    std::wstring result(required, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value, length, result.data(), required);
    return result;
}

void CNewUICustomRanking::RenderEquipment()
{
    const UiRect equipment = GetAbsoluteRect({424, 163, 301, 283});
    UI::Character::RankingEquipment().SetReferenceLayout(g_RankingReferenceSkin != BITMAP_UNKNOWN, RankingPhysicalScale(m_PanelScale));
    UI::Character::RankingEquipment().Render(equipment.X, equipment.Y, equipment.Width, equipment.Height, m_WindowOpen && m_Tab!=TAB_GUILD && !m_ProfileName.empty());
}
