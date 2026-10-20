#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/Character/NewUICharacterInfoWindow.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/Scaling/UITransform.h"

namespace UI::Paris
{
    void Color(float red, float green, float blue, float alpha)
    {
        constexpr float ByteScale = 255.0f;
        SetRenderColor(static_cast<BYTE>(red * ByteScale), static_cast<BYTE>(green * ByteScale),
            static_cast<BYTE>(blue * ByteScale), static_cast<BYTE>(alpha * ByteScale));
    }

    void ClassicFrame(int x, int y, int width, int height)
    {
        using Frame = SEASON3B::CNewUICharacterInfoWindow;
        constexpr float TopHeight = 64.0f, BottomHeight = 45.0f, SideWidth = 21.0f;
        const float top = std::min(TopHeight, height / 4.0f);
        const float bottom = std::min(BottomHeight, height / 4.0f);
        const float side = std::min(SideWidth, width / 8.0f);
        EnableAlphaTest();
        Color(1, 1, 1, 1);
        SEASON3B::RenderImage(Frame::IMAGE_CHAINFO_BACK, x, y, width, height);
        SEASON3B::RenderImage(Frame::IMAGE_CHAINFO_TOP, x, y, width, top);
        SEASON3B::RenderImage(Frame::IMAGE_CHAINFO_LEFT, x, y + top, side, height - top - bottom);
        SEASON3B::RenderImage(Frame::IMAGE_CHAINFO_RIGHT, x + width - side, y + top, side, height - top - bottom);
        SEASON3B::RenderImage(Frame::IMAGE_CHAINFO_BOTTOM, x, y + height - bottom, width, bottom);
    }

    namespace
    {
        GLuint goldFrame = BITMAP_UNKNOWN;
        constexpr float TextureWidth = 512.0f, TextureHeight = 1024.0f;
        constexpr int CloseInset = 21, CloseTop = 7, CloseWidth = 13, CloseHeight = 12;
        void GoldPart(int x, int y, int w, int h, int sx, int sy, int sw, int sh)
        {
            if (goldFrame == BITMAP_UNKNOWN || w <= 0 || h <= 0) return;
            EnableAlphaTest();
            Color(1, 1, 1, 1);
            RenderBitmap(goldFrame, x, y, w, h, (sx + .5f) / TextureWidth,
                (sy + .5f) / TextureHeight, (sw - 1.f) / TextureWidth, (sh - 1.f) / TextureHeight);
        }
    }

    void LoadGoldTheme()
    {
        if (goldFrame == BITMAP_UNKNOWN)
            goldFrame = Bitmaps.LoadImage(L"Data\\Interface\\Paris_GoldFrame.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void UnloadGoldTheme()
    {
        if (goldFrame != BITMAP_UNKNOWN) DeleteBitmap(goldFrame);
        goldFrame = BITMAP_UNKNOWN;
    }

    void GoldBorder(int x, int y, int width, int height)
    {
        constexpr int Corner = 12, SourceCorner = 32, SourceBottom = 992;
        const int edge = std::min(Corner, std::min(width, height) / 3);
        GoldPart(x, y, width, height, 45, 220, 420, 180);
        GoldPart(x, y, edge, edge, 0, SourceBottom, SourceCorner, SourceCorner);
        GoldPart(x + width - edge, y, edge, edge, 480, SourceBottom, SourceCorner, SourceCorner);
        GoldPart(x, y + height - edge, edge, edge, 0, SourceBottom, SourceCorner, SourceCorner);
        GoldPart(x + width - edge, y + height - edge, edge, edge, 480, SourceBottom, SourceCorner, SourceCorner);
        constexpr int Line = 2;
        GoldPart(x + edge, y, width - edge * 2, Line, 40, 1010, 430, 6);
        GoldPart(x + edge, y + height - Line, width - edge * 2, Line, 40, 1010, 430, 6);
        GoldPart(x, y + edge, Line, height - edge * 2, 16, 140, 6, 700);
        GoldPart(x + width - Line, y + edge, Line, height - edge * 2, 488, 140, 6, 700);
    }

    void Frame(int x, int y, int width, int height)
    {
        if (goldFrame == BITMAP_UNKNOWN) { ClassicFrame(x, y, width, height); return; }
        constexpr int Top = 42, Bottom = 28, Side = 9;
        constexpr int MinimumPanelHeight = 100;
        if (height < MinimumPanelHeight) { GoldBorder(x, y, width, height); return; }
        // Keep the transparent outer silhouette: only the interior needs a backing.
        GoldPart(x + Side, y + Top, width - Side * 2, height - Top - Bottom, 45, 220, 420, 180);
        GoldPart(x, y, width, Top, 0, 0, 512, 128);
        GoldPart(x, y + Top, Side, height - Top - Bottom, 0, 128, 25, 800);
        GoldPart(x + width - Side, y + Top, Side, height - Top - Bottom, 487, 128, 25, 800);
        GoldPart(x, y + height - Bottom, width, Bottom, 0, 928, 512, 96);
        // Erase the source close button and put its live target at the native hit rectangle.
        GoldPart(x + width * 444 / 512, y + Top * 54 / 128, width * 52 / 512 + 1,
            Top * 58 / 128 + 1, 45, 220, 420, 180);
        GoldPart(x + width - CloseInset, y + CloseTop, CloseWidth, CloseHeight, 444, 54, 52, 58);
    }

    void Clip(int x, int y, int width, int height)
    {
        const auto rectangle = UI::Scaling::ViewportForLogicalRect(
            UI::Scaling::GetActiveTransform(), x, y, width, height);
        mu::GetRenderer().SetScissor(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
    }

    void ResetClip()
    {
        mu::GetRenderer().SetScissor(0, 0, WindowWidth, WindowHeight);
    }

    void ConsumeMouse()
    {
        MouseLButton = MouseLButtonPush = MouseLButtonPop = false;
    }
}
