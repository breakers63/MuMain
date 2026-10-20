#include "stdafx.h"
#include "UI/NewUI/Character/ParisPanelReference.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "Render/Textures/ZzzTexture.h"

namespace UI::Theme
{
    namespace { constexpr float SourceU = 474.f / 512.f; }

    void ParisPanelReference::Load(const wchar_t* path)
    {
        if (texture == BITMAP_UNKNOWN) texture = Bitmaps.LoadImage(path, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void ParisPanelReference::Unload()
    {
        if (texture != BITMAP_UNKNOWN) DeleteBitmap(texture);
        texture = BITMAP_UNKNOWN;
    }

    bool ParisPanelReference::Ready() const { return texture != BITMAP_UNKNOWN; }

    void ParisPanelReference::Draw(int x, int y, int width, int height) const
    {
        if (!Ready()) { UI::Paris::Frame(x, y, width, height); return; }
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(texture, x, y, width, height, 0, 0, SourceU, 1);
    }

    void ParisPanelReference::Slice(int x, int y, int width, int height, int sourceTop,
        int sourceBottom, int top, int bottom) const
    {
        RenderBitmap(texture, x, y + top, width, bottom - top, 0,
            static_cast<float>(sourceTop) / height, SourceU,
            static_cast<float>(sourceBottom - sourceTop) / height);
    }

    void ParisPanelReference::DrawVip(int x, int y, int width, int height, int extraHeight) const
    {
        if (!Ready()) { UI::Paris::Frame(x, y, width, height + extraHeight); return; }
        constexpr int HeaderEnd = 87, InformationEnd = 185, CompactEnd = 151, FooterStart = 379;
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        Slice(x, y, width, height, 0, HeaderEnd, 0, HeaderEnd);
        Slice(x, y, width, height, HeaderEnd, InformationEnd, HeaderEnd, CompactEnd);
        Slice(x, y, width, height, InformationEnd, FooterStart, CompactEnd, FooterStart + extraHeight);
        Slice(x, y, width, height, FooterStart, height, FooterStart + extraHeight, height + extraHeight);
    }
}
