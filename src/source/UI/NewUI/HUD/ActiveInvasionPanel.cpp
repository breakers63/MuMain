#include "stdafx.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzTexture.h"
#include "ActiveInvasionPanel.h"
#include "GameLogic/Events/ActiveInvasionState.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/HUD/ParisOverlayInput.h"
#include "Render/UI/MonsterPreview.h"
#include <SDL3/SDL_timer.h>
#include <algorithm>
namespace UI::Invasions
{
namespace
{
    GameLogic::Events::ActiveInvasionState state;
    using Event=GameLogic::Events::ActiveInvasionState::Event;
    using Monster=GameLogic::Events::ActiveInvasionState::Monster;
    constexpr int X=90,Y=49,Width=116,Height=142,RowHeight=11,Rows=10,PreviewX=X+Width+6,PreviewWidth=122,PreviewHeight=166;
    constexpr int ButtonX=2,ButtonY=21,ButtonWidth=87,ButtonHeight=21;
    GLuint on=BITMAP_UNKNOWN,off=BITMAP_UNKNOWN;
    bool open=false,previewReady=false;
    int selected=-1,hovered=-1,scroll=0;
    const Event* Selected()
    {
        if(selected>=0&&selected<state.events.size()&&state.events[selected].used) return &state.events[selected];
        for(size_t i=0;i<state.events.size();++i) if(state.events[i].used) {selected=static_cast<int>(i);return &state.events[i];}
        selected=-1;return nullptr;
    }
    const Monster* Hovered()
    {
        const auto* event=Selected();
        return event&&hovered>=0&&hovered<event->monsters.size()&&event->monsters[hovered].used ? &event->monsters[hovered] : nullptr;
    }
    void Text(int x,int y,int width,const wchar_t* text,bool gold=false)
    {
        g_pRenderText->SetFont(gold?g_hFontBold:g_hFont);g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(gold?255:225,gold?215:225,gold?75:225,255);
        g_pRenderText->RenderText(x,y,text,width,0,RT3_SORT_CENTER);
    }
    void Cycle(int direction)
    {
        Selected();
        for(size_t step=1;step<=state.events.size();++step)
        {
            int index=(selected+direction*static_cast<int>(step)+static_cast<int>(state.events.size())*2)%state.events.size();
            if(state.events[index].used) {selected=index;scroll=0;hovered=-1;break;}
        }
    }
    void Background(int x,int y,int width,int height)
    {
        EnableAlphaTest();Paris::Color(0,0,0,.84f);
        RenderColor(x,y,width,height);EndRenderColor();Paris::Color(1,1,1,1);
    }
    void RowsText(const Event& event)
    {
        int row=0;
        for(size_t i=0;i<event.monsters.size();++i)
        {
            const auto& monster=event.monsters[i];if(!monster.used)continue;
            const int visible=row++-scroll;if(visible<0||visible>=Rows)continue;
            const int top=Y+33+visible*RowHeight;
            if(hovered==i)
            {
                Paris::Color(.32f,.10f,.02f,.72f);RenderColor(X+3,top,Width-6,RowHeight);EndRenderColor();
            }
            Paris::Clip(X+4,top,Width-39,RowHeight);
            g_pRenderText->SetFont(g_hFont);g_pRenderText->SetBgColor(0);
            g_pRenderText->SetTextColor(235,220,180,255);
            g_pRenderText->RenderText(X+5,top,monster.name.data());Paris::ResetClip();
            wchar_t count[32]{};swprintf_s(count,L"%d / %d",monster.alive,monster.total);
            Text(X+Width-37,top,34,count);
        }
    }
    void Header(const Event& event)
    {
        Text(X+14,Y+5,Width-42,event.name.data(),true);
        Text(X+2,Y+5,10,L"<",true);Text(X+Width-28,Y+5,10,L">",true);Text(X+Width-13,Y+5,10,L"X",true);
        const auto now=SDL_GetTicks();const auto seconds=event.deadline>now?(event.deadline-now)/1000:0;
        wchar_t time[48]{};swprintf_s(time,L"Tiempo: %llu:%02llu",seconds/60,seconds%60);
        Text(X+4,Y+21,Width-39,time);
        int page=0,count=0;for(size_t i=0;i<state.events.size();++i)if(state.events[i].used){++count;if(i==selected)page=count;}
        swprintf_s(time,L"%d / %d",page,count);Text(X+Width-34,Y+21,30,time);
    }
}
void Load()
{
    on=Bitmaps.LoadImage(L"Data\\Interface\\ascend_on.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
    off=Bitmaps.LoadImage(L"Data\\Interface\\ascend_off.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
}
void Unload()
{
    for(auto texture:{on,off})if(texture!=BITMAP_UNKNOWN)DeleteBitmap(texture);
    on=off=BITMAP_UNKNOWN;Reset();
}
void Close() {open=false;hovered=-1;previewReady=false;::Render::UI::MonsterPreview::Clear();}
void Reset() {Close();state.Clear();selected=-1;scroll=0;}
bool IsOpen() {return open;}
bool ConsumeMetadata(std::wstring_view text) {return state.Apply(text,SDL_GetTicks());}
bool Mouse()
{
    hovered=-1;previewReady=false;
    if(!Selected()) {Close();return false;}
    const bool toggle=SEASON3B::CheckMouseIn(ButtonX,ButtonY,ButtonWidth,ButtonHeight);
    const bool panel=open&&SEASON3B::CheckMouseIn(X,Y,Width,Height);
    if(!toggle&&!panel) return false;
    if(SEASON3B::IsPress(VK_LBUTTON))
    {
        if(toggle) {open=!open;scroll=0;}
        else if(SEASON3B::CheckMouseIn(X+Width-14,Y,14,20)) Close();
        else if(SEASON3B::CheckMouseIn(X,Y,14,20))Cycle(-1);
        else if(SEASON3B::CheckMouseIn(X+Width-29,Y,14,20))Cycle(1);
        OverlayInput::Capture();
    }
    const auto* event=Selected();
    if(panel&&event)
    {
        int count=0;for(const auto& monster:event->monsters)if(monster.used)++count;
        if(MouseWheel){scroll=std::clamp(scroll-(MouseWheel>0?1:-1),0,std::max(0,count-Rows));MouseWheel=0;}
        int row=0;
        for(size_t i=0;i<event->monsters.size();++i)
        {
            if(!event->monsters[i].used)continue;const int visible=row++-scroll;
            if(visible<0||visible>=Rows)continue;
            if(SEASON3B::CheckMouseIn(X,Y+33+visible*RowHeight,Width,RowHeight))
            {hovered=static_cast<int>(i);previewReady=::Render::UI::MonsterPreview::Prepare(event->monsters[i].id);break;}
        }
    }
    Paris::ConsumeMouse();return true;
}
void Render()
{
    EnableAlphaTest();Paris::Color(1,1,1,1);
    const auto* event=Selected();
    if(!event) {Close();return;}
    const GLuint texture=on;
    if(texture!=BITMAP_UNKNOWN)RenderBitmap(texture,ButtonX,ButtonY,ButtonWidth,ButtonHeight);
    else Theme::Bevel(ButtonX,ButtonY,ButtonWidth,ButtonHeight,L"ACTIVE INVASION",event!=nullptr);
    if(!open)return;
    Background(X,Y,Width,Height);
    if(!event) {Text(X+5,Y+25,Width-10,L"Sin invasiones activas");return;}
    Header(*event);RowsText(*event);
    const auto* monster=Hovered();if(!monster)return;
    Background(PreviewX,Y,PreviewWidth,PreviewHeight);
    Text(PreviewX+5,Y+5,PreviewWidth-10,monster->name.data(),true);
    wchar_t count[32]{};swprintf_s(count,L"%d / %d",monster->alive,monster->total);
    Text(PreviewX+5,Y+PreviewHeight-14,PreviewWidth-10,count);
    if(!previewReady)Text(PreviewX+5,Y+70,PreviewWidth-10,L"Vista no disponible");
}
void Render3D()
{
    if(open&&previewReady&&Hovered())::Render::UI::MonsterPreview::Draw(PreviewX+5,Y+20,PreviewWidth-10,PreviewHeight-40);
}
}
