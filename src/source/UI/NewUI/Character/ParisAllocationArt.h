#pragma once
#include "Render/Sprites/GlobalBitmap.h"
namespace UI::Character
{
class ParisAllocationArt
{
public:
    enum Control
    {
        Input,
        Available,
        Checkbox,
        Amounts,
        Percent,
        Assign,
        Close
    };
    void Load();
    void Unload();
    void Frame(int x, int y, int width, int height, Control control) const;

private:
    GLuint texture = BITMAP_UNKNOWN;
    void Draw(int x, int y, int width, int height, int sx, int sy, int sw, int sh) const;
};
} // namespace UI::Character
