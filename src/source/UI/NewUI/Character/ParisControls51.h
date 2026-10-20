#pragma once
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
namespace UI::Theme {
class ParisControls51 {
    GLuint texture=BITMAP_UNKNOWN;
    void Part(int x,int y,int w,int h,int sx,int sy,int sw,int sh) const;
    void Hover(int x,int y,int w,int h) const;
public:
    void Load();
    void Unload();
    bool Ready() const;
    void Tab(int tier,bool selected,int x,int y,int w,int h) const;
    void Purchase(int x,int y,int w,int h) const;
    void Reset(int x,int y,int w,int h) const;
};
}
