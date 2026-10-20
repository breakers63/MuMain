#include "stdafx.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "AuctionPanel.h"
#include "AuctionViewScope50.h"
#include "ParisAuctionWidgets50.h"
#include "ParisFeatureSkin49.h"
#include "Network/Paris/AuctionProtocol.h"
#include "UI/NewUI/HUD/ParisOverlayInput.h"
#include "UI/NewUI/Character/PanelAspectScale.h"
#include "UI/NewUI/NPCs/ParisNpcSkin56.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Legacy/UIControls.h"
#include "Engine/Object/ZzzInventory.h"
#include <memory>

namespace UI::Auctions
{
    namespace
    {
        using namespace Network::Paris::Auctions;
        constexpr int Width = 854, Height = 584, PageSize = 7, InventoryPageSize = 16;
        constexpr int RowY = 176, RowHeight = 50;
        UI::Theme::ParisFeatureSkin49 skin;
        UI::Theme::ParisAuctionWidgets50 widgets;
        UI::Theme::ParisFrameAssets frame;
        std::unique_ptr<CUITextInputBox> search, priceInput, buyInput;
        std::vector<ITEM*> previews;
        bool visible = false, pending = false, picker = true, filterOpen=false;
        HFONT titleFont = nullptr;
        int tab = 0, page = 0, subTab = 0, group = 255, selected = -1, inventoryPage = 0, currency = 0, duration = 0, ox = 0, oy = 0;
        float scale = 1;
        DWORD observed = 0, sent = 0, lastPoll = 0;
        Row confirmation;
        BYTE confirmationAction = 0, pendingAction = 0;
        std::wstring query;
        std::string selectedSource;

        int W(int value) { return UI::Character::PanelAspect::Axis(value, scale, g_fScreenRate_x); }
        int H(int value) { return UI::Character::PanelAspect::Axis(value, scale, g_fScreenRate_y); }
        int X(int value) { return ox + W(value); }
        int Y(int value) { return oy + H(value); }
        bool Hit(int x, int y, int width, int height) { return SEASON3B::CheckMouseIn(X(x), Y(y), W(width), H(height)); }

        void Layout()
        {
            scale = UI::Character::PanelAspect::Fit(WindowWidth, WindowHeight * .9f, Width, Height);
            ox = static_cast<int>((WindowWidth - Width * scale) / (2 * g_fScreenRate_x));
            oy = static_cast<int>((WindowHeight - Height * scale) / (2 * g_fScreenRate_y));
            search->SetPosition(X(220),Y(106));search->SetSize(W(185),H(20));
            priceInput->SetPosition(X(682), Y(191)); priceInput->SetSize(W(110), H(25));
            buyInput->SetPosition(X(682), Y(271)); buyInput->SetSize(W(110), H(25));
            for (auto* input : { search.get(), priceInput.get(), buyInput.get() }) input->SetFont(g_hFont);
        }

        void Text(int x, int y, int width, const std::wstring& text, bool gold = true, int alignment = RT3_SORT_LEFT)
        {
            EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1); g_pRenderText->SetFont(g_hFont); g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->SetTextColor(gold ? RGBA(255, 220, 130, 255) : RGBA(230, 230, 235, 255));
            g_pRenderText->RenderText(X(x), Y(y), text.c_str(), W(width), 0, alignment);
        }

        void Button(int x, int y, int width, int height, const std::wstring& caption, bool enabled = true, bool active = false)
        {
            const int color=caption==Label("Cancel") || caption==Label("Close")?0:
                caption==Label("Publish") || caption==Label("Buy") || caption==Label("Bid") || caption==Label("Collect") || caption==Label("Confirm")?2:1;
            widgets.Button(X(x),Y(y),W(width),H(height),enabled,active || enabled && Hit(x,y,width,height),color);
            skin.Caption(X(x), Y(y), W(width), H(height), caption.c_str(), enabled);
        }

        bool Matches()
        { return Received && MatchesAuctionView(tab,page,subTab,Data.Tab,Data.Page,Data.SubTab) && Data.Group==group && Data.Query==query; }

        const std::vector<Row>& Rows()
        { static const std::vector<Row> empty;return Matches()?Data.Rows:empty; }

        int TextWidth(const std::wstring& value)
        {
            g_pRenderText->SetFont(g_hFont);
            const SIZE size = g_pRenderText->MeasureText(value.c_str(), static_cast<int>(value.size()));
            return static_cast<int>(size.cx/std::max(.01f,g_fScreenRate_x));
        }

        std::wstring Amount(int number)
        {
            auto value=std::to_wstring(std::max(0,number));
            for(int pos=static_cast<int>(value.size())-3;pos>0;pos-=3)value.insert(pos,L",");
            return value;
        }

        void Cell(int left,int right,int y,const std::wstring& value,bool gold=false)
        {
            const int width=right-left-12;auto shown=value;
            if(TextWidth(shown)>W(width))
            {while(shown.size()>1&&TextWidth(shown+L"...")>W(width))shown.pop_back();shown+=L"...";}
            Text(left+6,y,width,shown,gold,RT3_SORT_CENTER);
        }

        int MoneyLeft(int left,int right,int amount)
        { return left+(right-left)/2-static_cast<int>((TextWidth(Amount(amount))+W(24))/(2*std::max(.01f,scale/g_fScreenRate_x))); }

        void Money(int left,int right,int y,int amount,int currency)
        {
            const int x=MoneyLeft(left,right,amount);
            if(currency<=1)widgets.Coin(X(x),Y(y-2),W(18),H(20));
            Text(x+24,y,right-x-26,Amount(amount),false);
        }

        std::array<int,8> Columns()
        {
            if(tab==2)return {190,248,385,495,620,717,825,825};
            if(tab==3)return {190,248,395,610,715,825,825,825};
            if(tab==4)return {190,248,395,520,625,715,825,825};
            return {190,248,369,449,549,638,722,825};
        }

        std::pair<int,int> MoneyCell()
        {const auto columns=Columns();const int index=tab==0?3:2;return {columns[index],columns[index+1]};}

        void Tooltip()
        {
            if(!Matches() || observed!=Received || confirmationAction || filterOpen)return;
            const auto& rows=Rows();
            for(size_t index=0;index<rows.size()&&index<previews.size();++index)
            {
                if(!previews[index])continue;
                int x=192,y=RowY+static_cast<int>(index)*RowHeight+3,w=48,h=43;
                if(tab==1)
                {
                    if(!picker&&selected>=0){if(index!=static_cast<size_t>(selected))continue;x=215;y=147;w=195;h=137;}
                    else{const int tile=static_cast<int>(index)-inventoryPage*InventoryPageSize;if(tile<0||tile>=InventoryPageSize)continue;x=201+tile%4*55;y=157+tile/4*75;w=48;h=65;}
                }
                if(Hit(x,y,w,h)){RenderMarketItemInfo(std::clamp(MouseX+12,12,400),std::clamp(MouseY+12,30,240),previews[index]);return;}
            }
        }

        void ClearPreviews()
        {
            if (g_pNewItemMng) for (auto* item : previews) if (item) g_pNewItemMng->DeleteItem(item);
            previews.clear();
        }

        bool Send(BYTE action,const Row* row,int price,int buyNow);

        void Synchronize()
        {
            if (observed == Received) return;
            observed = Received;
            const bool published=pendingAction==1 && Data.Action==1 && Data.Decision==0;
            if(Data.Action==pendingAction){pending=false;pendingAction=0;}
            Data.Currencies.erase(std::remove(Data.Currencies.begin(),Data.Currencies.end(),2),Data.Currencies.end());
            ClearPreviews();
            for (const auto& row : Data.Rows) previews.push_back(row.ItemData.empty() || !g_pNewItemMng ? nullptr : g_pNewItemMng->CreateItem(row.ItemData));
            if (Data.Tab == 1 && !selectedSource.empty())
            {
                auto found = std::find_if(Data.Rows.begin(), Data.Rows.end(), [](const Row& row) { return row.SourceId == selectedSource; });
                selected = found == Data.Rows.end() ? -1 : static_cast<int>(std::distance(Data.Rows.begin(), found));
                if (selected < 0) { picker = true; selectedSource.clear(); }
            }
            else if (selected >= static_cast<int>(Data.Rows.size())) selected = -1;
            if (tab == 1 && !Data.Currencies.empty() && std::find(Data.Currencies.begin(), Data.Currencies.end(), currency) == Data.Currencies.end()) currency = Data.Currencies.front();
            if (duration >= static_cast<int>(Data.Durations.size())) duration = 0;
            if(published)
            {
                tab=page=subTab=inventoryPage=0;group=255;query.clear();search->SetText(L"");
                selected=-1;selectedSource.clear();picker=true;Send(0,nullptr,0,0);
            }
        }

        bool Send(BYTE action = 0, const Row* row = nullptr, int price = 0, int buyNow = 0)
        {
            int minutes = Data.Durations.empty() ? 60 : Data.Durations[duration];
            if (!Request(action, static_cast<BYTE>(tab), static_cast<BYTE>(page), static_cast<BYTE>(subTab), query,
                static_cast<BYTE>(group), row, price, buyNow, minutes, static_cast<BYTE>(currency))) return false;
            sent = lastPoll = timeGetTime(); pending = action != 0;pendingAction=action; return true;
        }

        int Number(CUITextInputBox& input)
        {
            wchar_t text[16]{}; input.GetText(text, 16);
            if (!*text) return 0;
            wchar_t* end = nullptr; const auto value = _wcstoi64(text, &end, 10);
            return end && !*end && value >= 0 && value <= INT_MAX ? static_cast<int>(value) : -1;
        }

        std::wstring CurrencyName(int value)
        {
            const auto key = "Currency" + std::to_string(value); return Label(key.c_str());
        }

        std::wstring Remaining(const Row& row)
        {
            auto seconds = std::max(0LL, row.Ends - Data.ServerTime - static_cast<long long>((timeGetTime() - Received) / 1000));
            wchar_t text[40]{}; swprintf_s(text, L"%02lld:%02lld:%02lld", seconds / 3600, seconds / 60 % 60, seconds % 60); return text;
        }

        constexpr int FilterX=436,FilterY=101,FilterWidth=190,FilterHeight=30,OptionHeight=26;
        constexpr int FilterCodes[]={255,16,6,17,20,18,19,21};
        constexpr const wchar_t* FilterNames[]={L"Todos",L"Armas",L"Escudos",L"Armaduras",L"Alas / capas",L"Mascotas",L"Scrolls / poderes",L"Accesorios"};

        const wchar_t* FilterName()
        {
            for(int index=0;index<8;++index)if(FilterCodes[index]==group)return FilterNames[index];
            return FilterNames[0];
        }

        void SearchBar()
        {
            skin.Outline(X(190),Y(101),W(228),H(30),true);
            search->SetState(UISTATE_NORMAL);search->Render();
            skin.Sprite(X(196),Y(105),W(18),H(21),196,100,22,25);
            if(!search->HaveFocus() && query.empty())Text(220,110,185,L"Buscar item...",false,RT3_SORT_CENTER);
            Button(FilterX,FilterY,FilterWidth,FilterHeight,FilterName(),true,filterOpen);
            Text(FilterX+FilterWidth-22,FilterY+8,14,L"v",false,RT3_SORT_CENTER);
            Button(720,101,105,30,Label("Search"));
        }

        void FilterMenu()
        {
            if(tab!=0 || !filterOpen)return;
            for(int index=0;index<8;++index)
            {
                const int y=FilterY+FilterHeight+index*OptionHeight;
                widgets.Button(X(FilterX),Y(y),W(FilterWidth),H(OptionHeight),true,group==FilterCodes[index]||Hit(FilterX,y,FilterWidth,OptionHeight));
                Text(FilterX+4,y+7,FilterWidth-8,FilterNames[index],group==FilterCodes[index],RT3_SORT_CENTER);
            }
        }

        void SubTabs()
        {
            const char* first = tab == 2 ? "MyBids" : tab == 3 ? "ItemsToReceive" : "ActiveListings";
            const char* second = tab == 2 ? "Purchases" : tab == 3 ? "CoinsToReceive" : "History";
            Button(190, 95, 200, 38, Label(first), true, subTab == 0);
            Button(400, 95, 200, 38, Label(second), true, subTab == 1);
        }

        std::wstring Date(long long stamp)
        {
            if(stamp<=0) return L"";
            const __time64_t seconds=stamp;tm value{};wchar_t text[32]{};
            if(_gmtime64_s(&value,&seconds)!=0) return L"";
            wcsftime(text,32,L"%Y-%m-%d %H:%M",&value);return text;
        }

        std::wstring OptionCaption(size_t index)
        {
            if (index >= previews.size() || !previews[index]) return {};
            const auto* item = previews[index];
            std::wstring text = L"+" + std::to_wstring(Data.Rows[index].Level);
            if (item->HasSkill) text += L" +" + Label("Skill");
            if (item->HasLuck) text += L" +" + Label("Luck");
            return text;
        }

        void Table()
        {
            if(tab==0)SearchBar();else SubTabs();
            const auto columns=Columns();const int count=tab==0?7:tab==3?5:6;
            const char* headers[]={"Item","Name","Options","Price","Seller","Time",""};
            if(tab==2){headers[2]="Offer";headers[3]="Seller";headers[4]="Time";headers[5]="State";}
            if(tab==3){headers[2]=subTab==0?"Date":"Price";headers[3]="State";headers[4]="";}
            if(tab==4){headers[2]="Price";headers[3]="Time";headers[4]="State";headers[5]="";}
            skin.Fill(X(190),Y(143),W(635),H(29),.10f,.11f,.10f);skin.Outline(X(190),Y(143),W(635),H(383));
            for(int column=0;column<count;++column)Cell(columns[column],columns[column+1],153,Label(headers[column]));
            for(int column=1;column<count;++column)skin.Fill(X(columns[column]),Y(143),W(1),H(383),.22f,.23f,.20f);
            for(int row=0;row<PageSize;++row)skin.Separator(X(190),Y(RowY+(row+1)*RowHeight),W(635));
            const auto& rows=Rows();
            for(size_t index=0;index<rows.size()&&index<PageSize;++index)
            {
                const auto& row=rows[index];const int y=RowY+static_cast<int>(index)*RowHeight,base=y+18;
                skin.Outline(X(197),Y(y+4),W(43),H(42));Cell(columns[1],columns[2],base,row.Name,true);
                const auto state=tab==2?row.State:row.Status==1?"Sold":row.Status==2?"Expired":row.Status==3?"Cancelled":"Active";
                if(tab==0){Cell(columns[2],columns[3],base,OptionCaption(index));Money(columns[3],columns[4],base,row.Price,row.Currency);Cell(columns[4],columns[5],base,row.Seller);Cell(columns[5],columns[6],base,Remaining(row),true);}
                else if(tab==2){Money(columns[2],columns[3],base,row.Amount,row.Currency);Cell(columns[3],columns[4],base,row.Seller);Cell(columns[4],columns[5],base,Remaining(row));Cell(columns[5],columns[6],base,Label(state.c_str()),true);}
                else if(tab==3){if(subTab==0)Cell(columns[2],columns[3],base,Date(row.Time));else Money(columns[2],columns[3],base,row.Amount,row.Currency);Cell(columns[3],columns[4],base,Label("Available"),true);}
                else{Money(columns[2],columns[3],base,row.Price,row.Currency);Cell(columns[3],columns[4],base,Remaining(row));Cell(columns[4],columns[5],base,Label(state.c_str()),true);}
                if(tab==0)Button(725,y+9,95,32,Label(row.BuyNow>0?"Buy":"Bid"),Data.Enabled&&!row.Owned&&!pending);
                else if(tab==3)Button(725,y+9,95,32,Label("Collect"),!pending);
                else if(tab==4&&subTab==0)Button(725,y+9,95,32,Label("Cancel"),row.CanCancel&&!pending);
            }
            if(Matches()&&rows.empty()){const char* empty=tab==0?"NoListings":tab==2?"NoOffers":tab==3?"NoDeliveries":"NoOwnListings";Text(230,330,550,Label(empty),false,RT3_SORT_CENTER);}
            Button(392,536,38,27,L"<",Matches()&&page>0);Text(441,545,117,std::to_wstring(page+1),false,RT3_SORT_CENTER);
            Button(574,536,38,27,L">",Matches()&&Data.More&&page<100);
        }

        void Publishing()
        {
            skin.Outline(X(190), Y(98), W(245), H(433), true);
            if (picker || selected < 0)
            {
                Text(197, 117, 232, Label("ChooseInventory"));
                for (int index = 0; index < InventoryPageSize; ++index)
                {
                    const int row = inventoryPage * InventoryPageSize + index;
                    if (row >= static_cast<int>(Rows().size())) break;
                    const int x = 201 + index % 4 * 55, y = 157 + index / 4 * 75;
                    skin.Outline(X(x), Y(y), W(48), H(65), true);
                }
                Button(205, 476, 50, 30, L"<", inventoryPage > 0);
                Text(267, 486, 91, std::to_wstring(inventoryPage + 1), false, RT3_SORT_CENTER);
                Button(370, 476, 50, 30, L">", (inventoryPage + 1) * InventoryPageSize < static_cast<int>(Rows().size()));
            }
            else
            {
                const auto& row = Rows()[selected]; Text(200, 115, 225, row.Name, true, RT3_SORT_CENTER);
                Text(205, 295, 220, OptionCaption(selected), false);
                if (static_cast<size_t>(selected) < previews.size() && previews[selected])
                {
                    const auto* item = previews[selected];
                    Text(205, 326, 220, Label("Durability") + L": " + std::to_wstring(item->Durability), false);
                    Text(205, 351, 220, Label("ItemDefense") + L": " + std::to_wstring(item->Defense), false);
                    Text(205, 378, 220, Label("Strength") + L": " + std::to_wstring(item->RequireStrength), false);
                    Text(205, 405, 220, Label("Agility") + L": " + std::to_wstring(item->RequireDexterity), false);
                }
                Button(218, 474, 191, 35, Label("ChooseInventory"));
            }
            Text(458, 109, 174, Label("Currency"), false);
            for (size_t index = 0; index < Data.Currencies.size(); ++index)
            {
                const int value = Data.Currencies[index], y = 143 + static_cast<int>(index) * 32;
                widgets.Radio(X(461),Y(y),W(22),H(22),value==currency);
                if(value<=1)widgets.Coin(X(490),Y(y-1),W(23),H(25)); Text(519, y + 5, 122, CurrencyName(value), false);
            }
            Text(681, 164, 130, Label("StartingBid"), false); skin.Outline(X(676), Y(184), W(133), H(40), true);
            Text(681,245,130,Label("BuyNowShort"),false); skin.Outline(X(676), Y(264), W(133), H(40), true);
            priceInput->SetState(UISTATE_NORMAL); buyInput->SetState(UISTATE_NORMAL); priceInput->Render(); buyInput->Render();
            Text(681, 328, 130, Label("Duration"), false);
            Button(676, 351, 133, 40, Data.Durations.empty() ? L"" : Data.Durations[duration]%1440==0 ? std::to_wstring(Data.Durations[duration]/1440)+L" dias" : Data.Durations[duration]%60==0 ? std::to_wstring(Data.Durations[duration]/60)+L" horas" : std::to_wstring(Data.Durations[duration])+L" min");
            Text(659, 437, 157, Label("PublicationFeeZen"), false); Money(659,816,473,Data.Fee,0);
            Button(667, 508, 150, 43, Label("Publish"), selected >= 0 && Data.Enabled && !pending);
        }

        void Confirmation()
        {
            if (!confirmationAction) return;
            skin.Fill(X(254), Y(211), W(430), H(194)); skin.Outline(X(254), Y(211), W(430), H(194), true);
            Text(282, 237, 370, confirmation.Name, true, RT3_SORT_CENTER);
            Text(282, 268, 370, CurrencyName(confirmation.Currency) + L" " + std::to_wstring(confirmationAction == 3 ? confirmation.BuyNow : confirmation.NextBid), false, RT3_SORT_CENTER);
            if (confirmationAction == 2)
            {
                priceInput->SetPosition(X(378), Y(302)); priceInput->SetSize(W(169), H(24)); priceInput->SetState(UISTATE_NORMAL); priceInput->Render();
            }
            Button(282, 356, 165, 32, Label(confirmationAction == 2 ? "Bid" : confirmationAction == 3 ? "Buy" : "Confirm"), !pending);
            Button(486, 356, 165, 32, Label("Cancel"));
        }

        void OpenConfirmation(const Row& row, BYTE action)
        {
            confirmation = row; confirmationAction = action;
            if (action == 2) priceInput->SetText(std::to_wstring(row.NextBid).c_str());
            search->SetState(UISTATE_HIDE);
        }
    }

    void Load()
    {
        skin.Load(); frame.Load(); widgets.Load();
        titleFont=CreateFontW(32,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
        search = std::make_unique<CUITextInputBox>(); priceInput = std::make_unique<CUITextInputBox>(); buyInput = std::make_unique<CUITextInputBox>();
        search->Init(g_hWnd,185,18,30);
        SetWindowLongPtrW(search->GetHandle(),GWL_STYLE,GetWindowLongPtrW(search->GetHandle(),GWL_STYLE)|ES_CENTER); priceInput->Init(g_hWnd, 120, 18, 10); buyInput->Init(g_hWnd, 120, 18, 10);
        for (auto* input : { search.get(), priceInput.get(), buyInput.get() }) { input->SetTextColor(255, 255, 255, 255); input->SetBackColor(0, 0, 0, 0); input->SetState(UISTATE_HIDE); }
        priceInput->SetText(L"0"); buyInput->SetText(L"0"); Layout();
    }

    void Close()
    {
        visible = pending = filterOpen = false; confirmationAction = pendingAction = 0;
        for (auto* input : { search.get(), priceInput.get(), buyInput.get() }) if (input) input->SetState(UISTATE_HIDE);
        UI::OverlayInput::Capture();
    }

    void Unload() { Close(); ClearPreviews(); skin.Unload(); frame.Unload(); widgets.Unload(); if(titleFont)DeleteObject(titleFont);titleFont=nullptr; search.reset(); priceInput.reset(); buyInput.reset(); observed = 0; Network::Paris::Auctions::Clear(); }
    bool IsOpen() { return visible; }
    bool HasInputFocus() { return visible && (search->HaveFocus() || priceInput->HaveFocus() || buyInput->HaveFocus()); }
    void Toggle()
    {
        if (visible) { Close(); return; }
        visible = true; pending = false; tab = page = subTab = inventoryPage = 0; selected = -1; selectedSource.clear(); picker = true; observed = 0; Layout(); Send();
        UI::OverlayInput::Capture();
    }

    void Render()
    {
        if (!visible) return;
        Layout(); Synchronize();
        if (pending && timeGetTime() - sent > 5000) { pending = false; Send(); }
        if (!pending && !confirmationAction && (tab != 1 || !Matches()) && timeGetTime() - lastPoll > 3000) Send();
        skin.Frame(X(0), Y(0), W(Width), H(Height));
        const char* titles[] = { "AuctionTitle", "Publish", "MyOffers", "Deliveries", "MyAuctions" };
        skin.Brand(X(30),Y(17),W(264),H(58));
        skin.Separator(X(26),Y(84),W(802)); skin.Outline(X(24),Y(87),W(150),H(443));
        skin.Heading(tab,X(305),Y(14),W(450),H(64)); skin.Close(X(796), Y(22), W(28), H(28));
        for (int navigation = 0; navigation < 5; ++navigation) Button(35, 95 + navigation * 55, 134, 45, Label(titles[navigation]), true, tab == navigation);
        if (tab == 1) Publishing(); else Table();
        if (!Matches()) Text(197,390,625,timeGetTime()-sent>5000?L"El servidor no responde. Vuelve a abrir Subastas.":Label("Loading"),false,RT3_SORT_CENTER);
        if (pending) Text(197, 518, 625, Label("Pending"), false, RT3_SORT_CENTER);
        else if (Matches() && !Data.Enabled) Text(197, 518, 625, Label("Disabled"), false, RT3_SORT_CENTER);
        else if (Matches() && Data.Decision > 0) { const auto key = "AuctionError" + std::to_string(Data.Decision); Text(197, 518, 625, Label(key.c_str()), false, RT3_SORT_CENTER); }
        FilterMenu();
        Confirmation();
        Tooltip();
    }

    void Render3D()
    {
        if (!visible || !Matches() || observed!=Received) return;
        int hovered = -1;
        for (size_t index = 0; index < Data.Rows.size(); ++index)
        {
            const auto& row = Data.Rows[index]; if (index >= previews.size() || !previews[index]) continue;
            int x = 199, y = RowY + static_cast<int>(index) * RowHeight + 5, w = 39, h = 40;
            if (tab == 1)
            {
                if (!picker && selected >= 0) { if (index != static_cast<size_t>(selected)) continue; x = 215; y = 147; w = 195; h = 137; }
                else { const int tile = static_cast<int>(index) - inventoryPage * InventoryPageSize; if (tile < 0 || tile >= InventoryPageSize) continue; x = 204 + tile % 4 * 55; y = 160 + tile / 4 * 75; w = 43; h = 59; }
            }
            const float itemScale = std::min(.58f, std::min(W(w) / (24.f * std::max(1, int(ItemAttribute[row.Code].Width))),
                H(h) / (24.f * std::max(1, int(ItemAttribute[row.Code].Height)))));
            RenderMarketPreviewItem3D(X(x), Y(y), W(w), H(h), row.Code, previews[index]->Level, previews[index]->ExcellentFlags, previews[index]->AncientDiscriminator, itemScale);
            if (!confirmationAction && Hit(x, y, w, h)) hovered = static_cast<int>(index);
            if(!filterOpen && tab!=1 && (tab!=3 || subTab==1) && row.Currency>1)
            {
                const auto cell=MoneyCell();const int left=MoneyLeft(cell.first,cell.second,tab==2||tab==3?row.Amount:row.Price);
                RenderMarketPreviewItem3D(X(left),Y(RowY+static_cast<int>(index)*RowHeight+16),W(18),H(20),row.Currency==3?12*512+15:row.Currency==4?14*512+13:row.Currency==6?14*512+16:row.Currency==7?14*512+22:14*512+14,0,0,0,.4f);
            }
        }
        if (tab == 1) for (size_t index = 0; index < Data.Currencies.size(); ++index)
        {
            const int value = Data.Currencies[index];
            const int code = value == 3 ? 12 * 512 + 15 : value == 0 ? 14 * 512 + 15 : value == 4 || value == 2 ? 14 * 512 + 13
                : value == 6 ? 14 * 512 + 16 : value == 7 ? 14 * 512 + 22 : 14 * 512 + 14;
            if(value>1) RenderMarketPreviewItem3D(X(490),Y(142+static_cast<int>(index)*32),W(23),H(25),code,0,0,0,.4f);
        }

    }

    bool Mouse()
    {
        if (!visible) return false;
        UI::OverlayInput::Scope owner; Layout();
        if (tab == 0 && !confirmationAction) search->DoAction();
        if (tab == 1 || confirmationAction == 2) priceInput->DoAction();
        if (tab == 1 && !confirmationAction) buyInput->DoAction();
        if (!SEASON3B::IsPress(VK_LBUTTON)) return true;
        if (Hit(796, 22, 28, 28)) { Close(); return true; }
        if (confirmationAction)
        {
            if (Hit(486, 356, 165, 32)) { confirmationAction = 0; priceInput->SetState(UISTATE_HIDE); }
            else if (Hit(282, 356, 165, 32) && !pending)
            {
                const int price = confirmationAction == 2 ? Number(*priceInput) : confirmation.BuyNow;
                if (price >= 0) { Send(confirmationAction, &confirmation, price); confirmationAction = 0; priceInput->SetState(UISTATE_HIDE); }
            }
            else if (confirmationAction == 2 && Hit(378, 302, 169, 24)) priceInput->GiveFocus();
            return true;
        }
        for (int navigation = 0; navigation < 5; ++navigation)
            if (Hit(35, 95 + navigation * 55, 134, 45))
            {
                for (auto* input : { search.get(), priceInput.get(), buyInput.get() }) input->SetState(UISTATE_HIDE);
                filterOpen=false;tab = navigation;group=255;query.clear();search->SetText(L""); page = subTab = inventoryPage = 0; selected = -1; selectedSource.clear(); picker = true; Send(); return true;
            }
        if(!Matches() || pending)return true;
        if (tab == 0)
        {
            if(filterOpen)
            {
                for(int option=0;option<8;++option)if(Hit(FilterX,FilterY+FilterHeight+option*OptionHeight,FilterWidth,OptionHeight))
                {group=FilterCodes[option];page=0;filterOpen=false;Send();return true;}
                filterOpen=false;
                return true;
            }
            if (Hit(220,106,185,20)) search->GiveFocus();
            if(Hit(FilterX,FilterY,FilterWidth,FilterHeight)){filterOpen=!filterOpen;return true;}
            if (Hit(720,101,105,30)) { wchar_t text[31]{}; search->GetText(text, 31); query = text; page = 0; Send(); return true; }
        }
        if (tab > 1)
        {
            if (Hit(190, 95, 200, 38) || Hit(400, 95, 200, 38)) { subTab = Hit(190, 95, 200, 38) ? 0 : 1; page = 0; Send(); return true; }
        }
        if (tab != 1)
        {
            if (Hit(392, 536, 38, 27) && page > 0) { --page; Send(); return true; }
            if (Hit(574, 536, 38, 27) && Data.More && page < 100) { ++page; Send(); return true; }
            for (size_t index = 0; index < Data.Rows.size(); ++index)
            {
                const auto& row = Data.Rows[index]; const int y = RowY + static_cast<int>(index) * RowHeight;
                if (!Hit(190, y, 635, RowHeight)) continue;
                if (tab == 0 && Data.Enabled && !row.Owned) OpenConfirmation(row, Hit(725, y + 7, 95, 33) && row.BuyNow > 0 ? 3 : 2);
                else if (tab == 3 && Hit(725, y + 7, 95, 33)) Send(5, &row);
                else if (tab == 4 && subTab == 0 && row.CanCancel && Hit(725, y + 7, 95, 33)) OpenConfirmation(row, 4);
                break;
            }
            return true;
        }
        if (picker || selected < 0)
        {
            for (int tile = 0; tile < InventoryPageSize; ++tile)
                if (Hit(201 + tile % 4 * 55, 157 + tile / 4 * 75, 48, 65) && inventoryPage * InventoryPageSize + tile < static_cast<int>(Data.Rows.size()))
                { selected = inventoryPage * InventoryPageSize + tile; selectedSource = Data.Rows[selected].SourceId; picker = false; return true; }
            if (Hit(205, 476, 50, 30) && inventoryPage > 0) --inventoryPage;
            if (Hit(370, 476, 50, 30) && (inventoryPage + 1) * InventoryPageSize < static_cast<int>(Data.Rows.size())) ++inventoryPage;
        }
        else if (Hit(218, 474, 191, 35)) picker = true;
        for (size_t index = 0; index < Data.Currencies.size(); ++index) if (Hit(461, 143 + static_cast<int>(index) * 32, 180, 25)) currency = Data.Currencies[index];
        if (Hit(682, 191, 110, 25)) priceInput->GiveFocus();
        if (Hit(682, 271, 110, 25)) buyInput->GiveFocus();
        if (Hit(676, 351, 133, 40) && !Data.Durations.empty()) duration = (duration + 1) % Data.Durations.size();
        if (Hit(667, 508, 150, 43) && selected >= 0 && Data.Enabled)
        {
            const int price = Number(*priceInput), buy = Number(*buyInput);
            if (price > 0 && buy >= 0) Send(1, &Data.Rows[selected], price, buy);
        }
        return true;
    }
}
