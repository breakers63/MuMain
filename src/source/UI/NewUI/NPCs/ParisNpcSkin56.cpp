#include "stdafx.h"
#include "UI/NewUI/NPCs/ParisNpcSkin56.h"
#include "UI/Legacy/UIControls.h"
namespace UI::Theme
{
    bool ParisNpcSkin56::Loaded() const
    {
        return frame != BITMAP_UNKNOWN;
    }
    void ParisNpcSkin56::Load()
    {
        if (Loaded())
            return;
        frame = Bitmaps.LoadImage(L"Data\\Interface\\Paris_NpcFrame58.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        button = Bitmaps.LoadImage(L"Data\\Interface\\Paris_NpcButton56.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        crest = Bitmaps.LoadImage(L"Data\\Interface\\Paris_NpcCrest58.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        headerClose = Bitmaps.LoadImage(L"Data\\Interface\\Paris_NpcClose58.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        close = Bitmaps.LoadImage(L"Data\\Interface\\Paris_ShopButtons53.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        actionIcons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_VaultButtons58.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    }
    void ParisNpcSkin56::Unload()
    {
        if (frame != BITMAP_UNKNOWN)
            DeleteBitmap(frame);
        if (button != BITMAP_UNKNOWN)
            DeleteBitmap(button);
        if (close != BITMAP_UNKNOWN)
            DeleteBitmap(close);
        if (crest != BITMAP_UNKNOWN)
            DeleteBitmap(crest);
        if (headerClose != BITMAP_UNKNOWN)
            DeleteBitmap(headerClose);
        if (actionIcons != BITMAP_UNKNOWN)
            DeleteBitmap(actionIcons);
        frame = button = close = crest = headerClose = actionIcons = BITMAP_UNKNOWN;
    }
    void ParisNpcSkin56::Panel(float x, float y, float width, float height, bool showClose, bool showCrest) const
    {
        if (!Loaded())
            return;
        constexpr float top = 64, bottom = 45, sourceHeight = 429, frameOffset = 4;
        const float originalY = y;
        y += frameOffset;
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(frame, x, y, width, top, 0, 0, 1, top / sourceHeight);
        RenderBitmap(frame, x, y + top, width, height - top - bottom, 0, top / sourceHeight, 1,
                     (sourceHeight - top - bottom) / sourceHeight);
        RenderBitmap(frame, x, y + height - bottom, width, bottom, 0, (sourceHeight - bottom) / sourceHeight, 1,
                     bottom / sourceHeight);
        if (showCrest)
            RenderBitmap(crest, x + (width - 60) / 2, originalY + 4, 60, 40, 0, 0, 1, 1);
        if (showClose)
            RenderBitmap(headerClose, x + width - 25, originalY + 24, 16, 20, 0, 0, 1, 1);
    }
    void ParisNpcSkin56::Base(float x, float y, float width, float height, bool enabled, bool hover) const
    {
        if (button == BITMAP_UNKNOWN)
            return;
        EnableAlphaTest();
        const float shade = enabled ? (hover ? 1.f : .90f) : .42f;
        UI::Paris::Color(shade, shade, shade, 1);
        RenderBitmap(button, x, y, width, height, 0, 0, 1, 1);
        UI::Paris::Color(1, 1, 1, 1);
    }
    void ParisNpcSkin56::Caption(float x, float y, float width, float height, const wchar_t *text, bool enabled) const
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->SetTextColor(enabled ? RGBA(255, 220, 130, 255) : RGBA(125, 125, 125, 255));
        g_pRenderText->RenderText(x, y + (height - 10) / 2, text, width, 0, RT3_SORT_CENTER);
    }
    void ParisNpcSkin56::Close(float x, float y, float width, float height) const
    {
        if (close == BITMAP_UNKNOWN)
            return;
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(close, x, y, width, height, 0, 0, .25f, .25f);
    }
    void ParisNpcSkin56::NativeButton(SEASON3B::CNewUIButton &native, bool exit) const
    {
        const POINT pos = native.GetPos(), size = native.GetSize();
        native.UnRegisterButtonState();
        if (exit)
            Close(pos.x, pos.y, size.x, size.y);
        else
            Base(pos.x, pos.y, size.x, size.y, !native.IsLock(), native.GetBTState() != SEASON3B::BUTTON_STATE_UP);
        native.ChangeTextColor(native.IsLock() ? RGBA(125, 125, 125, 255) : RGBA(255, 220, 130, 255));
        native.Render();
    }
    void ParisNpcSkin56::Separator(float x, float y, float width) const
    {
        UI::Character::ParisStyle::Solid(x, y, width, 1, .64f, .43f, .09f);
    }
    void ParisNpcSkin56::ActionButton(SEASON3B::CNewUIButton &native, ActionIcon icon) const
    {
        constexpr float columns = 4, rows = 2;
        const auto position = native.GetPos();
        const auto size = native.GetSize();
        const int tile = static_cast<int>(icon);
        const float shade = native.IsLock() ? .4f : 1.f;
        if (actionIcons != BITMAP_UNKNOWN)
        {
            EnableAlphaTest();
            UI::Paris::Color(shade, shade, shade, 1);
            RenderBitmap(actionIcons, position.x, position.y, size.x, size.y, (tile % 4) / columns, (tile / 4) / rows,
                         1 / columns, 1 / rows);
            UI::Paris::Color(1, 1, 1, 1);
        }
        native.UnRegisterButtonState();
        native.Render();
    }
} // namespace UI::Theme
