#include "stdafx.h"
#include "UI/NewUI/Inventory/ParisJewelBankWindow.h"
#include "UI/NewUI/Character/ParisExactAssets.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/NewUISystem.h"
#include "Network/Paris/ParisRequests.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Network/Paris/JewelBankProtocol.h"
#include <array>
#include <memory>
#include <cwctype>
namespace UI::Inventory::ParisBank
{
    constexpr int Width=190, Height=429, RowTop=80, RowHeight=36, VisibleRows=8, JewelCount=24;
    using Entry = SEASON3B::CParisJewelBankWindow::Jewel;
    Entry Entries[JewelCount]{};
    std::array<bool, JewelCount> ReceivedEntries{};
    bool HeaderReceived = false;
    const wchar_t* Status=L"Conectando con el servidor...";
    int Selected=-1, Scroll=0, Count=10, SnapshotCount=10;
    // Pixel-based offset, matching the Info panel's continuous scroll model.
    int ScrollOffset=0;
    int Filtered[JewelCount]={0,1,2,3,4,5,6,7,8,9};
    bool AutoPick=false, WasOpen=false, Pending=false, HasSnapshot=false;
    bool DraggingScroll=false;
    int ScrollDragOffset=0;
    unsigned int NextRequest=1, PendingRequest=0;
    DWORD PendingStartedAt=0;
    bool PendingIsList=false;
    wchar_t LastQuery[32]={};
    constexpr const wchar_t* Names[]={L"Bless",L"Soul",L"Life",L"Creation",L"Guardian",L"Gemstone",L"Harmony",L"Chaos",L"Low Stone",L"High Stone"};
    UI::Theme::ParisFrameAssets Art;

    UI::Theme::ParisExactAssets ScrollArt;
    std::unique_ptr<CUITextInputBox> Search;
    void SendCommand(const wchar_t* command)
    {
        if (Hero == nullptr || !Network::Paris::RequestJewelBank(Hero->ID, command))
        {
            Pending = false;
            Status = L"Sin conexion con el servidor";
            return;
        }

    }
    void Request(const wchar_t* operation)
    {
        if(Pending) return;
        if (NextRequest == 0) ++NextRequest;
        PendingRequest=NextRequest++;
        HeaderReceived = false;
        ReceivedEntries.fill(false);
        wchar_t command[128]={};
        swprintf_s(command,L"/parisbank %u %ls",PendingRequest,operation);
        Pending=true; PendingStartedAt=GetTickCount(); PendingIsList=wcscmp(operation,L"list")==0;
        SendCommand(command);
    }
    void ResetSnapshot()
    {
        HasSnapshot=false; Count=static_cast<int>(_countof(Names)); Scroll=0; ScrollOffset=0; Selected=-1;
        for(int i=0;i<Count;++i) Filtered[i]=i;
        LastQuery[0]=L'\x01';
    }
    void CheckResponseTimeout()
    {
        if(!Pending || static_cast<DWORD>(GetTickCount()-PendingStartedAt)<10000) return;
        const bool wasList=PendingIsList;
        Pending=false; PendingRequest=0; ResetSnapshot();
        // Reconcile balances, never repeat a mutation whose result is unknown.
        if(!wasList) {Request(L"list"); Status=L"Comprobando saldo...";}
        else Status=L"Sin respuesta. Clic para recargar";
    }
    void Transfer(int row,bool deposit,int quantity)
    {
        if(Pending || !HasSnapshot || row<0 || row>=SnapshotCount) return;
        wchar_t operation[64]={}; swprintf_s(operation,L"%ls %d %d",deposit?L"deposit":L"withdraw",row,quantity);
        Request(operation); Status=L"Procesando operacion...";
    }
    bool DepositInventoryItem(SEASON3B::CNewUIInventoryCtrl* inventory)
    {
        if(!WasOpen || !inventory || !SEASON3B::IsPress(VK_RBUTTON)
            || SEASON3B::CNewUIInventoryCtrl::GetPickedItem() || inventory->IsLocked()) return false;
        ITEM* item=inventory->FindItemAtPt(MouseX,MouseY);
        if(!item) return false;
        if(!HasSnapshot || Pending)
        {
            MouseRButton=MouseRButtonPop=MouseRButtonPush=false;
            return true;
        }
        for(int row=0;row<SnapshotCount;++row)
        {
            if(Entries[row].ItemIndex!=item->Type || Entries[row].Level!=(item->Level>>3)) continue;
            const int slot=inventory->GetIndexByItem(item);
            if(slot<MAX_EQUIPMENT || slot>255) return false;
            wchar_t operation[64]={};
            swprintf_s(operation,L"deposit-slot %d %d %d",slot,item->Type,item->Level>>3);
            Selected=row;
            Request(operation);
            Status=L"Guardando joya...";
            MouseRButton=MouseRButtonPop=MouseRButtonPush=false;
            return true;
        }
        return false;
    }

    void ReceiveHeader(const Network::Paris::JewelBankMessage& message)
    {
        HeaderReceived = true;
        ReceivedEntries.fill(false);
        AutoPick = message.automatic;
        SnapshotCount = message.count;
        HasSnapshot = false;
        for (auto& entry : Entries) entry = {};
        if (message.status == 0) Status = L"Jewel Bank actualizado";
        else if (message.status == 3) Status = L"Jewel Bank desactivado";
        else if (message.status == 4) Status = L"Auto Pick no disponible";
        else if (message.status == 5) Status = L"Joya no configurada";
        else Status = L"Operacion no disponible";
    }

    void ReceiveEntry(const Network::Paris::JewelBankMessage& message)
    {
        if (!HeaderReceived || message.row >= SnapshotCount) return;
        Entry& entry = Entries[message.row];
        entry.ItemIndex = message.item;
        entry.Level = message.level;
        entry.Inventory = message.inventory;
        entry.Balance = message.balance;
        entry.Maximum = message.maximum;
        std::copy(message.name.begin(), message.name.end(), entry.Name);
        entry.Name[message.name.size()] = L'\0';
        swprintf_s(entry.InventoryText, L"%d", entry.Inventory);
        swprintf_s(entry.BalanceText, L"%lld", entry.Balance);
        ReceivedEntries[message.row] = true;
    }

    void CompleteSnapshot()
    {
        HasSnapshot = HeaderReceived
            && std::all_of(ReceivedEntries.begin(), ReceivedEntries.begin() + SnapshotCount,
                [](bool received) { return received; });
        Pending = false;
        PendingRequest = 0;
        if (!HasSnapshot)
        {
            ResetSnapshot();
            Status = L"Respuesta incompleta. Clic para recargar";
            return;
        }
        LastQuery[0] = L'\x01';
    }

    bool Receive(const wchar_t* text)
    {
        if (text == nullptr || wcsncmp(text, L"[#PJB", 5) != 0) return false;
        Network::Paris::JewelBankMessage message;
        if (!Pending || !Network::Paris::JewelBank::Decode(text, PendingRequest, message)) return true;
        using Kind = Network::Paris::JewelBankMessage::Kind;
        switch (message.kind)
        {
        case Kind::Header: ReceiveHeader(message); break;
        case Kind::Entry: ReceiveEntry(message); break;
        case Kind::Complete: CompleteSnapshot(); break;
        }
        return true;
    }
    int OriginX()
    {
        const auto* inventory=g_pNewUISystem->GetUI_NewMyInventory();
        return inventory ? std::max(0,static_cast<int>(inventory->GetPos().x)-Width+10):0;
    }
    int OriginY()
    {
        const auto* inventory=g_pNewUISystem->GetUI_NewMyInventory();
        return inventory ? static_cast<int>(inventory->GetPos().y):0;
    }
    void Load()
    {
        Art.Load(); ScrollArt.Load();
        Search=std::make_unique<CUITextInputBox>();
        Search->Init(g_hWnd,156,14,31);
        Search->SetText(L"");
        Search->SetTextColor(255,235,224,200);
        Search->SetBackColor(0,0,0,0); Search->SetFont(g_hFont);
        Search->SetState(UISTATE_HIDE);
    }
    void Unload() { Search.reset(); Art.Unload(); ScrollArt.Unload(); WasOpen=false; }
    bool Contains(const wchar_t* name,const wchar_t* query)
    {
        if(!*query) return true;
        for(const wchar_t* start=name;*start;++start)
        {
            const wchar_t* a=start; const wchar_t* b=query;
            while(*a && *b && std::towlower(*a)==std::towlower(*b)) {++a;++b;}
            if(!*b) return true;
        }
        return false;
    }
    void Sync(bool open)
    {
        if(!Search) return;
        if(open != WasOpen)
        {
            Search->SetState(open ? UISTATE_NORMAL:UISTATE_HIDE);
            if(!open && Search->HaveFocus()) CUITextInputBox::ReleaseFocus();
            WasOpen=open;
            DraggingScroll=false;
            if(open) { Pending=false; ResetSnapshot(); Request(L"list"); }
        }
        if(!open) return;
        CheckResponseTimeout();
        Search->SetPosition(OriginX()+15,OriginY()+55);
        wchar_t query[32]={}; Search->GetText(query,32);
        if(wcscmp(query,LastQuery)==0) return;
        wcscpy_s(LastQuery,query); Count=0; Scroll=0; ScrollOffset=0; Selected=-1;
        const int available=HasSnapshot ? SnapshotCount : 10; int filtered=0;
        for(int i=0;i<available;++i)
        {
            const wchar_t* name=HasSnapshot ? Entries[i].Name : Names[i];
            if(Contains(name,query)) Filtered[filtered++]=i;
        }
        Count=filtered;
    }
    int VisualRow(int index)
    {
        const int first=ScrollOffset/RowHeight;
        const int offset=ScrollOffset-first*RowHeight;
        for(int row=0;row<=VisibleRows && first+row<Count;++row)
            if(Filtered[first+row]==index) return static_cast<int>(row*RowHeight-offset);
        // A missing row must be outside the clip area, including during partial scrolling.
        return -RowHeight;
    }
    bool IsVisible(int index)
    {
        const int rowY=VisualRow(index);
        return rowY>-RowHeight && rowY<VisibleRows*RowHeight;
    }
    int CardX(int) {return OriginX()+8;}
    int CardY(int index) {return OriginY()+RowTop+VisualRow(index);}
    int MaximumScrollPixels() {return std::max(0,Count-VisibleRows)*RowHeight;}
    void ScrollBy(int amount)
    {
        ScrollOffset=std::clamp(ScrollOffset+amount,0,MaximumScrollPixels());
        Scroll=ScrollOffset/RowHeight;
    }
    int ScrollThumbHeight()
    {
        return 26;
}
    int ScrollThumbY()
    {
        const int movable=VisibleRows*RowHeight-24-ScrollThumbHeight();
        const int maximum=MaximumScrollPixels();
        return OriginY()+RowTop+12+(maximum>0?(ScrollOffset*movable)/maximum:0);
    }
    void ScrollFromThumb(int pointerY)
    {
        const int movable=VisibleRows*RowHeight-24-ScrollThumbHeight();
        if(movable<=0) {Scroll=0;ScrollOffset=0;return;}
        const int relative=std::clamp(pointerY-OriginY()-RowTop-12,0,movable);
        ScrollOffset=(relative*MaximumScrollPixels())/movable;
        Scroll=ScrollOffset/RowHeight;
    }
    bool Mouse(bool& open)
    {
        Sync(open);
        if(Search) Search->DoAction();
        const int x=OriginX(),y=OriginY();
        if(DraggingScroll)
        {
            if((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
            {
                ScrollFromThumb(MouseY-ScrollDragOffset);
                UI::Paris::ConsumeMouse();
                return true;
            }
            DraggingScroll=false;
        }
        if(!SEASON3B::CheckMouseIn(x,y,Width,Height)) return false;
        const bool pressed=SEASON3B::IsPress(VK_LBUTTON);
        UI::Paris::ConsumeMouse();
        if(MouseWheel) {ScrollBy(-MouseWheel*45);MouseWheel=0;}
        if(!pressed) return true;
        const int arrow=UI::Theme::ParisExactAssets::ScrollArrow(x+175,y+RowTop+12,6,VisibleRows*RowHeight-24);
        if(arrow) {ScrollBy(arrow*45);return true;}
        if(SEASON3B::CheckMouseIn(x+13,y+391,36,29) || SEASON3B::CheckMouseIn(x+169,y+7,13,12)) {open=false;Sync(false);return true;}
        if(SEASON3B::CheckMouseIn(x+55,y+397,110,17)) {if(!Pending && HasSnapshot) {Request(AutoPick?L"auto 0":L"auto 1");Status=L"Guardando Auto Pick...";}return true;}
        if(!Pending && !HasSnapshot && SEASON3B::CheckMouseIn(x+8,y+374,Width-16,15))
        {Request(L"list");Status=L"Actualizando saldo...";return true;}
        if(SEASON3B::CheckMouseIn(x+12,y+52,164,21)) {if(Search) Search->GiveFocus();return true;}
        if(Search && Search->HaveFocus()) CUITextInputBox::ReleaseFocus();
        if(SEASON3B::CheckMouseIn(x+171,y+RowTop,14,VisibleRows*RowHeight))
        {
            if(Count>VisibleRows)
            {
                const int thumbY=ScrollThumbY();
                ScrollDragOffset=MouseY>=thumbY && MouseY<thumbY+ScrollThumbHeight()?MouseY-thumbY:ScrollThumbHeight()/2;
                DraggingScroll=true;
                ScrollFromThumb(MouseY-ScrollDragOffset);
            }
            return true;
        }
        if (!SEASON3B::CheckMouseIn(x + 8, y + RowTop, 163, VisibleRows * RowHeight)) return true;
        const int first=ScrollOffset/RowHeight;
        const int offset=ScrollOffset-first*RowHeight;
        for(int row=0;row<=VisibleRows && first+row<Count;++row)
        {
            const int index=Filtered[first+row], ry=y+RowTop+row*RowHeight-offset;
            for(int col=0;col<3;++col)
            {
                const int bx=x+119+col*18, quantity=col==0?1:(col==1?10:0);
                if(SEASON3B::CheckMouseIn(bx,ry+11,17,12)) {Selected=index;Transfer(index,true,quantity);return true;}
                if(SEASON3B::CheckMouseIn(bx,ry+24,17,12)) {Selected=index;Transfer(index,false,quantity);return true;}
            }
            if(SEASON3B::CheckMouseIn(x+9,ry,164,RowHeight)) {Selected=index;break;}
        }
        return true;
    }
    void RenderFrame(int x, int y)
    {
        UI::Paris::Frame(x, y, Width, Height);
        constexpr DWORD BorderGold = 0xFFC99A38u, ListBlack = 0xE0080909u;
        constexpr int ListWidth = 164, Border = 1;
        RenderColorQuadARGB(x + 8, y + RowTop - Border, ListWidth + Border * 2,
            VisibleRows * RowHeight + Border * 2, BorderGold);
        RenderColorQuadARGB(x + 9, y + RowTop, ListWidth, VisibleRows * RowHeight, ListBlack);
    }
    void RenderRowBackground(int x, int y, int visibleIndex)
    {
        constexpr int Inset = 9, RowWidth = 164, SeparatorHeight = 1;
        constexpr DWORD EvenColor = 0x30000000u, OddColor = 0x301A150Bu, Separator = 0x604F3B16u;
        RenderColorQuadARGB(x + Inset, y, RowWidth, RowHeight, visibleIndex % 2 == 0 ? EvenColor : OddColor);
        RenderColorQuadARGB(x + Inset, y + RowHeight - SeparatorHeight, RowWidth, SeparatorHeight, Separator);
    }

    void RenderTransferButton(int x, int y, const wchar_t* label, bool deposit)
    {
        constexpr int ButtonWidth = 17, ButtonHeight = 12;
        const bool enabled = !Pending && HasSnapshot;
        const bool hovered = enabled && SEASON3B::CheckMouseIn(x, y, ButtonWidth, ButtonHeight);
        const DWORD color = hovered ? (deposit ? 0xFF398C46u : 0xFF97513Au) : 0xFF3C3018u;
        RenderColorQuadARGB(x, y, ButtonWidth, ButtonHeight, 0xFFB58B35u);
        RenderColorQuadARGB(x + 1, y + 1, ButtonWidth - 2, ButtonHeight - 2, color);
        UI::Character::ParisStyle::Label(x, y + 1, ButtonWidth, label);
    }
    void Render(const wchar_t* const* names)
    {
        using namespace UI::Character::ParisStyle;
        Sync(true); const int x=OriginX(),y=OriginY();
        RenderFrame(x,y);
        Label(x+10,y+43,Width-20,L"Jewel Bank");
        UI::Theme::Bevel(x+12,y+52,164,21,L"");
        if (Search) Search->Render();
        wchar_t searchText[32] = {};
        if (Search) Search->GetText(searchText, _countof(searchText));
        if (searchText[0] == L'\0' && (!Search || !Search->HaveFocus()))
        {
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->SetTextColor(190, 190, 190, 255);
            g_pRenderText->RenderText(x + 20, y + 57, L"Buscar joya...", 142, 0, RT3_SORT_LEFT);
        }
        constexpr const wchar_t* quantity[]={L"x1",L"x10",L"ALL"};
        const int first=ScrollOffset/RowHeight;
        const int pixelOffset=ScrollOffset-first*RowHeight;
        UI::Paris::Clip(x, y + RowTop, Width, VisibleRows * RowHeight);
        for(int row=0;row<=VisibleRows && first+row<Count;++row)
        {
            const int index=Filtered[first+row], ry=y+RowTop+row*RowHeight-pixelOffset;
            RenderRowBackground(x, ry, first + row);
            const bool active=!DraggingScroll && SEASON3B::CheckMouseIn(x+10,ry,163,RowHeight);
            const wchar_t* title=HasSnapshot?Entries[index].Name:names[index];
            g_pRenderText->SetFont(g_hFont); g_pRenderText->SetBgColor(0,0,0,0);
            g_pRenderText->SetTextColor(235,195,85,255);
            g_pRenderText->RenderText(x+35,ry+3,title,active?83:136,0,RT3_SORT_LEFT);
            Label(x+30,ry+15,48,L"Inventario");
            Label(x+78,ry+15,36,L"Banco");
            Label(x+30,ry+26,48,HasSnapshot ? Entries[index].InventoryText : L"--");
            Label(x+78,ry+26,36,HasSnapshot ? Entries[index].BalanceText : L"--");
            if(!active) continue;
            for(int col=0;col<3;++col)
            {
                const int bx=x+119+col*18;
                Label(bx,ry,17,quantity[col]);
                RenderTransferButton(bx,ry+11,L"+",true);
                RenderTransferButton(bx,ry+24,L"-",false);
            }
        }
        UI::Paris::ResetClip();
        const bool scrollHover=DraggingScroll || SEASON3B::CheckMouseIn(x+171,ScrollThumbY(),14,ScrollThumbHeight());
        ScrollArt.Scroll(x+175,y+RowTop+12,6,VisibleRows*RowHeight-24,ScrollThumbY(),ScrollThumbHeight());
        Label(x+8,y+378,Width-16,Status);
        Art.Exit(x+13,y+391); UI::Theme::Check(x+55,y+397,AutoPick);
        Label(x+74,y+399,92,L"Auto Pick");
    }
}


using namespace SEASON3B;
namespace Bank = UI::Inventory::ParisBank;

CParisJewelBankWindow::~CParisJewelBankWindow() { Release(); }

bool CParisJewelBankWindow::Create(CNewUIManager* manager)
{
    if (manager == nullptr) return false;
    m_manager = manager;
    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    m_manager->AddUIObj(INTERFACE_PARIS_JEWEL_BANK, this);
    g_pNewUI3DRenderMng->Add3DRenderObj(this);
    Bank::Load();
    SetRelatedWnd(Bank::Search->GetHandle());
    Show(false);
    return true;
}

void CParisJewelBankWindow::Release()
{
    if (m_manager == nullptr) return;
    g_pNewUI3DRenderMng->Remove3DRenderObj(this);
    Bank::Pending = false;
    Bank::PendingRequest = 0;
    Bank::HasSnapshot = false;
    Bank::Unload();
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

bool CParisJewelBankWindow::ReceiveText(const wchar_t* text) { return Bank::Receive(text); }

bool CParisJewelBankWindow::UpdateMouseEvent()
{
    if (!IsVisible()) return true;
    if (Bank::DepositInventoryItem(g_pMyInventory->GetInventoryCtrl())) return false;
    bool open = true;
    const bool consumed = Bank::Mouse(open);
    if (!open) Show(false);
    return !consumed;
}

bool CParisJewelBankWindow::UpdateKeyEvent()
{
    if (!IsVisible() || !IsPress(VK_ESCAPE)) return true;
    Show(false);
    Bank::Sync(false);
    return false;
}

bool CParisJewelBankWindow::Update()
{
    if (IsVisible() && !g_pNewUISystem->IsVisible(INTERFACE_INVENTORY)) Show(false);
    Bank::Sync(IsVisible());
    return true;
}

bool CParisJewelBankWindow::Render()
{
    if (IsVisible()) Bank::Render(Bank::Names);
    DisableAlphaBlend();
    return true;
}

void CParisJewelBankWindow::Render3D()
{
    if (!IsVisible()) return;
    constexpr int DefaultItems[] = {ITEM_JEWEL_OF_BLESS, ITEM_JEWEL_OF_SOUL, ITEM_JEWEL_OF_LIFE,
        ITEM_JEWEL_OF_CREATION, ITEM_JEWEL_OF_GUARDIAN, ITEM_GEMSTONE, ITEM_JEWEL_OF_HARMONY,
        ITEM_JEWEL_OF_CHAOS, ITEM_LOWER_REFINE_STONE, ITEM_HIGHER_REFINE_STONE};
    Bank::Sync(true);
    UI::Paris::Clip(Bank::OriginX() + 8, Bank::OriginY() + Bank::RowTop,
        Bank::Width - 16, Bank::VisibleRows * Bank::RowHeight);
    const int count = Bank::HasSnapshot ? Bank::SnapshotCount : static_cast<int>(_countof(DefaultItems));
    for (int row = 0; row < count; ++row)
    {
        if (!Bank::IsVisible(row)) continue;
        const int item = Bank::HasSnapshot ? Bank::Entries[row].ItemIndex : DefaultItems[row];
        if (item < 0) continue;
        RenderItem3D(Bank::CardX(row) + 3, Bank::CardY(row) + 4, 23, 26, item,
            Bank::HasSnapshot ? Bank::Entries[row].Level : 0, 0, 0, false);
    }
    UI::Paris::ResetClip();
}

bool CParisJewelBankWindow::IsVisible() const { return CNewUIObj::IsVisible(); }
CNewUIObj* CParisJewelBankWindow::GetLayoutOwner() const { return const_cast<CParisJewelBankWindow*>(this); }
float CParisJewelBankWindow::GetLayerDepth() { return 8.5f; }
float CParisJewelBankWindow::GetKeyEventOrder() { return 2.0f; }
