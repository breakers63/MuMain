#pragma once
#include "ParisPanelStyle.h"

namespace UI::Character::ParisFlat
{
    inline void Box(int x, int y, int width, int height, bool selected = false)
    {
        const float border = selected ? .67f : .28f;
        ParisStyle::Solid(x, y, width, height, border, border * .77f, border * .36f);
        ParisStyle::Solid(x + 1, y + 1, width - 2, height - 2,
            selected ? .17f : .055f, selected ? .12f : .052f, .035f);
    }

    inline void Frame(int x, int y, int width, int height)
    {
        Box(x, y, width, height, true);
        ParisStyle::Solid(x + 2, y + 2, width - 4, height - 4, .025f, .025f, .025f);
        constexpr int cornerLength = 18;
        for (int corner = 0; corner < 4; ++corner)
        {
            const int left = x + (corner % 2 ? width - cornerLength - 6 : 6);
            const int top = y + (corner / 2 ? height - 7 : 6);
            ParisStyle::Solid(left, top, cornerLength, 1, .8f, .61f, .22f);
        }
    }
}
