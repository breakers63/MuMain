#pragma once
#include "Render/Sprites/GlobalBitmap.h"

namespace UI::Theme
{
    class ParisPanelReference
    {
    public:
        void Load(const wchar_t* path);
        void Unload();
        bool Ready() const;
        void Draw(int x, int y, int width, int height) const;
        void DrawVip(int x, int y, int width, int height, int extraHeight) const;
    private:
        GLuint texture = BITMAP_UNKNOWN;
        void Slice(int x, int y, int width, int height, int sourceTop, int sourceBottom,
            int top, int bottom) const;
    };
}
