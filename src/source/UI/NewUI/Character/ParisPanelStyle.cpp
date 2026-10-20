#include "stdafx.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/Legacy/UIControls.h"
namespace UI::Character::ParisStyle
{

    void Solid(float x, float y, float w, float h, float r, float g, float b)
    {
        EnableAlphaTest();
        UI::Paris::Color(r, g, b, 1.0f);
        RenderColor(x, y, w, h);
        EndRenderColor();
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
    }

    void Box(int x, int y, int w, int h, bool selected)
    {
        Solid(x, y, w, h, 0.78f, 0.60f, 0.22f);
        const float fill = selected ? 0.17f : 0.06f;
        Solid(x + 1, y + 1, w - 2, h - 2, fill, fill, 0.04f);
    }

    void ScrollBar(int x, int y, int width, int height, int thumbY, int thumbHeight, bool hovered)
    {
        constexpr float goldRed = .68f, goldGreen = .51f, goldBlue = .22f;
        Solid(x, y, width, height, goldRed, goldGreen, goldBlue);
        Solid(x + 1, y + 1, width - 2, height - 2, .025f, .025f, .025f);
        Solid(x + 2, y + 2, 1, height - 4, .16f, .13f, .08f);
        Solid(x, thumbY, width, thumbHeight, .38f, .29f, .14f);
        Solid(x + 1, thumbY + 1, width - 2, thumbHeight - 2, .07f, .06f, .045f);
        const int centerX = x + width / 2, centerY = thumbY + thumbHeight / 2;
        constexpr int jewelRadius = 7;
        for (int line = -jewelRadius; line <= jewelRadius; ++line)
        {
            const int half = jewelRadius - abs(line);
            Solid(centerX - half, centerY + line, half * 2 + 1, 1, goldRed, goldGreen, goldBlue);
            if (half > 1) Solid(centerX - half + 1, centerY + line, half * 2 - 1, 1, .10f, .085f, .06f);
        }
        Solid(centerX - 2, centerY - 2, 5, 5, hovered ? 1.f : .90f, hovered ? .79f : .63f, .14f);
        Solid(centerX - 1, centerY - 2, 2, 1, 1.f, .92f, .52f);
    }

    void Label(int x, int y, int w, const wchar_t* text)
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(235, 224, 200, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(x, y, text, w, 0, RT3_SORT_CENTER);
    }

    void Panel(GLuint texture, const POINT& position)
    {
        constexpr float u = 496.0f / 512.0f, v = 1048.0f / 2048.0f;
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(texture, position.x, position.y, 190, 429, 0, 0, u, v);
    }

    void Footer(GLuint atlas, int x, int y, int tile, bool enabled)
    {
        constexpr float uv = 0.5f;
        EnableAlphaTest();
        const float brightness = enabled ? 1.0f : 0.4f;
        UI::Paris::Color(brightness, brightness, brightness, 1);
        RenderBitmap(atlas, x, y, 36, 29, (tile % 2) * uv, (tile / 2) * uv, uv, uv);
        UI::Paris::Color(1, 1, 1, 1);
    }

}
