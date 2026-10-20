#include "stdafx.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/Character/ParisExactAssets.h"
#include "UI/NewUI/HUD/NewUIChatLogWindow.h"

namespace UI::Theme
{
void ParisExactAssets::Load() {}
void ParisExactAssets::Unload() {}

void ParisExactAssets::Scroll(int x, int y, int w, int h, int thumbY, int thumbHeight) const
{
    const bool hovered = SEASON3B::CheckMouseIn(x, thumbY, w, thumbHeight);
    UI::Character::ParisStyle::ScrollBar(x, y, w, h, thumbY, thumbHeight, hovered);
}

int ParisExactAssets::ScrollArrow(int x, int y, int w, int h)
{
    constexpr int HitSize = 12;
    if (SEASON3B::CheckMouseIn(x + w / 2 - HitSize / 2, y - HitSize, HitSize, HitSize))
        return -1;
    if (SEASON3B::CheckMouseIn(x + w / 2 - HitSize / 2, y + h, HitSize, HitSize))
        return 1;
    return 0;
}
} // namespace UI::Theme
