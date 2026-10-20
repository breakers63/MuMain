#include "stdafx.h"
#include "ParisBattlePassWindow.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Dialogs/ParisCustomMenuWindow.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"

#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "Engine/Object/ZzzInventory.h"
#include "Network/Server/WSclient.h"
#include "Network/Paris/ParisBattlePassProtocol.h"
#include "GameLogic/Social/BattlePassRewardState.h"
#include "UI/NewUI/HUD/BattlePassScroll.h"
#include "UI/NewUI/Widgets/ParisTintOverlay.h"
#include <array>
#include <vector>
#include <string>
#include <span>
#include <cstring>
#include <algorithm>
#include <ctime>

namespace UI::BattlePass
{
    struct Reward : Network::Paris::BattlePass::Reward
    {
        ITEM* Item=nullptr;
    };
    UI::Theme::ParisFrameAssets Art;
    GLuint Panel = BITMAP_UNKNOWN;
    std::array<GLuint, 11> Textures{};
    std::vector<int> ViewLevels;
    bool DraggingScroll = false;
    bool PremiumDialog = false;
    bool PremiumTermsKnown = false, PremiumRequirementReady = false;
    int PremiumItemLevel = 0;
    std::int64_t PremiumExpiresUtc = 0;
    constexpr int DialogWidth = 220, DialogHeight = 318, DialogX = (640-DialogWidth)/2, DialogY = (429-DialogHeight)/2;
    bool Open = false, Pending = false, Active = false, Premium = false, PremiumFree = false;
    int Level = 0, MaxLevel = 0, Points = 0, RequiredPoints = 0, Page = 0, PremiumItem = 0, PremiumCount = 0;
    DWORD RequestedAt = 0;
    BYTE RequestedOperation = 0;
    unsigned long long ConfigurationToken = 0;
    std::array<BYTE,16> Season{};
    std::wstring Name, Message;
    std::vector<Reward> Rewards;
    constexpr int X = 50, Y = 40, Width = 540, Height = 380, PerPage = 6;
    int MaximumScroll() { return Scroll::MaximumOffset(static_cast<int>(ViewLevels.size()), PerPage); }
    int SlotLevel(int col) { return Page + col < static_cast<int>(ViewLevels.size()) ? ViewLevels[Page + col] : 0; }
    int U16(const BYTE* p) { return p[0] | (p[1] << 8); }
    template<class T> T Number(const BYTE* p) { T value; std::memcpy(&value,p,sizeof(value)); return value; }
    void Clear()
    {
        if (g_pNewItemMng) for (auto& row : Rewards) if (row.Item) g_pNewItemMng->DeleteItem(row.Item);
        Rewards.clear();
        ViewLevels.clear();
    }
    void Close()
    {
        if (!Open) return;
        Open = Pending = PremiumDialog = DraggingScroll = false;
        Clear();
    }
    void Load()
    {
        Art.Load();
        constexpr const wchar_t* names[]{L"background",L"free_line",L"premium_line",L"icon_locked",L"icon_checked",L"zen_coins",L"w_coins",L"bar_bg",L"bar",L"buff",L"ruud_coins"};
        Textures.fill(BITMAP_UNKNOWN);
        for(int i=0;i<11;++i){wchar_t file[160]{};swprintf_s(file,L"Data\\Custom\\Interface\\BattlePass\\%ls.tga",names[i]);Textures[i]=Bitmaps.LoadImage(file,GL_LINEAR,GL_CLAMP_TO_EDGE);}

        if (Panel == BITMAP_UNKNOWN) Panel = Bitmaps.LoadImage(L"Data\\Custom\\Interface\\BattlePass\\panel.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
    }
    void Unload()
    {
        for(auto image:Textures)if(image!=BITMAP_UNKNOWN)DeleteBitmap(image);Textures.fill(BITMAP_UNKNOWN);
        Clear(); Art.Unload(); if (Panel != BITMAP_UNKNOWN) DeleteBitmap(Panel); Panel = BITMAP_UNKNOWN; Open = Pending = false;
    }
    void Request(BYTE operation = 0, int level = 0, bool premium = false)
    {
        if (!SocketClient || !SocketClient->IsConnected() || Pending) return;
        BYTE packet[33] = {0xC1,33,0xFA,0xB0,1,operation};
        std::copy(Season.begin(),Season.end(),packet+6);
        packet[22] = static_cast<BYTE>(level); packet[23] = static_cast<BYTE>(level >> 8); packet[24] = premium;
        std::memcpy(packet+25,&ConfigurationToken,8);
        RequestedOperation=operation;SocketClient->Send(packet,sizeof(packet)); Pending = true; RequestedAt = timeGetTime();
    }
    void Toggle()
    {
        const bool opening = !Open;
        
        DraggingScroll=false;PremiumDialog=false;
        Open = opening; Pending = Active = Premium = false; Page = Level = MaxLevel = 0;
        PremiumCount=0;PremiumItem=-1;PremiumItemLevel=0;PremiumTermsKnown=PremiumRequirementReady=false;PremiumExpiresUtc=0;
        Season.fill(0); ConfigurationToken=0; Name.clear(); Message.clear(); Clear();
        if (Open) Request();
    }
    bool Receive(const BYTE* packet,int size)
    {
        if(!Open || !Pending || !packet || size<0)return false;
        auto received=Network::Paris::BattlePass::Decode(std::span<const BYTE>(packet,static_cast<std::size_t>(size)));
        if(!received)return false;
        Clear();
        for(auto& row:received->Rewards)
        {
            Reward reward;static_cast<Network::Paris::BattlePass::Reward&>(reward)=std::move(row);
            if(g_pNewItemMng&&reward.Type==6&&!reward.Data.empty())
                reward.Item=g_pNewItemMng->CreateItem(std::span<const BYTE>(reward.Data.data(),reward.Data.size()));
            Rewards.push_back(std::move(reward));
        }
        Season=received->Season;ConfigurationToken=received->ConfigurationToken;
        Level=received->Level;MaxLevel=received->MaxLevel;Points=received->Points;RequiredPoints=received->RequiredPoints;
        PremiumCount=received->PremiumCount;PremiumItem=received->PremiumItem;
        PremiumItemLevel=received->PremiumItemLevel;PremiumExpiresUtc=received->PremiumExpiresUtc;
        PremiumTermsKnown=received->PremiumTermsKnown;PremiumRequirementReady=received->PremiumRequirementReady;
        Active=received->Active;Premium=received->Premium;PremiumFree=received->PremiumFree;
        Name=std::move(received->Name);Message=std::move(received->Message);
        for(const auto& reward:Rewards)ViewLevels.push_back(reward.Level);
        std::sort(ViewLevels.begin(),ViewLevels.end());
        ViewLevels.erase(std::unique(ViewLevels.begin(),ViewLevels.end()),ViewLevels.end());
        Page=(std::min)(Page,MaximumScroll());Pending=false;
        if(RequestedOperation==2&&Premium)PremiumDialog=false;
        return true;
    }
    Reward* Find(int level, bool premium)
    {
        auto it=std::find_if(Rewards.begin(),Rewards.end(),[&](const Reward& r){return r.Level==level && r.Premium==premium;});
        return it==Rewards.end()?nullptr:&*it;
    }
    bool Unlocked(const Reward& r)
    {
        return GameLogic::Social::BattlePassRewardState::Unlocked(
            Active, Level, r.Level, r.Premium, Premium, r.Claimed);
    }
    bool Claimable(const Reward& r) { return Unlocked(r) && !Pending && !r.Claimed; }
    int SlotX(int column) { return X+31+column*84; }
    int SlotY(bool premium) { return Y+(premium?250:156); }
    void Sprite(int index,int x,int y,int width,int height,float u=140.f/256.f,float v=140.f/256.f)
    {
        if(Textures[index]==BITMAP_UNKNOWN)return;
        EnableAlphaTest();UI::Paris::Color(1,1,1,1);RenderBitmap(Textures[index],x,y,width,height,0,0,u,v);
    }
    void Label(int x,int y,int width,const wchar_t* text,bool gold=false,int alignment=RT3_SORT_CENTER)
    {
        g_pRenderText->SetFont(g_hFont);g_pRenderText->SetBgColor(0,0,0,0);
        g_pRenderText->SetTextColor(255,gold?216:255,gold?100:255,255);
        g_pRenderText->RenderText(x,y,text,width,0,alignment);
    }
    void Button(int x,int width,const wchar_t* caption,bool gold)
    {
        const int y=Y+Height-32;
        const bool hover=SEASON3B::CheckMouseIn(x,y,width,19);
        for(int row=0;row<19;++row){const float shade=(hover?1.3f:1.f)-row/30.f;UI::Character::ParisStyle::Solid(x,y+row,width,1,gold?.78f*shade:.65f*shade,gold?.57f*shade:.035f,gold?.04f:.025f);}
        Label(x,y+5,width,caption);
    }
    void SetScrollFromMouse()
    {
        const int trackWidth=Width-44;
        const int thumb=Scroll::ThumbWidth(trackWidth,static_cast<int>(ViewLevels.size()),PerPage);
        Page=Scroll::OffsetAt(MouseX-(X+22),trackWidth,thumb,MaximumScroll());
    }
    bool Mouse()
    {
        if(!Open)return false;
        if (!DraggingScroll && !SEASON3B::CheckMouseIn(X, Y, Width, Height)
            && (!PremiumDialog || !SEASON3B::CheckMouseIn(DialogX, DialogY, DialogWidth, DialogHeight))) return false;
        const bool pressed=SEASON3B::IsPress(VK_LBUTTON);
        if(PremiumDialog)
        {
            if(pressed&&(!Pending||RequestedOperation!=2)&&SEASON3B::CheckMouseIn(DialogX+116,DialogY+260,72,22))PremiumDialog=false;
            else if(pressed&&SEASON3B::CheckMouseIn(DialogX+32,DialogY+260,72,22)&&!Pending)
            {if(Active&&!Premium)Request(2);else PremiumDialog=false;}
            MouseLButton=MouseLButtonPush=MouseLButtonPop=false;
            MouseWheel=0;
            return true;
        }
        if(!(GetAsyncKeyState(VK_LBUTTON)&0x8000))DraggingScroll=false;
        if(pressed&&SEASON3B::CheckMouseIn(X+22,Y+Height-48,Width-44,10))DraggingScroll=true;
        if(DraggingScroll)SetScrollFromMouse();
        if(MouseWheel!=0&&SEASON3B::CheckMouseIn(X,Y,Width,Height)){Page=(std::clamp)(Page+(MouseWheel>0?-1:1),0,MaximumScroll());MouseWheel=0;}
        if(pressed&&!DraggingScroll)
        {
            if(SEASON3B::CheckMouseIn(X+Width-92,Y+Height-32,70,19)){Close();}
            else if(SEASON3B::CheckMouseIn(X+22,Y+Height-32,120,19))PremiumDialog=true;
            else if(Active)for(int col=0;col<PerPage;++col)for(bool premium:{false,true})
                if(auto* row=Find(SlotLevel(col),premium);row&&Claimable(*row)&&SEASON3B::CheckMouseIn(SlotX(col),SlotY(premium),70,70))Request(1,row->Level,row->Premium);
        }
        MouseLButton=MouseLButtonPush=MouseLButtonPop=false;
        return true;
    }
    void RenderProgress()
    {
        wchar_t text[80]{};swprintf_s(text,L"Level: %d / %d",Level,MaxLevel);Label(X+20,Y+80,180,text,false,RT3_SORT_LEFT);
        Sprite(7,X+20,Y+91,180,12,271.f/512.f,18.f/32.f);
        const float progress=RequiredPoints>0?(std::clamp)(static_cast<float>(Points)/RequiredPoints,0.f,1.f):0.f;
        if(progress>0&&Textures[8]!=BITMAP_UNKNOWN)RenderBitmap(Textures[8],X+23,Y+95,174*progress,5,0,0,261.f/512.f*progress,8.f/8.f);
        swprintf_s(text,L"%d / %d Points",Points,RequiredPoints);Label(X+20,Y+94,180,text);
    }
    void RenderReward(int col,bool premium)
    {
        const int level=SlotLevel(col);if(level==0)return;
        const int x=SlotX(col),y=SlotY(premium);
        const bool hover=!PremiumDialog&&SEASON3B::CheckMouseIn(x,y,70,70);
        UI::Character::ParisStyle::Solid(x,y,70,70,hover?.12f:.035f,hover?.10f:.035f,hover?.05f:.035f);
        if(hover){UI::Character::ParisStyle::Solid(x,y,70,1,.95f,.76f,.25f);UI::Character::ParisStyle::Solid(x,y+69,70,1,.95f,.76f,.25f);UI::Character::ParisStyle::Solid(x,y,1,70,.95f,.76f,.25f);UI::Character::ParisStyle::Solid(x+69,y,1,70,.95f,.76f,.25f);}
        if(!premium){wchar_t caption[32]{};swprintf_s(caption,L"Level %d",level);Label(x,y-14,70,caption);}
        auto* reward=Find(level,premium);if(!reward)return;
        if(reward->Claimed)Sprite(4,x+57,y,13,13);
        else if(!Unlocked(*reward))Sprite(3,x+57,y,13,13);
        if(reward->Type!=6)Sprite(reward->Type==2?5:reward->Type==1?9:reward->Type==5?10:6,x+18,y+18,35,35);
        if(reward->Amount>1&&reward->Type==6){wchar_t amount[32]{};swprintf_s(amount,L"X%lld",reward->Amount);Label(x+4,y+5,45,amount,true,RT3_SORT_LEFT);}
    }
    void RenderScroll()
    {
        const int trackWidth=Width-44,y=Y+Height-44;
        const int thumb=Scroll::ThumbWidth(trackWidth,static_cast<int>(ViewLevels.size()),PerPage);
        const int offset=MaximumScroll()>0?Page*(trackWidth-thumb)/MaximumScroll():0;
        UI::Character::ParisStyle::Solid(X+22,y,trackWidth,5,.015f,.015f,.015f);
        UI::Character::ParisStyle::Solid(X+22+offset,y,thumb,5,.35f,.35f,.35f);
    }
    void RenderTooltips()
    {
        for(int col=0;col<PerPage;++col)for(bool premium:{false,true})
            if(auto* reward=Find(SlotLevel(col),premium);reward&&SEASON3B::CheckMouseIn(SlotX(col),SlotY(premium),70,70))
            {
                if(reward->Item)RenderMarketItemInfo((std::clamp)(MouseX+12,12,400),(std::clamp)(MouseY+12,30,240),reward->Item);
                else UI::Theme::HoverTip(SlotX(col),SlotY(premium),70,70,reward->Name.c_str());
            }
        if(!Premium&&SEASON3B::CheckMouseIn(X+22,Y+Height-32,120,19))
        {
            std::wstring tip=PremiumFree?L"Premium libre, sin costo":L"Requiere "+std::to_wstring(PremiumCount)+L" objeto(s): ";
            if(!PremiumFree&&PremiumItem>=0&&PremiumItem<MAX_ITEM)tip+=ItemAttribute[PremiumItem].Name;
            UI::Theme::HoverTip(X+22,Y+Height-32,120,19,tip.c_str());
        }
    }
    void DialogButton(int x,const wchar_t* caption)
    {
        for(int row=0;row<22;++row){const float shade=.52f-row*.008f;UI::Character::ParisStyle::Solid(x,DialogY+260+row,72,1,shade,shade,.9f*shade);}
        g_pRenderText->SetFont(g_hFontBold);g_pRenderText->SetBgColor(0,0,0,0);g_pRenderText->SetTextColor(15,15,15,255);
        g_pRenderText->RenderText(x,DialogY+266,caption,72,0,RT3_SORT_CENTER);
    }
    void RenderPremiumDialog()
    {
        if(!PremiumDialog)return;
        UI::Theme::TintOverlay(X,Y,Width,Height,0.f,0.f,0.f,.65f);
        UI::Character::ParisStyle::Solid(DialogX,DialogY,DialogWidth,DialogHeight,.7f,.6f,.35f);
        UI::Character::ParisStyle::Solid(DialogX+1,DialogY+1,DialogWidth-2,DialogHeight-2,.06f,.06f,.06f);
        Art.Stone(DialogX+3,DialogY+3,DialogWidth-6,DialogHeight-6);
        Label(DialogX+15,DialogY+12,DialogWidth-30,L"Premium Buy");
        UI::Character::ParisStyle::Solid(DialogX+26,DialogY+38,168,17,.17f,.13f,.025f);
        Label(DialogX+26,DialogY+43,168,L"Requirements",true);
        UI::Character::ParisStyle::Solid(DialogX+26,DialogY+57,168,72,.025f,.025f,.015f);
        wchar_t requirement[128]{};
        if(Pending&&RequestedOperation==0)wcscpy_s(requirement,L"Consultando requisitos...");
        else if(PremiumFree||PremiumCount<=0)wcscpy_s(requirement,L"Sin item de requerimiento (0)");
        else if(PremiumItem>=0&&PremiumItem<MAX_ITEM)
        {
            if(PremiumItemLevel>0)swprintf_s(requirement,L"%ls +%d x%d",ItemAttribute[PremiumItem].Name,PremiumItemLevel,PremiumCount);
            else swprintf_s(requirement,L"%ls x%d",ItemAttribute[PremiumItem].Name,PremiumCount);
        }
        else wcscpy_s(requirement,L"Requisito no publicado por el servidor");
        g_pRenderText->SetFont(g_hFont);g_pRenderText->SetBgColor(0,0,0,0);g_pRenderText->SetTextColor(255,75,65,255);
        g_pRenderText->RenderText(DialogX+28,DialogY+117,requirement,164,0,RT3_SORT_CENTER);
        UI::Character::ParisStyle::Solid(DialogX+26,DialogY+143,168,1,.62f,.48f,.08f);
        g_pRenderText->SetFont(g_hFont);g_pRenderText->SetBgColor(0,0,0,0);g_pRenderText->SetTextColor(70,255,220,255);
        g_pRenderText->RenderText(DialogX+20,DialogY+151,Pending?(RequestedOperation==2?L"Activando...":L"Consultando servidor..."):Premium?L"Premium ya activo":!Active?L"Sin temporada activa":PremiumTermsKnown?(PremiumRequirementReady?L"Ready to Activate!":L"Falta el requisito"):L"Confirmar activacion",DialogWidth-40,0,RT3_SORT_CENTER);
        Label(DialogX+26,DialogY+172,70,L"Expires:",true,RT3_SORT_LEFT);
        wchar_t date[48]{};__time64_t stamp=PremiumExpiresUtc;tm utc{};
        if(stamp>0&&_gmtime64_s(&utc,&stamp)==0)swprintf_s(date,L"%02d/%02d/%04d",utc.tm_mday,utc.tm_mon+1,utc.tm_year+1900);
        else wcscpy_s(date,L"No publicado");
        Label(DialogX+98,DialogY+172,96,date,false,RT3_SORT_RIGHT);
        DialogButton(DialogX+32,L"OK");DialogButton(DialogX+116,L"CANCEL");
    }
    void Render()
    {
        if(!Open)return;
        if(Pending&&timeGetTime()-RequestedAt>6000){Pending=false;if(RequestedOperation==0)Active=false;Message=L"Sin respuesta. Cierra y vuelve a abrir.";}
        EnableAlphaTest();UI::Paris::Color(1,1,1,1);
        if(Panel!=BITMAP_UNKNOWN)RenderBitmap(Panel,X,Y,Width,Height,0,0,810.f/1024.f,570.f/1024.f);
        else Art.Frame(X,Y,Width,Height);
        RenderProgress();
        Sprite(1,X+5,SlotY(false),14,70,30.f/32.f,140.f/256.f);
        Sprite(2,X+5,SlotY(true),14,70,30.f/32.f,140.f/256.f);
        if(Active)for(int col=0;col<PerPage;++col)for(bool premium:{false,true})RenderReward(col,premium);
        else Label(X+25,Y+210,Width-50,Pending?L"Consultando temporada...":L"No hay una temporada activa.");
        RenderScroll();
        Button(X+22,120,L"Activar Premium",true);
        Button(X+Width-92,70,L"Close",false);
        if(!Message.empty())Label(X+153,Y+Height-28,Width-240,Message.c_str(),true);
        if(Active&&!PremiumDialog)RenderTooltips();
        RenderPremiumDialog();
    }
    void Render3D()
    {
        if(!Open||!Active)return;
        if(PremiumDialog)
        {
            if(!PremiumFree&&PremiumItem>=0&&PremiumItem<MAX_ITEM)
                RenderMarketPreviewItem3D(DialogX+95.f,DialogY+76.f,30.f,30.f,PremiumItem,PremiumItemLevel,0,0,.58f);
            return;
        }
        for(int col=0;col<PerPage;++col)for(bool premium:{false,true})
        {
            auto* reward=Find(SlotLevel(col),premium);if(!reward||!reward->Item)continue;
            auto* item=reward->Item;if(item->Type<0||item->Type>=MAX_ITEM)continue;
            const float width=(std::min)(34.f,18.f*(std::max)(1,int(ItemAttribute[item->Type].Width)));
            const float height=(std::min)(34.f,18.f*(std::max)(1,int(ItemAttribute[item->Type].Height)));
            RenderMarketPreviewItem3D(SlotX(col)+(70.f-width)/2,SlotY(premium)+(70.f-height)/2,width,height,item->Type,item->Level,item->ExcellentFlags,item->AncientDiscriminator,.58f);
        }
    }
}

using namespace SEASON3B;
namespace Pass = UI::BattlePass;
CParisBattlePassWindow::~CParisBattlePassWindow() { Release(); }
bool CParisBattlePassWindow::Create(CNewUIManager* manager)
{
    if (!manager) return false;
    constexpr DWORD PassInterface = 0x50425053;
    m_manager = manager;
    manager->AddUIObj(PassInterface, this);
    g_pNewUI3DRenderMng->Add3DRenderObj(this);
    Pass::Load();
    Show(false);
    return true;
}
void CParisBattlePassWindow::Release()
{
    if (!m_manager) return;
    g_pNewUI3DRenderMng->Remove3DRenderObj(this);
    Pass::Unload();
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisBattlePassWindow::ToggleWindow()
{
    CParisCustomMenuWindow::CloseAllOverlays(this);
    Pass::Toggle();
    Show(Pass::Open);
}
void CParisBattlePassWindow::CloseWindow() { Pass::Close(); Show(false); }
bool CParisBattlePassWindow::ReceivePacket(const BYTE* packet, int size) { return Pass::Receive(packet, size); }
bool CParisBattlePassWindow::UpdateMouseEvent() { return !Pass::Mouse(); }
bool CParisBattlePassWindow::UpdateKeyEvent()
{
    if (!IsVisible() || !IsPress(VK_ESCAPE)) return true;
    CloseWindow(); return false;
}
bool CParisBattlePassWindow::Update()
{
    if (SceneFlag != MAIN_SCENE || !SocketClient || !SocketClient->IsConnected() ||
        (Pass::Open && !CNewUIObj::IsVisible())) CloseWindow();
    Show(Pass::Open); return true;
}
bool CParisBattlePassWindow::Render() { Pass::Render(); DisableAlphaBlend(); return true; }
void CParisBattlePassWindow::Render3D() { Pass::Render3D(); }
bool CParisBattlePassWindow::IsVisible() const { return Pass::Open && CNewUIObj::IsVisible(); }
CNewUIObj* CParisBattlePassWindow::GetLayoutOwner() const { return const_cast<CParisBattlePassWindow*>(this); }
float CParisBattlePassWindow::GetLayerDepth() { return 8.5f; }
float CParisBattlePassWindow::GetKeyEventOrder() { return 2.f; }

