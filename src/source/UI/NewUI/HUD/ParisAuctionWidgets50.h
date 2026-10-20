#pragma once
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include <cmath>
#include "UI/NewUI/Widgets/ParisTintOverlay.h"
#include "UI/NewUI/Character/ParisMarketAssets.h"
namespace UI::Theme
{
    class ParisAuctionWidgets50
    {
        GLuint buttons=BITMAP_UNKNOWN, colored=BITMAP_UNKNOWN;
        ParisMarketAssets market;
    public:
        void Load(){if(buttons==BITMAP_UNKNOWN)buttons=Bitmaps.LoadImage(L"Data\\Interface\\Paris_MarketWidgetsV7.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);market.Load();if(colored==BITMAP_UNKNOWN)colored=Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingColorsV14.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);}
        void Unload(){if(buttons!=BITMAP_UNKNOWN)DeleteBitmap(buttons);buttons=BITMAP_UNKNOWN;market.Unload();if(colored!=BITMAP_UNKNOWN)DeleteBitmap(colored);colored=BITMAP_UNKNOWN;}
        void Button(int x,int y,int w,int h,bool enabled,bool active,int color=1) const
        {
            EnableAlphaTest();const float shade=enabled?1.f:.55f;UI::Paris::Color(shade,shade,shade,1);
            if(colored!=BITMAP_UNKNOWN)RenderBitmap(colored,x,y,w,h,0.f,std::clamp(color,0,2)*64.f/256.f,200.f/256.f,56.f/256.f);
            else if(buttons!=BITMAP_UNKNOWN)RenderBitmap(buttons,x,y,w,h,0,80.f/256,124.f/256,32.f/256);
            else market.Button(x,y,w,h);
            UI::Paris::Color(1,1,1,1);
            if(enabled&&active)UI::Theme::TintOverlay(x+2,y+2,w-4,h-4,.45f,.30f,.06f,.16f);
        }
        void Coin(int x,int y,int w,int h)const{market.Coin(x,y,w,h);}
        void Radio(int x,int y,int w,int h,bool selected)const
        {
            const float cx=x+w/2.f,cy=y+h/2.f,rx=w*.29f,ry=h*.29f;
            for(int step=0;step<32;++step){const float a=step*6.2831853f/32;UI::Character::ParisStyle::Solid(cx+std::cos(a)*rx,cy+std::sin(a)*ry,1.2f,1.2f,.86f,.65f,.27f);}
            if(selected)for(int row=-1;row<=1;++row)for(int col=-1;col<=1;++col)
                if(row*row+col*col<=1)UI::Theme::TintOverlay(static_cast<int>(cx)+col,static_cast<int>(cy)+row,1,1,1.f,.8f,.35f,.7f);
        }
    };
}
