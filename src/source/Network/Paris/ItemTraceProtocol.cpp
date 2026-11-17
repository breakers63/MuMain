#include "stdafx.h"
#include "ItemTraceProtocol.h"
#include "ItemTraceState.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/MyInventory.h"

namespace Network::Paris::ItemTrace {
namespace {
ParisItemTrace::State state;
unsigned synchronizedAt=0;
constexpr unsigned SynchronizationInterval=250;
constexpr int MaximumInventorySlots=256, EquipmentSlots=12, ResponseHeaderBytes=6, MaximumResponseBytes=4096;

void Synchronize(unsigned now) {
    if(synchronizedAt&&now-synchronizedAt<SynchronizationInterval)return;
    synchronizedAt=now;
    std::vector<ParisItemTrace::Binding> bindings;
    for(int slot=0;slot<MaximumInventorySlots;++slot) {
        ITEM* item=slot<EquipmentSlots?&CharacterMachine->Equipment[slot]:g_pMyInventory->FindItem(slot);
        if(item&&item->Type>=0&&item->Type<8192)
            bindings.push_back({item->Key,slot,item->Type,item->Level,std::string(32,'0')});
    }
    state.Synchronize(bindings);
}
}

void Reset(){state.Reset();synchronizedAt=0;}

const ParisItemTrace::Cached* Query(const tagITEM& item) {
    if(SceneFlag!=MAIN_SCENE||!SocketClient||!SocketClient->IsConnected()||!CharacterMachine||!g_pMyInventory)return nullptr;
    const unsigned now=timeGetTime();Synchronize(now);
    std::array<unsigned char,ParisItemTrace::RequestBytes> packet{};
    if(state.Prepare(item.Key,item.Type,item.Level,now,packet))
        SocketClient->Send(packet.data(),static_cast<int>(packet.size()));
    return state.Find(item.Key,item.Type,item.Level);
}

bool Receive(const unsigned char* packet,int size) {
    if(!packet||size<ResponseHeaderBytes||size>MaximumResponseBytes||packet[0]!=0xC2
        ||((packet[1]<<8)|packet[2])!=size||packet[3]!=0xFA||packet[4]!=0xDD||packet[5]!=1||SceneFlag!=MAIN_SCENE)return false;
    try {
        return state.Receive(nlohmann::json::parse(packet+ResponseHeaderBytes,packet+size),timeGetTime());
    }catch(const std::exception&){return false;}
}
}
