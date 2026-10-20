#pragma once
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
namespace UI::Theme
{
class ParisExactAssets
{

public:
    void Load();
    void Unload();
    void Scroll(int x, int y, int w, int h, int thumbY, int thumbHeight) const;
    static int ScrollArrow(int x, int y, int w, int h);
};
} // namespace UI::Theme
