#include "stdafx.h"
#include "UI/NewUI/Character/ParisPanelReference.h"
#include "UI/NewUI/Character/ParisVipWindow.h"
#include "UI/NewUI/Character/ParisControls51.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/NewUIAdvancedStatsWindow.h"
#include "UI/NewUI/Inventory/ParisJewelBankWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "Network/Paris/ParisRequests.h"
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
namespace UI::Character::VipPanel
{
    UI::Theme::ParisControls51 Mockup;
    UI::Theme::ParisPanelReference Reference;
    GLuint CloseTexture = BITMAP_UNKNOWN;
    bool Open = false, CanBuy = false, Pending = false, Dragging = false;
    unsigned long long Quote = 0;
    int CurrentTier = 0, FirstLine = 0, DragOffset = 0, SelectedPlan = 1;
    DWORD RequestedAt = 0;
    std::vector<std::wstring> Lines, Benefits, Costs;
    std::vector<std::wstring> BenefitLabels, BenefitValues;
    std::wstring PlanName, CurrentName=L"Cuenta normal", Duration, Result;
    int RemainingDays=0;
    bool PendingPurchase=false;
    constexpr int Width = 190, Height = 429, LineTop = 224, LineStep = 20;
    constexpr int MinimumLineStep = 11, CostLineStep = 11, CostBottomGap = 22;
    constexpr int TabTop = 81, CloseTop = 7, PurchaseTop = 370;

    int X()
    {
        return g_pCharacterInfoWindow ? (std::max)(4, static_cast<int>(g_pCharacterInfoWindow->GetPos().x) - Width + 10) : 260;
    }
    int Y() { return g_pCharacterInfoWindow ? g_pCharacterInfoWindow->GetPos().y : 0; }
    int ExtraHeight()
    {
        const int costRows = (std::max)(1, static_cast<int>(Costs.size()));
        const int needed = static_cast<int>(Benefits.size()) * MinimumLineStep;
        const int available = PurchaseTop - CostBottomGap - costRows * CostLineStep - 20 - LineTop;
        return (std::max)(0, needed - available);
    }
    int CostTop() { return PurchaseTop + ExtraHeight() - CostBottomGap - (std::max)(1, static_cast<int>(Costs.size())) * CostLineStep; }
    int CostHeadingTop() { return CostTop() - 17; }
    int BenefitStep()
    {
        const int available = CostHeadingTop() - 3 - LineTop;
        return (std::min)(LineStep, available / (std::max)(1, static_cast<int>(Benefits.size())));
    }
    int SelectedRemainingDays() { return CurrentTier == SelectedPlan ? RemainingDays : 0; }
    const wchar_t* AccountLabel()
    {
        constexpr const wchar_t* Accounts[] = {L"Normal", L"VIP 1", L"VIP 2", L"VIP 3"};
        return Lines.empty() ? L"..." : Accounts[CurrentTier];
    }

    void CacheBenefitRows()
    {
        BenefitLabels.clear();
        BenefitValues.clear();
        for (const auto& line : Benefits)
        {
            const auto colon = line.find(L':');
            const bool split = !line.empty() && line[0] != L'>'
                && colon != std::wstring::npos && line.find(L'%', colon) != std::wstring::npos;
            BenefitLabels.push_back(split ? line.substr(0, colon) : line);
            BenefitValues.push_back(split ? line.substr(colon + 1) : L"");
        }
    }

    void ParseDetails()
    {
        Benefits.clear(); Costs.clear(); Duration.clear(); Result.clear();
        PlanName=Lines.empty()?L"":Lines[0]; CurrentName=L"Cuenta normal";
        int section=0;
        for(std::size_t i=1;i<Lines.size();++i)
        {
            const auto& line=Lines[i];
            if(line.rfind(L"Actual: ",0)==0) {CurrentName=line.substr(8);continue;}
            if(line.rfind(L"Te quedan ",0)==0 || line==L"VIP permanente") continue;
            if(line.rfind(L"Duraci",0)==0) {auto colon=line.find(L':');if(colon!=std::wstring::npos)Duration=line.substr(colon+2);continue;}
            if(line==L">Costo") {section=1;continue;}
            if(line==L">Beneficios") {section=2;continue;}
            if(line==L">Resultado") {section=3;continue;}
            if(line.rfind(L"!Compra VIP",0)==0) {Result=line.substr(1);continue;}
            if(section==1) Costs.push_back(line);
            else if(section==2 && line!=L"Suma los días restantes" && line!=L"Suma los dias restantes") Benefits.push_back(line);
            else if(section==3) Result=line;
        }
        CacheBenefitRows();
    }

    void Request(bool buy=false)
    {
        if(Pending) return;
        if(buy && (!CanBuy || Quote==0))
        {
            Result=Quote==0?L"Actualizando los datos VIP...":L"Revisa los requisitos y el costo VIP.";
            Request(false);return;
        }
        if (!Network::Paris::RequestVip(SelectedPlan, buy, Quote))
        {
            Result = L"Sin conexion con el servidor.";
            return;
        }
        RequestedAt = timeGetTime();
        Pending = true;
        PendingPurchase = buy;

        if(buy) Result=L"Procesando compra...";
    }

    void Toggle()
    {
        const bool opening = !Open;
        g_pNewUISystem->HideAll();
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_PARIS_JEWEL_BANK);
        if (g_pAdvancedStatsWindow) g_pAdvancedStatsWindow->CloseWindow(false);
        Open = opening;
        Pending = CanBuy = Dragging = false;
        FirstLine = CurrentTier = 0; SelectedPlan = 1;
        Quote = 0;
        Lines.clear(); Benefits.clear(); Costs.clear(); PlanName.clear(); Duration.clear(); Result.clear(); CurrentName=L"Cuenta normal";RemainingDays=0;
        if (Open && g_pNewUISystem)
        {
            g_pNewUISystem->Show(SEASON3B::INTERFACE_CHARACTER);
            Request();
        }
    }

    bool Receive(const BYTE* packet,int size)
    {

        if(!packet||!Open||!Pending||size<33||size>16032||packet[0]!=0xC2||((packet[1]<<8)|packet[2])!=size
            ||packet[3]!=0xFA||packet[4]!=0xB3||packet[5]!=1||packet[6]>3||packet[7]<1||packet[7]>3||packet[8]>1)return false;
        if(packet[7]!=SelectedPlan)return false;
        const int count=packet[24]|(packet[25]<<8);if(count<1||count>128)return false;
        int days;std::memcpy(&days,packet+12,4);if(days<0)return false;
        unsigned long long quote;std::memcpy(&quote,packet+16,8);if(!quote)return false;
        std::vector<std::wstring> received;int offset=32;
        for(int i=0;i<count;++i)
        {
            int end=offset;while(end<size&&packet[end])++end;
            if(end==size||end-offset>1024)return false;
            int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(packet+offset),end-offset,nullptr,0);
            if(length<1||length>512)return false;
            std::wstring line(length,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(packet+offset),end-offset,line.data(),length);
            received.push_back(std::move(line));offset=end+1;
        }
        if(offset!=size)return false;
        CurrentTier=packet[6];RemainingDays=days;CanBuy=packet[8]!=0;Quote=quote;Lines=std::move(received);Pending=false;PendingPurchase=false;ParseDetails();

        FirstLine=0;return true;
    }

    bool Mouse()
    {
        if(!Open) return false;
        const int x=X(),y=Y();
        if (!SEASON3B::CheckMouseIn(x, y, Width, Height + ExtraHeight())) return false;
        if(SEASON3B::IsPress(VK_LBUTTON))
        {
            if(SEASON3B::CheckMouseIn(x+169,y+CloseTop,13,12)
                || SEASON3B::CheckMouseIn(x+12,y+382+ExtraHeight(),24,24)) Open=false;
            else if(SEASON3B::CheckMouseIn(x+35,y+PurchaseTop+ExtraHeight(),120,28)) Request(true);
            else if(!Pending && SEASON3B::CheckMouseIn(x+12,y+TabTop,166,33))
            {
                SelectedPlan=(std::clamp)((MouseX-x-12)/56+1,1,3);
                CanBuy=false;Quote=0;FirstLine=0;Result.clear();
                PlanName.clear();Duration.clear();Benefits.clear();Costs.clear();
                Request();
            }
        }
        UI::Paris::ConsumeMouse();
        return true;
    }
    bool UpdateMouse() {return Mouse();}

    void Text(int x,int y,int width,const wchar_t* text,bool gold=false,bool bold=false,
        int align=RT3_SORT_LEFT,bool green=false,bool large=false)
    {
        if (!text || !*text) return;
        g_pRenderText->SetFont(bold||large?g_hFontBold:g_hFont);
        g_pRenderText->SetBgColor(0,0,0,0);
        g_pRenderText->SetTextColor(green?80:255,green?255:gold?213:255,green?80:gold?117:255,255);
        g_pRenderText->RenderText(x,y,text,width,0,align);
    }
    void DrawHeader(int x,int y)
    {
        Text(x+10,y+49,Width-20,L"VIP SYSTEM",true,true,RT3_SORT_CENTER,false,true);
        Text(x+22,y+65,80,L"Estado Cuenta:");
        const auto account=AccountLabel();
        Text(x+103,y+65,71,account,false,true,RT3_SORT_LEFT,true);
        UI::Theme::HoverTip(x+103,y+64,71,13,CurrentName.c_str());
        for(int tier=1;tier<=3;++tier)
        {
            const int bx=x+12+(tier-1)*56; const bool selected=tier==SelectedPlan;
            if(Mockup.Ready()) Mockup.Tab(tier,selected,bx,y+TabTop,54,33);
            else UI::Theme::Bevel(bx,y+TabTop,54,33,L"",selected);
            wchar_t label[12];swprintf_s(label,L"VIP %d",tier);
            if(Mockup.Ready()) Text(bx,y+TabTop+23,54,label,selected,true,RT3_SORT_CENTER);
        }
    }

    void DrawBenefitRow(int x,int y,int index)
    {
        const auto& line = Benefits[index];
        const bool heading=!line.empty()&&line[0]==L'>';
        const auto colon=line.find(L':');
        if (!heading && colon!=std::wstring::npos && line.find(L'%',colon)!=std::wstring::npos)
        {
            const auto& label = BenefitLabels[index];
            const auto& value = BenefitValues[index];
            Text(x+27,y,109,label.c_str());
            Text(x+136,y,40,value.c_str(),false,true,RT3_SORT_LEFT,true);
        }
        else Text(x+27,y,149,line.c_str()+(heading?1:0),heading,heading);
        UI::Theme::HoverTip(x+27,y,149,LineStep,line.c_str());
    }
    void DrawBenefits(int x,int y)
    {
        Text(x+15,y+132,160,PlanName.c_str(),true,true,RT3_SORT_CENTER,false,true);
        UI::Theme::HoverTip(x+15,y+131,160,20,PlanName.c_str());
        Text(x+27,y+151,83,L"Tiempo restante:");
        wchar_t days[32];const int selectedDays=SelectedRemainingDays();if(selectedDays==INT_MAX) wcscpy_s(days,L"Permanente");else swprintf_s(days,L"%d dias",selectedDays);
        Text(x+110,y+151,66,Lines.empty()?L"...":days,true);
        Text(x+27,y+167,83,L"Duracion:");Text(x+110,y+167,66,Duration.c_str());
        Text(x+15,y+200,160,L"BENEFICIOS",true,true,RT3_SORT_CENTER,false,true);
        for(int row=0;row<static_cast<int>(Benefits.size());++row)
            DrawBenefitRow(x,y+LineTop+row*BenefitStep(),row);
        if (Lines.empty())
            Text(x+25,y+LineTop,140,Pending?L"Consultando servidor...":L"Servidor sin respuesta",false,false,RT3_SORT_CENTER);
    }
    void DrawCosts(int x,int y)
    {
        const int top=CostTop();Text(x+15,y+CostHeadingTop(),160,L"COSTO",true,true,RT3_SORT_CENTER,false,true);
        for(int i=0;i<static_cast<int>(Costs.size());++i)
        {
            const auto& line=Costs[i];const bool met=!line.empty()&&line[0]==L'+';const bool missing=!line.empty()&&line[0]==L'!';
            g_pRenderText->SetFont(g_hFont);g_pRenderText->SetBgColor(0,0,0,0);
            g_pRenderText->SetTextColor(missing?255:met?105:255,missing?85:met?235:255,missing?85:met?125:255,255);
            g_pRenderText->RenderText(x+15,y+top+i*CostLineStep,line.c_str()+((met||missing)?1:0),160,0,RT3_SORT_CENTER);
            UI::Theme::HoverTip(x+15,y+top+i*CostLineStep,160,10,line.c_str()+((met||missing)?1:0));
        }
        if(!Result.empty()) UI::Theme::HoverTip(x+35,y+PurchaseTop+ExtraHeight(),120,28,Result.c_str());
    }
    void DrawFooter(int x,int y)
    {
        if(Mockup.Ready()) Mockup.Purchase(x+35,y+PurchaseTop+ExtraHeight(),120,28);
        else UI::Theme::Bevel(x+35,y+PurchaseTop+ExtraHeight(),120,28,L"",false,true);
        Text(x+35,y+PurchaseTop+ExtraHeight()+9,120,Pending?L"Procesando...":L"COMPRAR VIP",true,true,RT3_SORT_CENTER,false,true);
    }
    void DrawInteriorSeparators(int x, int y, int height)
    {
        if (CloseTexture == BITMAP_UNKNOWN) return;
        constexpr float SourceWidth = 512, SourceHeight = 1024;
        constexpr float LineLeft = 38, LineWidth = 437, LineHeight = 2;
        constexpr float LineTops[] = {291, 446, 742};
        for (const float top : LineTops)
            RenderBitmap(CloseTexture, x + LineLeft * Width / SourceWidth,
                y + top * height / SourceHeight, LineWidth * Width / SourceWidth,
                LineHeight * height / SourceHeight, LineLeft / SourceWidth,
                top / SourceHeight, LineWidth / SourceWidth, LineHeight / SourceHeight);
    }

    void Render()
    {
        if(!Open) return;
        if(!g_pNewUISystem||!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER)) {Open=false;return;}
        if(Pending&&timeGetTime()-RequestedAt>6000)
        {
            const bool wasPurchase=PendingPurchase;Pending=false;PendingPurchase=false;CanBuy=false;Quote=0;

            Result=wasPurchase?L"Verificando el estado de la compra...":L"Sin respuesta. Pulsa comprar para consultar.";
            if(wasPurchase) Request(false);
        }
        const int x=X(),y=Y();
        Reference.DrawVip(x, y, Width, Height, ExtraHeight());
        DrawInteriorSeparators(x, y, Height + ExtraHeight());
        DrawHeader(x,y);DrawBenefits(x,y);DrawCosts(x,y);DrawFooter(x,y);
    }
}

using namespace SEASON3B;
namespace Vip = UI::Character::VipPanel;

CParisVipWindow::~CParisVipWindow() { Release(); }

bool CParisVipWindow::Create(CNewUIManager* manager)
{
    if (manager == nullptr) return false;
    m_manager = manager;
    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    manager->AddUIObj(INTERFACE_PARIS_VIP, this);
    Vip::Mockup.Load();
    Vip::Reference.Load(L"Data\\Interface\\Paris_VipLayout.tga");
    Vip::CloseTexture = Bitmaps.LoadImage(L"Data\\Interface\\Paris_VipStats51.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    Show(false);
    return true;
}

void CParisVipWindow::Release()
{
    if (m_manager == nullptr) return;
    Vip::Open = false;
    Vip::Mockup.Unload();
    Vip::Reference.Unload();
    if (Vip::CloseTexture != BITMAP_UNKNOWN) DeleteBitmap(Vip::CloseTexture);
    Vip::CloseTexture = BITMAP_UNKNOWN;
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CParisVipWindow::ToggleWindow()
{
    Vip::Toggle();
    Show(Vip::Open);
}

bool CParisVipWindow::ReceivePacket(const BYTE* packet, int size) { return Vip::Receive(packet, size); }
bool CParisVipWindow::UpdateMouseEvent()
{
    const bool handled = Vip::Mouse();
    Show(Vip::Open);
    return !handled;
}

bool CParisVipWindow::UpdateKeyEvent()
{
    if (!IsVisible() || !IsPress(VK_ESCAPE)) return true;
    Vip::Open = false;
    Show(false);
    return false;
}

bool CParisVipWindow::Update()
{
    if (!IsVisible() || !g_pNewUISystem->IsVisible(INTERFACE_CHARACTER)) Vip::Open = false;
    Show(Vip::Open);
    return true;
}

bool CParisVipWindow::Render() { Vip::Render(); DisableAlphaBlend(); return true; }
float CParisVipWindow::GetLayerDepth() { return 8.5f; }
float CParisVipWindow::GetKeyEventOrder() { return 2.0f; }
