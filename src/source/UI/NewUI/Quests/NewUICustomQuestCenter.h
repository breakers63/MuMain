#pragma once

// ETRO_QUEST_SIDE_PANEL_STAGE1_TEMP_FLOW_20260820

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"

#include <cstdint>
#include <vector>

namespace SEASON3B
{
    constexpr DWORD CUSTOM_QUEST_CENTER_INTERFACE_KEY = 0x43515354; // 'CQST'

    enum class CustomQuestClientState : BYTE
    {
        Locked = 1,
        Available = 2,
        Active = 3,
        ReadyToClaim = 4,
        Claimed = 5,
        Cooldown = 6,
    };

    struct CustomQuestRewardView
    {
        BYTE Type = 0;
        std::uint32_t Amount = 0;
        BYTE ItemGroup = 0;
        std::uint16_t ItemNumber = 0;
        BYTE ItemLevel = 0;
        BYTE Durability = 0;
        BYTE OptionLevel = 0;
        BYTE ExcellentMask = 0;
        BYTE AncientDiscriminator = 0;
        BYTE AncientBonusLevel = 0;
        BYTE ItemFlags = 0;
        wchar_t DisplayName[64] = {};
    };

    struct CustomQuestDefinitionView
    {
        std::int32_t Id = 0;
        BYTE Flags = 0;
        BYTE Category = 0;
        CustomQuestClientState State = CustomQuestClientState::Locked;
        std::uint16_t SortOrder = 0;
        std::uint16_t MinimumLevel = 0;
        std::uint16_t MaximumLevel = 0;
        std::uint32_t CurrentKills = 0;
        std::uint32_t RequiredKills = 0;
        std::uint16_t MonsterNumber = 0;
        std::uint32_t RemainingCooldownSeconds = 0;
        BYTE RewardCount = 0;
        wchar_t Name[65] = {};
        wchar_t Description[193] = {};
        wchar_t ObjectiveName[65] = {};
        CustomQuestRewardView Rewards[5] = {};
    };

    struct CustomQuestRewardPreviewInfo
    {
        bool Active = false;
        int X = 0;
        int Y = 0;
        int Width = 0;
        int Height = 0;
        int ItemType = -1;
        int Level = 0;
        int ExcellentFlags = 0;
        int AncientDiscriminator = 0;
        wchar_t Name[64] = {};
    };

    struct CustomQuestMonsterPreviewInfo
    {
        bool Active = false;
        std::uint16_t MonsterNumber = 0;
        int X = 0;
        int Y = 0;
        int Width = 0;
        int Height = 0;
        wchar_t Name[65] = {};
    };

    enum class CustomQuestUiElement : int
    {
        LauncherButton = 0,
        DragArea,
        CloseButton,
        QuestTitle,
        MiniMapLabel,
        QuestMissionHover,
        QuestRow1,
        QuestRow2,
        QuestRow3,
        QuestRow4,
        QuestText1,
        QuestText2,
        QuestText3,
        QuestText4,
        MissionTitle,
        MissionState,
        MissionDescription,
        MissionObjective,
        RewardSlot1,
        RewardSlot2,
        RewardSlot3,
        RewardSlot4,
        RewardSlot5,
        MinimumLevel,
        ActionButton,
        PagePrevious,
        PageNext,
        PageText,
        StatusText,
        Count,
    };

    struct CustomQuestUiRect
    {
        const wchar_t* Name = L"";
        int X = 0;
        int Y = 0;
        int Width = 0;
        int Height = 0;
        bool ScreenSpace = false;
    };

    class CNewUICustomQuestCenter : public CNewUIObj
    {
    public:
        // Quest artwork deliberately uses dynamic non-named bitmap IDs.
        // Never reserve fixed numeric IDs here: this client already has several
        // custom UI systems and fixed IDs can overwrite unrelated textures.

        enum
{
    WINDOW_WIDTH = 220,
    WINDOW_HEIGHT = 258,
    QUESTS_PER_PAGE = 4,
    MAX_REWARDS = 5,
};

        CNewUICustomQuestCenter();
        virtual ~CNewUICustomQuestCenter();

        bool Create(CNewUIManager* manager, int x, int y);
        void Release();

        void SetPos(int x, int y);
        void ToggleWindow();
        void OpenWindow();
        void CloseWindow();
        bool IsWindowOpen() const;

        // Called from the native HUD 3D pass. Rendering item models from the
        // normal 2D quest pass makes them disappear on this MuMain branch.
        void RenderRewardItems3D();
        bool GetHoveredRewardPreview(CustomQuestRewardPreviewInfo& preview) const;
        bool GetHoveredMonsterPreview(CustomQuestMonsterPreviewInfo& preview) const;

        bool UpdateMouseEvent();
        bool UpdateKeyEvent();
        bool Update();
        bool Render();
        float GetLayerDepth();

        void ReceivePacket(const BYTE* packet, int size);
        void ClearCatalog();
        void RequestCatalog();
        const std::vector<CustomQuestDefinitionView>& MissionCatalog() const { return m_Quests; }
        bool IsCatalogLoading() const { return m_LoadingCatalog; }
        std::uint64_t CatalogRevision() const { return m_CatalogRevision; }
        void SendAcceptRequest(std::int32_t questId);
        void SendClaimRequest(std::int32_t questId);

    private:
        enum QuestTextureSlot
        {
            TextureQuestButton = 0,
            TextureQuestPanel,
            TextureQuestTracker,
            TextureQuestTrackerExpanded,
            TextureCount,
        };

        enum QuestInteractionDebugSlot
        {
            InteractionQuestRowHover = 0,
            InteractionQuestButtonHover,
            InteractionRewardHover,
            InteractionCount,
        };

        enum QuestTextDebugSlot
        {
            TextPanelTitle = 0,
            TextPanelSubtitle,
            TextCloseButton,
            TextQuestName,
            TextQuestExpandSymbol,
            TextRequirementsTitle,
            TextObjective,
            TextRewardsTitle,
            TextReward1,
            TextReward2,
            TextReward3,
            TextReward4,
            TextReward5,
            TextActionButton,
            TextStatus,
            TextPageCounter,
            TextTrackerLabel,
            TextTrackerProgress,
            TextCount,
        };

        void LoadImages();
        void UnloadImages();

        void ReceiveCatalogBegin(const BYTE* packet, int size);
        void ReceiveQuestDefinition(const BYTE* packet, int size);
        void ReceiveCatalogEnd(const BYTE* packet, int size);
        void ReceiveProgressUpdate(const BYTE* packet, int size);
        void ReceiveActionResult(const BYTE* packet, int size);

        void RenderLauncherButton();
        void RenderQuestTracker();
        void RenderWindow();
        void RenderPanelLabels();
        void RenderQuestButtons();
        void RenderQuestDetails();
        void RenderRewardSlotHover();
        void RenderRewardItemTooltip();
        void RenderHoverPreviewPanel();
        void RenderActionButton();
        void RenderPageControls();
        void RenderWrappedText(const wchar_t* text, int x, int y, int width, int maxLines, int lineHeight);
        void RenderStatusText();
        void RenderStateText(const CustomQuestDefinitionView& quest, int x, int y, int width);

        bool ProcessLauncherButton();
        bool ProcessTrackerClick();
        bool ProcessWindowButtons();
        bool ProcessQuestSelection();
        bool ProcessRewardsToggle();
        bool ProcessActionButton();
        bool ProcessPageControls();
        bool ProcessDragging();

        int GetPageCount() const;
        int GetQuestIndexForRow(int row) const;
        int GetSelectedRowOnPage() const;
        int GetCurrentProgressiveQuestIndex() const;
        int GetProgressiveSequenceNumber(int actualIndex) const;
        void BuildVisibleQuestIndices(std::vector<int>& indices) const;
        void AutoAcceptCurrentProgressiveQuest();
        const CustomQuestDefinitionView* GetTrackedQuest() const;
        CustomQuestUiRect GetTemporaryLauncherRect() const;
        CustomQuestUiRect GetTemporaryPanelRect() const;
        CustomQuestUiRect GetTemporaryTrackerRect() const;
        CustomQuestUiRect GetTemporaryTrackerExpandedRect() const;
        CustomQuestUiRect GetTemporaryQuestRowRect(int row) const;
        CustomQuestUiRect GetTemporaryObjectiveRect() const;
        CustomQuestUiRect GetTemporaryRewardsHeaderRect() const;
        CustomQuestUiRect GetTemporaryRewardLineRect(int rewardIndex) const;
        CustomQuestUiRect GetTemporaryActionRect() const;
        CustomQuestUiRect GetTemporaryCloseRect() const;
        CustomQuestUiRect GetTemporaryPagePreviousRect() const;
        CustomQuestUiRect GetTemporaryPageNextRect() const;
        CustomQuestUiRect GetQuestRowHoverRect(int row) const;
        CustomQuestUiRect GetQuestButtonHoverRect() const;
        CustomQuestDefinitionView* GetSelectedQuest();
        const CustomQuestDefinitionView* GetSelectedQuest() const;
        CustomQuestDefinitionView* FindQuest(std::int32_t questId);

        void SelectFirstQuestOnPage();
        void SetStatus(const wchar_t* text);
        const wchar_t* GetStateText(CustomQuestClientState state) const;
        const wchar_t* GetActionResultText(BYTE result) const;

        void InitializeTextureDebugLayout();
        void ResetTextureDebugElement(int index);
        bool ProcessTextureDebugKeyEvent();
        void RenderTextureDebugOverlay();
        void ExportTextureDebugLayout();
        const CustomQuestUiRect& GetTextureDebugRect(int index) const;

        void InitializeInteractionDebugLayout();
        bool ProcessInteractionDebugKeyEvent();
        void RenderInteractionDebugOverlay();
        void ExportInteractionDebugLayout();
        void ResetInteractionDebugElement(int index);
        CustomQuestUiRect GetInteractionDebugScreenRect(int index) const;

        void InitializeTextDebugLayout();
        bool ProcessTextDebugKeyEvent();
        void RenderTextDebugOverlay();
        void ExportTextDebugLayout();
        void ResetTextDebugElement(int index);
        const CustomQuestUiRect& GetTextDebugOffset(int index) const;
        CustomQuestUiRect GetTextDebugScreenRect(int index) const;
        int GetTextOffsetX(int index) const;
        int GetTextOffsetY(int index) const;

        void InitializeDebugLayout();
        bool ProcessDebugKeyEvent();
        void RenderDebugOverlay();
        void RenderDebugRectangle(const CustomQuestUiRect& rectangle, bool selected);
        void ExportDebugLayout();
        void ResetSelectedDebugElement();
        void SetPanelScalePercent(int percent);
        int GetPanelWidth() const;
        int GetPanelHeight() const;
        int ScalePanelValue(int value) const;
        CustomQuestUiRect& GetLayout(CustomQuestUiElement element);
        const CustomQuestUiRect& GetLayout(CustomQuestUiElement element) const;
        int GetLayoutScreenX(const CustomQuestUiRect& rectangle) const;
        int GetLayoutScreenY(const CustomQuestUiRect& rectangle) const;
        int GetLayoutScreenWidth(const CustomQuestUiRect& rectangle) const;
        int GetLayoutScreenHeight(const CustomQuestUiRect& rectangle) const;
        bool CheckLayoutMouse(CustomQuestUiElement element) const;
        CustomQuestUiElement GetQuestRowElement(int row) const;
        CustomQuestUiElement GetQuestTextElement(int row) const;
        CustomQuestUiElement GetRewardSlotElement(int rewardIndex) const;
        int GetHoveredRewardIndex() const;
        CustomQuestUiRect GetRewardHoverRect(int rewardIndex) const;
        CustomQuestUiRect GetRewardTooltipRect() const;
        CustomQuestUiRect GetHoverPreviewRect() const;

        static std::uint16_t ReadUInt16(const BYTE* data);
        static std::uint32_t ReadUInt32(const BYTE* data);
        static std::int32_t ReadInt32(const BYTE* data);
        static void WriteInt32(BYTE* data, std::int32_t value);
        static void CopyFixedUtf8(wchar_t* destination, std::size_t destinationCount, const BYTE* source, std::size_t sourceLength);

    private:
        CNewUIManager* m_Manager;
        POINT m_Pos;
        bool m_WindowOpen;
        bool m_Dragging;
        int m_DragOffsetX;
        int m_DragOffsetY;
        int m_Page;
        int m_SelectedIndex;
        bool m_RewardsExpanded;
        std::int32_t m_TrackedQuestId;
        bool m_TrackerExpanded;
        bool m_AutoCatalogRequested;
        bool m_LoadingCatalog;
        std::uint16_t m_ExpectedQuestCount;
        DWORD m_LastCatalogRequestTick;
        wchar_t m_StatusText[160];
        int m_PanelScalePercent;
        GLuint m_TextureIds[TextureCount];
        bool m_TextureLoaded[TextureCount];
        bool m_TextureDebugEnabled;
        int m_TextureDebugIndex;
        CustomQuestUiRect m_TextureLayout[TextureCount];
        CustomQuestUiRect m_DefaultTextureLayout[TextureCount];
        bool m_InteractionDebugEnabled;
        int m_InteractionDebugIndex;
        CustomQuestUiRect m_InteractionDebugLayout[InteractionCount];
        CustomQuestUiRect m_DefaultInteractionDebugLayout[InteractionCount];
        bool m_TextDebugEnabled;
        int m_TextDebugIndex;
        CustomQuestUiRect m_TextDebugLayout[TextCount];
        CustomQuestUiRect m_DefaultTextDebugLayout[TextCount];
        bool m_DebugEnabled;
        int m_DebugElementIndex;
        CustomQuestUiRect m_Layout[static_cast<int>(CustomQuestUiElement::Count)];
        CustomQuestUiRect m_DefaultLayout[static_cast<int>(CustomQuestUiElement::Count)];
        std::vector<CustomQuestDefinitionView> m_Quests;
        std::uint64_t m_CatalogRevision = 0;
    };
}

extern SEASON3B::CNewUICustomQuestCenter* g_pCustomQuestCenter;
