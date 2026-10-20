#pragma once
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "ParisPanelStyle.h"
namespace UI::Character
{
inline void RenderRankingBorder(GLuint image, int x, int y, int width, int height)
{
    constexpr float Margin = .012f;
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    const int edgeX = std::max(3, static_cast<int>(width * Margin));
    const int edgeY = std::max(3, static_cast<int>(height * Margin));
    RenderBitmap(image, x, y, width, edgeY, 0, 0, 1, Margin);
    RenderBitmap(image, x, y + height - edgeY, width, edgeY, 0, 1 - Margin, 1, Margin);
    RenderBitmap(image, x, y + edgeY, edgeX, height - 2 * edgeY, 0, Margin, Margin, 1 - 2 * Margin);
    RenderBitmap(image, x + width - edgeX, y + edgeY, edgeX, height - 2 * edgeY, 1 - Margin, Margin, Margin,
                 1 - 2 * Margin);
}
} // namespace UI::Character
