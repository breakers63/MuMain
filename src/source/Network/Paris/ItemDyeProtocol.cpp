#include "stdafx.h"
#include "ItemDyeProtocol.h"
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
namespace { unsigned sequence=0, pendingRequest=0, latestAccepted=0, pendingSince=0; }
void Reset(){Inventory.clear();Received=0;Pending=false;Result=255;RequireReagents=true;Render::ItemDye::Clear();}
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
    if(action!=0){Pending=true;pendingRequest=request;pendingSince=timeGetTime();}
}
std::wstring Wide(const std::string& value)
{
    const int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
    if(length<1||length>100)return L"";
    std::wstring result(length,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),length);return result;
}
}
void Request(){if(Pending&&timeGetTime()-pendingSince>10000){Pending=false;Result=255;}Send(0,nullptr,nullptr,0);}
void Apply(const Entry& item,const int* reagentSlots,int color){Send(1,&item,reagentSlots,color);}
void RestoreOriginal(const Entry& item){const int slots[3]={0,0,0};Send(2,&item,slots,0);}
bool Receive(const BYTE* bytes,int size)
{
    // A delayed snapshot may arrive while returning to login and destroying inventory windows.
    if(!bytes||size<6||size>60006||bytes[5]!=1||SceneFlag!=MAIN_SCENE||!CharacterMachine||!g_pMyInventory)return false;
    try
    {
        const auto data=nlohmann::json::parse(bytes+6,bytes+size);
        const unsigned responseRequest=data.at("request");
        if(static_cast<int>(responseRequest-latestAccepted)<=0)return false;
        if(!data.at("inventory").is_array()||data.at("inventory").size()>256||data.at("equipment").size()>100)return false;
        std::vector<Entry> next;
        for(const auto& row:data.at("inventory"))
        {
            Entry item;item.Id=row.at("id");item.Name=Wide(row.at("name"));item.Slot=row.at("slot");item.Type=row.at("type");item.Level=row.at("level");item.Color=row.at("color");item.Eligible=row.at("eligible");
            if(item.Id.size()!=32||item.Slot<0||item.Slot>255||item.Type<0||item.Type>=MAX_ITEM||item.Level<0||item.Level>15||item.Color < -1||item.Color>0xFFFFFF)return false;
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
            const int key=row.at("key");const int index=FindCharacterIndex(key);
            if(index<0||index>=MAX_CHARACTERS_CLIENT)continue;
            auto* character=&CharactersClient[index];Render::ItemDye::Equipped equipped;equipped.Character=character;equipped.Key=character->Key;
            if(row.at("items").size()>12)return false;
            for(const auto& part:row.at("items"))
            {
                const int type=part.at("type"),color=part.at("color");
                if(type<0||type>=ITEM_WING||color<0||color>0xFFFFFF)return false;
                equipped.Colors[MODEL_ITEM+type]=color;
            }
            worldColors[&character->Object]=std::move(equipped);
        }
        // The owner's live inventory is authoritative even before the appearance cache rebuilds.
        if(Hero)
        {
            Render::ItemDye::Equipped own;own.Character=Hero;own.Key=Hero->Key;
            for(const auto& row:next)if(row.Slot<12&&row.Color>=0)own.Colors[MODEL_ITEM+row.Type]=row.Color;
            worldColors[&Hero->Object]=std::move(own);
        }
        const unsigned request=data.at("request");const int result=data.at("result");
        if(Pending&&request==pendingRequest){Pending=false;Result=result;
            g_ErrorReport.Write(L"RGB apply acknowledgement: result=%d inventoryBindings=%u\r\n",result,static_cast<unsigned>(inventoryColors.size()));}
        PotionsPerColor=std::clamp(data.value("potionsPerColor",1),1,20);RequireReagents=data.value("requireReagents",true);latestAccepted=responseRequest;Inventory=std::move(next);Render::ItemDye::Inventory=std::move(inventoryColors);Render::ItemDye::World=std::move(worldColors);Received=timeGetTime();++Revision;return true;
    }
    catch(const std::exception&){return false;}
}
}
