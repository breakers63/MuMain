#pragma once

// MUETRO_GUILD_RANKING_TABS_V1
// DEKOHUT_QUEST_RANKING_V1

#include "Engine/Object/ZzzCharacter.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class CUITextInputBox;

namespace SEASON3B
{
    constexpr DWORD TOP_RESET_RANKING_INTERFACE_KEY = 0x54505253; // 'TPRS'

    class CNewUICustomRanking : public CNewUIObj
    {
    public:
        enum IMAGE_LIST
        {
            IMAGE_TOP_RESET_BACKGROUND_BOTTOM = 31920,
            IMAGE_TOP_RESET_BACKGROUND_TOP = 31921,
            IMAGE_TOP_RESET_BUTTON = 31922,
            IMAGE_TOP_RESET_CLOSE = 31923,
        };

        enum
        {
            WINDOW_WIDTH = 750,
            WINDOW_HEIGHT = 560,
            PAGE_SIZE = 8,
            MAXIMUM_ENTRIES = 50,
        };

        CNewUICustomRanking();
        virtual ~CNewUICustomRanking();
        static CNewUICustomRanking* NewWindow();

        bool Create(CNewUIManager* manager, CNewUI3DRenderMng* renderManager);
        void Release();

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;
        bool IsVisible() const override;
        float GetLayerDepth() override;
        float GetKeyEventOrder() override;

        void OpenWindow();
        void CloseWindow(bool playSound = true);
        bool IsWindowOpen() const;
        bool IsMissionTab() const { return m_Tab == TAB_QUEST; }
        bool CloseForMenuHotKey();
        void ReceivePacket(const BYTE* packet, int size);
        void RenderInspectionCharacter(const CHARACTER& source, int x, int y, int width, int height);

    private:
        enum RankingTab
        {
            TAB_RESET = 0,
            TAB_GUILD,
            TAB_QUEST,
        };

        struct RankingEntry
        {
            std::array<char, 11> Name{};
            std::int32_t Resets{};
        };

        struct GuildRankingEntry
        {
            std::array<char, 11> GuildName{};
            std::array<char, 11> LeaderName{};
            std::int32_t Score{};
            std::int32_t MemberCount{};
        };

        struct QuestRankingEntry
        {
            std::array<char, 11> Name{};
            std::int32_t CompletedQuests{};
            std::int32_t TotalQuests{};
        };

        struct UiRect
        {
            int X;
            int Y;
            int Width;
            int Height;
        };

        enum DebugElement
        {
            DEBUG_PREVIEW = 0,
            DEBUG_PANEL,
            DEBUG_CHARACTERS,
            DEBUG_CHARACTER_NAMES,
            DEBUG_RESETS,
            DEBUG_RESET_VALUES,
            DEBUG_ROW_SPACING,
            DEBUG_PREVIOUS,
            DEBUG_NEXT,
            DEBUG_TITLE,
            DEBUG_STATS,
            DEBUG_ELEMENT_COUNT,
        };

        void PositionWindow();
        int ScaleValue(int value) const;
        int GetWindowWidth() const;
        int GetWindowHeight() const;
        UiRect GetAbsoluteRect(const UiRect& rect) const;
        UiRect GetListLayoutRect(const UiRect& rect) const;
        void LoadImages();
        void UnloadImages();
        void RequestList();
        void RequestCharacter(int index);
        void SwitchTab(RankingTab tab);
        void RestoreTabList();
        void ClearProfileState();
        int GetEntryCount() const;
        void ReceiveList(const BYTE* packet, int size, int payloadOffset);
        void ReceiveCharacter(const BYTE* packet, int size, int payloadOffset);
        void ReceiveGuildList(const BYTE* packet, int size, int payloadOffset);
        void ReceiveGuildLeader(const BYTE* packet, int size, int payloadOffset);
        void ReceiveQuestList(const BYTE* packet, int size, int payloadOffset);
        void ReceiveQuestCharacter(const BYTE* packet, int size, int payloadOffset);

        void BuildPreview(const BYTE* appearance);
        void BuildInspectionCharacter(const CHARACTER& source);
        void ReleasePreview();
        void UpdateGuildMark();
        void RenderPreview(int x, int y, int width, int height, float verticalOffset = 0.f);

        bool HandleEscape();
        void RenderPaginationButton(const UiRect& rect, bool next, bool enabled) const;
        void UpdateSearch();
        void RankingText(const UiRect& rect, const wchar_t* text, HFONT font, bool gold,
            int align = RT3_SORT_LEFT, bool centerVertically = false) const;
        void RenderRankingRow(int row, int index);
        void RenderRankingPager();
        void RenderWindow();
        void RenderLayoutPanels();
        void RenderList();
        void RenderGuildList();
        void RenderQuestList();
        void RenderProfile();
        void RenderEquipment();
        void RenderProfileStats();
        void RenderProfileIdentity();
        void RenderButton(const UiRect& rect, const wchar_t* text, bool enabled) const;
        void RenderCloseButton() const;
        void RenderPanel(int x, int y, int width, int height, BYTE alpha) const;
        void UpdateDebugElement();
        void ResetDebugElement();
        void RenderDebug() const;

        bool CheckRect(const UiRect& rect) const;
        bool HandleEquipmentPrivacyClick();
        bool CheckWindow() const;
        int GetClickedRow() const;
        int GetMaximumPage() const;

        static std::int32_t ReadInt32(const BYTE* data);
        static std::wstring Utf8ToWide(const char* value, int maximumBytes);

    private:
        CNewUIManager* m_Manager;
        CUITextInputBox* m_SearchBox;
        std::wstring m_SearchText;
        int m_SearchEntryCount;
        bool m_EscapeHeld;
        std::vector<int> m_FilteredEntries;
        POINT m_Position;
        bool m_ImagesLoaded;
        bool m_WindowOpen;
        bool m_ListPending;
        bool m_ProfilePending;
        DWORD m_ListRequestTick;
        DWORD m_ProfileRequestTick;

        std::array<DWORD, 3> m_ListCacheTick{};
        std::array<bool, 3> m_ListCacheReady{};
        RankingTab m_Tab;
        std::vector<RankingEntry> m_Entries;
        std::vector<GuildRankingEntry> m_GuildEntries;
        std::vector<QuestRankingEntry> m_QuestEntries;
        int m_Page;
        int m_SelectedIndex;

        std::wstring m_ProfileName;
        std::wstring m_GuildName;
        std::array<char, 11> m_GuildNameUtf8;
        std::array<BYTE, 32> m_GuildMark;
        int m_LastProfilePacketSize;
        int m_LastProfilePayloadOffset;
        int m_GuildMarkNonZeroBytes;
        int m_GuildMarkNonZeroPixels;
        std::uint32_t m_GuildMarkByteSum;
        std::int32_t m_Level;
        std::int32_t m_Resets;
        std::int32_t m_TotalStats;
        std::int32_t m_GuildScore;
        std::int32_t m_GuildMemberCount;
        std::int32_t m_QuestCompleted;
        std::int32_t m_QuestTotal;

        CHARACTER m_Preview;
        OBJECT m_PreviewHelper;
        bool m_HasPreview;
        std::uint64_t m_InspectionSignature = 0;
        float m_PreviewHelperScale;
        float m_PreviewAngle;
        float m_PreviewZoom;
        int m_GuildMarkIndex;

        float m_PanelScale;
        int m_PanelOffsetX;
        int m_PanelOffsetY;

        bool m_DebugEnabled;
        int m_DebugElement;
        int m_CharacterColumnOffsetX;
        int m_CharacterColumnOffsetY;
        int m_CharacterNameOffsetX;
        int m_CharacterNameOffsetY;
        int m_ResetColumnOffsetX;
        int m_ResetColumnOffsetY;
        int m_ResetValueOffsetX;
        int m_ResetValueOffsetY;
        int m_RowSpacingAdjustment;

        UiRect m_TitleRect;
        UiRect m_ResetTabRect;
        UiRect m_GuildTabRect;
        UiRect m_QuestTabRect;
        UiRect m_ProfileNameRect;
        UiRect m_GuildRect;
        UiRect m_GuildMarkRect;
        UiRect m_GuildNameRect;
        UiRect m_CloseRect;
        UiRect m_EquipmentPrivacyRect{573,45,129,20};
        UiRect m_RefreshRect;
        UiRect m_PreviousRect;
        UiRect m_NextRect;
        UiRect m_StatsRect;
        UiRect m_PreviewRect;
    };
}

extern SEASON3B::CNewUICustomRanking* g_pCustomRankingWindow;
