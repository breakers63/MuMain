#include "stdafx.h"
#include "UI/NewUI/Character/ParisControls51.h"

namespace UI::Theme
{
void ParisControls51::Part(int x,int y,int w,int h,int sx,int sy,int sw,int sh) const
{
        EnableAlphaTest();UI::Paris::Color(1,1,1,1);
        RenderBitmap(texture,x,y,w,h,(sx+.5f)/1024.f,(sy+.5f)/1024.f,(sw-1.f)/1024.f,(sh-1.f)/1024.f);
    }

void ParisControls51::Hover(int x,int y,int w,int h) const
{
        if(!SEASON3B::CheckMouseIn(x,y,w,h))return;
        EnableAlphaTest();UI::Paris::Color(.85f,.55f,.15f,MouseLButton?.12f:.06f);
        RenderColor(x+5,y+4,w-10,h-8);EndRenderColor();UI::Paris::Color(1,1,1,1);
    }

void ParisControls51::Load()
{texture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_Controls51.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);}

void ParisControls51::Unload()
{if(texture!=BITMAP_UNKNOWN)DeleteBitmap(texture);texture=BITMAP_UNKNOWN;}

bool ParisControls51::Ready() const
{return texture!=BITMAP_UNKNOWN;}

void ParisControls51::Tab(int tier,bool selected,int x,int y,int w,int h) const
{
        if(tier<1||tier>3||!Ready())return;
        Part(x,y,w,h,0,selected?0:140,163,116);
        Part(x+(w-17)/2,y+4,17,17,220+(tier-1)*64,0,49,59);
        Hover(x,y,w,h);
    }

void ParisControls51::Purchase(int x,int y,int w,int h) const
{if(Ready()){Part(x,y,w,h,0,300,364,80);Hover(x,y,w,h);}}

void ParisControls51::Reset(int x,int y,int w,int h) const
{if(Ready()){Part(x,y,w,h,0,420,425,94);Hover(x,y,w,h);}}
}
