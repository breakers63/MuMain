#pragma once
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/Widgets/ParisTintOverlay.h"

namespace UI::Theme
{
    // Original approved artwork, sampled by native UVs; live item data and
    // captions are drawn separately, never baked sample listings.
    class ParisFeatureSkin49
    {
        GLuint texture = BITMAP_UNKNOWN;
    public:
        void Load() { if (texture == BITMAP_UNKNOWN) texture = Bitmaps.LoadImage(L"Data\\Interface\\Paris_AuctionReference49.tga", GL_LINEAR, GL_CLAMP_TO_EDGE); }
        void Unload() { if (texture != BITMAP_UNKNOWN) DeleteBitmap(texture); texture = BITMAP_UNKNOWN; }
        void Sprite(float x,float y,float w,float h,int sx,int sy,int sw,int sh,float shade=1) const
        {
            if (texture == BITMAP_UNKNOWN) return;
            EnableAlphaTest(); UI::Paris::Color(shade,shade,shade,1);
            RenderBitmap(texture,x,y,w,h,sx/2048.f,sy/1024.f,sw/2048.f,sh/1024.f);
            UI::Paris::Color(1,1,1,1);
        }
        void Fill(float x,float y,float w,float h,float r=.018f,float g=.021f,float b=.018f) const
        { UI::Character::ParisStyle::Solid(x,y,w,h,r,g,b); }
        void Outline(float x,float y,float w,float h,bool active=false) const
        {
            const float r=active?.92f:.37f,g=active?.66f:.29f,b=active?.18f:.14f;
            Fill(x,y,w,1,r,g,b); Fill(x,y+h-1,w,1,r,g,b);
            Fill(x,y,1,h,r,g,b); Fill(x+w-1,y,1,h,r,g,b);
        }
        void Frame(float x,float y,float w,float h) const
        {
            Fill(x,y,w,h);
            constexpr int sx[]={0,28,826,854},sy[]={0,24,554,584};
            const float dx[]={x,x+22,x+w-22,x+w},dy[]={y,y+20,y+h-24,y+h};
            for(int row=0;row<3;++row) for(int col=0;col<3;++col)
                if(row!=1||col!=1) Sprite(dx[col],dy[row],dx[col+1]-dx[col],dy[row+1]-dy[row],sx[col],sy[row],sx[col+1]-sx[col],sy[row+1]-sy[row]);
        }
        void Brand(float x,float y,float w,float h) const { Sprite(x,y,w,h,32,17,264,58); }
        void Heading(int tab,float x,float y,float w,float h)const
        {
            constexpr int sx[]={315,1092,171,704,1158},sy[]={12,12,609,609,609},sw[]={430,369,290,286,321},sh[]={65,65,55,55,55};
            tab=std::clamp(tab,0,4);const float fit=std::min(w/sw[tab],h/sh[tab]);
            const float width=sw[tab]*fit,height=sh[tab]*fit;
            Sprite(x+(w-width)/2,y+(h-height)/2,width,height,sx[tab],sy[tab],sw[tab],sh[tab]);
        }
        void Close(float x,float y,float w,float h) const { Sprite(x,y,w,h,797,22,37,39); }
        void Base(float x,float y,float w,float h,bool enabled=true,bool active=false) const
        {
            Fill(x,y,w,h);
            if(active&&enabled) for(int row=0;row<24;++row)
            {
                const float t=static_cast<float>(row)/23;
                Fill(x+4,y+4+(h-8)*t,w-8,(h-8)/23+1,.27f+.28f*(1-t),.12f+.15f*(1-t),.025f);
            }
            const int srcY=active?95:149;
            constexpr int sx[]={36,45,158,169}, offsets[]={0,8,35,43};
            const float dx[]={x,x+7,x+w-7,x+w},dy[]={y,y+7,y+h-7,y+h};
            for(int row=0;row<3;++row) for(int col=0;col<3;++col)
                if(row!=1||col!=1) Sprite(dx[col],dy[row],dx[col+1]-dx[col],dy[row+1]-dy[row],sx[col],srcY+offsets[row],sx[col+1]-sx[col],offsets[row+1]-offsets[row],enabled?1.f:.5f);
        }
        void Separator(float x,float y,float w) const { Fill(x,y,w,1,.26f,.25f,.21f); }
        void Caption(float x,float y,float w,float h,const wchar_t* caption,bool enabled=true) const
        {
            EnableAlphaTest();UI::Paris::Color(1,1,1,1);g_pRenderText->SetFont(g_hFontBold);g_pRenderText->SetBgColor(0,0,0,0);
            g_pRenderText->SetTextColor(enabled?RGBA(250,240,218,255):RGBA(130,130,125,255));
            g_pRenderText->RenderText(x,y+(h-12)/2,caption,w,0,RT3_SORT_CENTER);
        }
    };
}
