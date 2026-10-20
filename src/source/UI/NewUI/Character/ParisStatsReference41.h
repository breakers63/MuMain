#pragma once
#include "Render/Sprites/GlobalBitmap.h"
namespace UI::Character
{
    class ParisStatsReference41
    {
    public:
        bool Ready() const;
        void Load();
        void Unload();
        void Information(int x,int y) const;
        void Attribute(int x,int y) const;
        void Plus(int x,int y) const;
        void Points(int x,int y,int width,int height) const;
        void FooterBackground(int x,int y) const;
        void Footer(int x,int y,int index) const;
    private:
        GLuint texture = BITMAP_UNKNOWN;
        void Draw(float x, float y, float width, float height,
            int sx, int sy, int sw, int sh) const;
        void Border(float x,float y,float width,float height,
            int sx,int sy,int sw,int sh,int corner,float targetCorner) const;
    };
}
