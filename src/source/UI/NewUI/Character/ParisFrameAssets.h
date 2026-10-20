#pragma once
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"

#include "UI/NewUI/Character/ClassicPanelFrame.h"
namespace UI::Theme
{
    // Each window owns a bitmap reference and releases it with its existing image lifecycle.
    class ParisFrameAssets
    {
    public:
        GLuint frame = BITMAP_UNKNOWN;
        GLuint buttons = BITMAP_UNKNOWN;
        GLuint inventoryButtons = BITMAP_UNKNOWN, shopButtons = BITMAP_UNKNOWN, statsButton = BITMAP_UNKNOWN;
        void Load();
        void Unload();
        void Frame(int x, int y, int w = 190, int h = 429, bool borderOnly = false) const;
        void Stone(int x, int y, int w, int h, bool light = false, bool brown = false) const;
        void Exit(int x, int y) const;
        void SpriteButton(int x,int y,int w,int h,int tile,bool enabled=true,bool shop=false) const;
        void InventoryButton(int x, int y, int w, int h, GLuint icon, bool enabled = true, bool expansion = false, float sourceScale = 1.f) const;
        void InventoryGrid(int x, int y, int columns = 8, int rows = 4) const;
    };

}
