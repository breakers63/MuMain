#include "stdafx.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/Legacy/UIControls.h"
namespace UI::Theme
{
    void HoverTip(int x,int y,int w,int h,const wchar_t* text)
    {
        if(!SEASON3B::CheckMouseIn(x,y,w,h) || !text || !*text) return;
        g_pRenderText->SetFont(g_hFont);
        const SIZE size = g_pRenderText->MeasureText(text, static_cast<int>(wcslen(text)));
        const float scaleX=g_fScreenRate_x>0?g_fScreenRate_x:1.f;
        const float scaleY=g_fScreenRate_y>0?g_fScreenRate_y:1.f;
        constexpr int padding=6;
        const int screenWidth=static_cast<int>(WindowWidth/scaleX);
        const int width=std::min(screenWidth,static_cast<int>(size.cx/scaleX)+padding);
        const int left=std::clamp(x+(w-width)/2,0,std::max(0,screenWidth-width));
        const int top=std::max(0,y-static_cast<int>(size.cy/scaleY)-3);
        g_pRenderText->SetTextColor(235,224,200,255);
        g_pRenderText->SetBgColor(0,0,0,200);
        g_pRenderText->RenderText(left,top,text,width,0,RT3_SORT_CENTER);
        g_pRenderText->SetBgColor(0,0,0,0);
    }

    void Bevel(int x, int y, int w, int h, const wchar_t* text, bool active, bool enabled)
    {
        using namespace Character::ParisStyle;
        Solid(x,y,w,h,.13f,.11f,.08f);
        Solid(x+1,y+1,w-2,h-2,.48f,.39f,.24f);
        Solid(x+2,y+2,w-4,h-4,active ? .24f : .13f,active ? .18f : .12f,.08f);
        Solid(x+3,y+3,w-6,1,.37f,.32f,.23f);
        if (text == nullptr || text[0] == L'\0') return;
        g_pRenderText->SetFont(g_hFont); g_pRenderText->SetBgColor(0,0,0,0);
        g_pRenderText->SetTextColor(enabled ? 240 : 120, enabled ? 218 : 115, enabled ? 163 : 100,255);
        g_pRenderText->RenderText(x,y+std::max(0,(h-10)/2),text,w,0,RT3_SORT_CENTER);
    }


    void IconButton(int x, int y, int w, int h, Symbol symbol, bool enabled)
    {
        using namespace Character::ParisStyle;
        Bevel(x,y,w,h,L"",false,enabled);
        const float gold=enabled ? .90f:.40f;
        auto line=[&](int ax,int ay,int bx,int by) {
            const int steps=std::max(std::abs(bx-ax),std::abs(by-ay));
            for(int i=0;i<=steps;++i)
                Solid(x+w/2+ax+(bx-ax)*i/std::max(1,steps),y+h/2+ay+(by-ay)*i/std::max(1,steps),1.5f,1.5f,gold,gold*.77f,gold*.33f);
        };
        if(symbol==Symbol::Close || symbol==Symbol::StopShop) {
            line(-5,-5,5,5); line(-5,5,5,-5);
            if(symbol==Symbol::Close) return;
        }
        if(symbol==Symbol::Shop || symbol==Symbol::StopShop || symbol==Symbol::Offline) {
            line(-6,-3,6,-3); line(-6,-3,-6,6); line(-6,6,6,6); line(6,6,6,-3);
            line(-6,-4,-3,-7); line(-3,-7,3,-7); line(3,-7,6,-4);
            line(-2,6,-2,1);line(-2,1,2,1);line(2,1,2,6);
            if(symbol==Symbol::Offline) line(-7,8,7,8);
        } else if(symbol==Symbol::Repair) {
            line(-5,6,4,-3);line(0,-6,6,0);line(1,-7,7,-1);
        } else if(symbol==Symbol::Expand) {
            line(-6,-6,6,-6);line(-6,-6,-6,6);line(-6,6,6,6);line(6,6,6,-6);
            line(-3,0,3,0);line(0,-3,0,3);
        } else if(symbol==Symbol::Info) {
            line(-1,-6,0,-6);line(-1,-2,0,-2);line(0,-2,0,5);line(-2,5,2,5);
        } else if(symbol==Symbol::Ranking) {
            line(-6,5,-6,0);line(-2,5,-2,-6);line(2,5,2,-3);line(6,5,6,1);line(-7,7,7,7);
        } else if(symbol==Symbol::Vip) {
            line(-6,-3,-4,5);line(-4,5,4,5);line(4,5,6,-3);
            line(-6,-3,-2,0);line(-2,0,0,-6);line(0,-6,2,0);line(2,0,6,-3);
        }
    }

    void Check(int x, int y, bool checked)
    {
        using namespace Character::ParisStyle;
        Solid(x,y,13,13,.43f,.39f,.29f); Solid(x+1,y+1,11,11,.95f,.95f,.90f);
        if (!checked) return;
        for(int i=0;i<3;++i) Solid(x+2+i,y+6+i,2,2,.07f,.25f,.09f);
        for(int i=0;i<6;++i) Solid(x+4+i,y+8-i,2,2,.07f,.25f,.09f);
    }

}
