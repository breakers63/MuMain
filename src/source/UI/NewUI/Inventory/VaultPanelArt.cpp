#include "stdafx.h"
#include "UI/NewUI/Inventory/VaultPanelArt.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"

namespace UI::Inventory
{
    namespace
    {
        constexpr int PanelWidth = 190;
        constexpr int PanelHeight = 429;
        constexpr int HeaderCloseLeft = 165;
        constexpr int HeaderCloseTop = 24;
        constexpr int HeaderCloseWidth = 16;
        constexpr int HeaderCloseHeight = 20;
        constexpr float AtlasColumns = 4.0f;
        constexpr float AtlasRows = 2.0f;
    } // namespace

    void VaultPanelArt::Load()
    {
        if (m_frame != BITMAP_UNKNOWN)
            return;
        m_frame = Bitmaps.LoadImage(L"Data\\Interface\\Paris_StatsBlank52.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        m_buttons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_VaultButtons58.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        m_close = Bitmaps.LoadImage(L"Data\\Interface\\Paris_ShopButtons53.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void VaultPanelArt::Unload()
    {
        if (m_frame != BITMAP_UNKNOWN)
            DeleteBitmap(m_frame);
        if (m_buttons != BITMAP_UNKNOWN)
            DeleteBitmap(m_buttons);
        if (m_close != BITMAP_UNKNOWN)
            DeleteBitmap(m_close);
        m_frame = m_buttons = m_close = BITMAP_UNKNOWN;
    }

    void VaultPanelArt::Frame(const POINT &position) const
    {
        if (m_frame == BITMAP_UNKNOWN)
            return;
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(m_frame, position.x, position.y, PanelWidth, PanelHeight, 0, 0, 1, 1);
    }

    void VaultPanelArt::Close(const POINT &position) const
    {
        if (m_close == BITMAP_UNKNOWN)
            return;
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(m_close, position.x + CloseLeft, position.y + FooterTop, ButtonSize, ButtonSize, 0, 0, .25f, .25f);
    }

    void VaultPanelArt::Button(const POINT &position, Icon icon, bool enabled) const
    {
        if (m_buttons == BITMAP_UNKNOWN)
            return;
        const bool hovered = SEASON3B::CheckMouseIn(position.x, position.y, ButtonSize, ButtonSize);
        const float shade = enabled ? (hovered ? 1.f : .9f) : .4f;
        const int tile = static_cast<int>(icon);
        EnableAlphaTest();
        UI::Paris::Color(shade, shade, shade, 1);
        RenderBitmap(m_buttons, position.x, position.y, ButtonSize, ButtonSize, (tile % 4) / AtlasColumns,
                     (tile / 4) / AtlasRows, 1 / AtlasColumns, 1 / AtlasRows);
        UI::Paris::Color(1, 1, 1, 1);
    }

    bool VaultPanelArt::ClosePressed(const POINT &position) const
    {
        if (!SEASON3B::IsPress(VK_LBUTTON))
            return false;
        const bool header = SEASON3B::CheckMouseIn(position.x + HeaderCloseLeft, position.y + HeaderCloseTop,
                                                   HeaderCloseWidth, HeaderCloseHeight);
        const bool footer =
            SEASON3B::CheckMouseIn(position.x + CloseLeft, position.y + FooterTop, ButtonSize, ButtonSize);
        if (!header && !footer)
            return false;
        UI::Paris::ConsumeMouse();
        return true;
    }
} // namespace UI::Inventory
