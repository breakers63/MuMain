#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"

#include "Network/Paris/AdvancedStatsField.h"
#include <array>
#include <cstdint>

namespace SEASON3B
{
    constexpr DWORD ADVANCED_STATS_INTERFACE_KEY = 0x41535453; // 'ASTS'

    class CNewUIAdvancedStatsWindow : public CNewUIObj
    {
    public:
        enum IMAGE_LIST
{
    IMAGE_ADVANCED_STATS_TOP = 31910,
    IMAGE_ADVANCED_STATS_BOTTOM = 31911,
    IMAGE_ADVANCED_STATS_END = IMAGE_ADVANCED_STATS_BOTTOM,
};

        enum
        {
            WINDOW_WIDTH = 190,
            WINDOW_HEIGHT = 429,
            PROTOCOL_FIELD_COUNT = 52,
        };

        using Field = Network::Paris::AdvancedStatsField;

        CNewUIAdvancedStatsWindow();
        virtual ~CNewUIAdvancedStatsWindow();

        bool Create(CNewUIManager* manager);
        void Release();

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;
        float GetLayerDepth() override;
        float GetKeyEventOrder() override;

        void OpenWindow();
        void CloseWindow(bool playSound = true);
        bool IsWindowOpen() const;
        void ReceivePacket(const BYTE* packet, int size);

    private:
        enum class ValueFormat
        {
            Integer,
            Percent,
        };

        struct UiRect
        {
            int X;
            int Y;
            int Width;
            int Height;
        };

        struct RowDefinition
        {
            const wchar_t* Label;
            Field DataField;
            ValueFormat Format;
            bool Section;
        };

        void LoadImages();
        void UnloadImages();
        void AnchorToCharacter();
        void RequestSnapshot();
        void InvalidateSnapshot();
        void ClampScroll();
        void SetScrollFromThumb(int pointerY);

        void RenderPanel();
        void RenderRows();
        void RenderScrollBar();
        void RenderStatus();
        void RenderDebug();
        void RenderCodeButton(int x, int y, int width, int height, const wchar_t* text, bool hovered, bool active) const;
        void RenderValue(int x, int y, int width, const RowDefinition& row) const;

        bool CheckCloseMouse() const;
        bool CheckPanelMouse() const;
        bool CheckScrollTrackMouse() const;
        bool CheckScrollThumbMouse() const;
        int GetScrollThumbHeight() const;
        int GetScrollThumbY() const;
        int GetMaxScroll() const;
        static std::int32_t ReadInt32(const BYTE* data);

    private:
        CNewUIManager* m_Manager;
        POINT m_PanelPos;
        bool m_WindowOpen;
        bool m_HasSnapshot;
        bool m_RequestPending;
        DWORD m_LastRequestTick;
        DWORD m_RequestSentTick;
        DWORD m_LastSnapshotTick;
        std::array<std::int32_t, PROTOCOL_FIELD_COUNT> m_Values;

        int m_ScrollOffset;
        int m_ContentHeight;
        bool m_DraggingScroll;
        int m_ScrollDragOffset;

        UiRect m_CloseRect;

        bool m_DebugEnabled;
    };
}

extern SEASON3B::CNewUIAdvancedStatsWindow* g_pAdvancedStatsWindow;
