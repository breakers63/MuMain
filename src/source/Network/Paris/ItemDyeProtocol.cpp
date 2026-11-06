#include "stdafx.h"
#include "ItemDyeProtocol.h"
#include "Render/Models/NameRgbAppearance.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/MyInventory.h"
#include "Render/Models/ItemDyeAppearance.h"
#include "../../../ThirdParty/json.hpp"
#include <cstring>
namespace Network::Paris::ItemDye
{
std::vector<Entry> Inventory;
unsigned Received=0,Revision=0;
int Result=255;
bool Pending=false;
bool RequireReagents=true;
int PotionsPerColor=1;
namespace { unsigned sequence=0, pendingRequest=0, latestAccepted=0, pendingSince=0;
constexpr unsigned RefreshDelay = 650;
unsigned refreshScheduledAt = 0;
bool refreshScheduled = false;
}
void Reset(){Inventory.clear();Received=0;Pending=false;Result=255;RequireReagents=true;Render::ItemDye::Clear();Render::NameRgb::Styles.clear();Render::NameRgb::Own={};}
namespace {
void Send(BYTE action,const Entry* item,const int* slots,int color)
{
    if(!SocketClient || Pending)return;
    BYTE packet[50]={0xC1,50,0xFA,0xE8,1,action};
    const unsigned request=++sequence;std::memcpy(packet+14,&request,4);
    if(item)
    {
        if(item->Id.size()!=32 || !slots)return;
        packet[6]=static_cast<BYTE>(item->Slot);
        for(int i=0;i<3;++i){if(action==1&&RequireReagents&&(slots[i]<12||slots[i]>255))return;packet[7+i]=static_cast<BYTE>(slots[i]);}
        std::memcpy(packet+10,&color,4);std::memcpy(packet+18,item->Id.data(),32);
    }
    SocketClient->Send(packet,sizeof(packet));
    if(action==1||action==2){Pending=true;pendingRequest=request;pendingSince=timeGetTime();}
}
std::wstring Wide(const std::string& value)
{
    const int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
    if(length<1||length>100)return L"";
    std::wstring result(length,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),length);return result;
}
}
void Request(){if(Pending&&timeGetTime()-pendingSince>10000){Pending=false;Result=255;}Send(0,nullptr,nullptr,0);}
void ApplyName(int mode,int first,int last,int speed)
{
    if(!SocketClient||Pending||mode<0||mode>3||speed<1||speed>10)return;
    BYTE packet[50]={0xC1,50,0xFA,0xE8,1,4};
    packet[6]=static_cast<BYTE>(mode);packet[7]=static_cast<BYTE>(speed);
    std::memcpy(packet+10,&first,4);std::memcpy(packet+18,&last,4);
    const unsigned request=++sequence;std::memcpy(packet+14,&request,4);
    SocketClient->Send(packet,sizeof(packet));
    Pending=true;pendingRequest=request;pendingSince=timeGetTime();
}
void ScheduleRefresh()
{
    refreshScheduledAt = timeGetTime();
    refreshScheduled = true;
}

void UpdateRefresh()
{
    static unsigned lastPoll = 0;
    const unsigned now = timeGetTime();
    if (!SocketClient || (SceneFlag != MAIN_SCENE && SceneFlag != CHARACTER_SCENE)) return;
    const bool due = refreshScheduled && now - refreshScheduledAt >= RefreshDelay;
    if (!due && now - lastPoll < 1500) return;
    if (Pending && now - pendingSince > 10000) { Pending = false; Result = 255; }
    if (Pending) return;
    lastPoll = now;
    if (SceneFlag == CHARACTER_SCENE) Send(3, nullptr, nullptr, 0);
    else Request();
    refreshScheduled = false;
}

void Apply(const Entry& item,const int* reagentSlots,int color){Send(1,&item,reagentSlots,color);}
void RestoreOriginal(const Entry& item){const int slots[3]={0,0,0};Send(2,&item,slots,0);}
bool Receive(const BYTE* bytes,int size)
{
    // A delayed snapshot may arrive while returning to login and destroying inventory windows.
    if(!bytes||size<6||size>60006||bytes[5]!=1||(SceneFlag!=MAIN_SCENE&&SceneFlag!=CHARACTER_SCENE))return false;
    try
    {
        const auto data=nlohmann::json::parse(bytes+6,bytes+size);
        const bool selection = data.value("selection", false);
        if(data.value("appearanceOnly",false))
        {
            if(selection||!data.at("equipment").is_array()||data.at("equipment").size()>100||data.at("names").size()>100)return false;
            for(const auto& row:data.at("equipment"))
            {
                const auto name=Wide(row.at("name"));if(name.empty()||row.at("items").size()>12)return false;
                std::unordered_map<int,int> colors;
                for(const auto& item:row.at("items")) {
                    const int type=item.at("type"),color=item.at("color");
                    if(type<0||type>=ITEM_WING||color<0||color>0xFFFFFF)return false;
                    colors[MODEL_ITEM+type]=color;
                }
                Render::ItemDye::Remember(name.c_str(),colors);
                const int index=FindCharacterIndex(row.at("key").get<int>());
                if(index>=0&&index<MAX_CHARACTERS_CLIENT&&name==CharactersClient[index].ID)
                    Render::ItemDye::BindCharacter(&CharactersClient[index]);
            }
            for(const auto& row:data.at("names")) {
                Render::NameRgb::Style style{row.at("mode"),row.at("first"),row.at("last"),row.at("speed")};
                if(style.Mode<0||style.Mode>3||style.First<0||style.First>0xFFFFFF||style.Last<0||style.Last>0xFFFFFF||style.Speed<1||style.Speed>10)return false;
                Render::NameRgb::Styles[Wide(row.at("name"))]=style;
            }
            return true;
        }
        if (selection != (SceneFlag == CHARACTER_SCENE)) return false;
        if (!selection && (!CharacterMachine || !g_pMyInventory)) return false;
        const unsigned responseRequest=data.at("request");
        if(static_cast<int>(responseRequest-latestAccepted)<=0)return false;
        if(!data.at("inventory").is_array()||data.at("inventory").size()>256||data.at("equipment").size()>100)return false;
        std::vector<Entry> next;
        for(const auto& row:data.at("inventory"))
        {
            Entry item;item.Id=row.at("id");item.Name=Wide(row.at("name"));item.Slot=row.at("slot");item.Type=row.at("type");item.Level=row.at("level");item.Color=row.at("color");item.Eligible=row.at("eligible");
            if(item.Id.size()!=32||item.Slot<0||item.Slot>255||item.Type<0||item.Type>=MAX_ITEM||!IsSupportedInventoryLevel(item.Level,item.Eligible)||item.Color < -1||item.Color>0xFFFFFF)return false;
            next.push_back(std::move(item));
        }
        std::unordered_map<DWORD,int> inventoryColors;
        for(const auto& row:next)
        {
            ITEM* item=row.Slot<12?&CharacterMachine->Equipment[row.Slot]:g_pMyInventory->FindItem(row.Slot);
            if(item && item->Type==row.Type && row.Color>=0)inventoryColors[item->Key]=row.Color;
        }
        std::unordered_map<const OBJECT*,Render::ItemDye::Equipped> worldColors;
        for(const auto& row:data.at("equipment"))
        {
            const int key=row.at("key");const int index=selection ? key : FindCharacterIndex(key);
            if(index<0||index>=MAX_CHARACTERS_CLIENT)continue;
            auto* character=&CharactersClient[index];Render::ItemDye::Equipped equipped;equipped.Character=character;equipped.Key=character->Key;
            if(row.at("items").size()>12)return false;
            for(const auto& part:row.at("items"))
            {
                const int type=part.at("type"),color=part.at("color");
                if(type<0||type>=ITEM_WING||color<0||color>0xFFFFFF)return false;
                equipped.Colors[MODEL_ITEM+type]=color;
            }
            Render::ItemDye::Remember(character->ID,equipped.Colors);
            worldColors[&character->Object]=std::move(equipped);
        }
        // The owner's live inventory is authoritative even before the appearance cache rebuilds.
        if(Hero && !selection)
        {
            Render::ItemDye::Equipped own;own.Character=Hero;own.Key=Hero->Key;
            for(const auto& row:next)if(row.Slot<12&&row.Color>=0)own.Colors[MODEL_ITEM+row.Type]=row.Color;
            worldColors[&Hero->Object]=std::move(own);
        }
        Render::NameRgb::Map names;
        if(data.contains("names"))for(const auto& row:data.at("names")) {
            Render::NameRgb::Style style{row.at("mode"),row.at("first"),row.at("last"),row.at("speed")};
            if(style.Mode<0||style.Mode>3||style.First<0||style.First>0xFFFFFF||style.Last<0||style.Last>0xFFFFFF||style.Speed<1||style.Speed>10)return false;
            names[Wide(row.at("name"))]=style;
        }
        if(data.contains("names"))Render::NameRgb::Styles=std::move(names);
        if(Hero&&!selection) {
            const auto own=Render::NameRgb::Styles.find(Hero->ID);
            Render::NameRgb::Own=own==Render::NameRgb::Styles.end()?Render::NameRgb::Style{}:own->second;
        }
        const unsigned request=data.at("request");const int result=data.at("result");
        if(Pending&&request==pendingRequest){Pending=false;Result=result;
            g_ErrorReport.Write(L"RGB apply acknowledgement: result=%d inventoryBindings=%u\r\n",result,static_cast<unsigned>(inventoryColors.size()));}
        PotionsPerColor=std::clamp(data.value("potionsPerColor",1),1,20);RequireReagents=data.value("requireReagents",true);latestAccepted=responseRequest;Inventory=std::move(next);Render::ItemDye::Inventory=std::move(inventoryColors);Render::ItemDye::World=std::move(worldColors);Received=timeGetTime();++Revision;return true;
    }
    catch(const std::exception&){return false;}
}
}
