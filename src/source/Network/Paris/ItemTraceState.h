#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include "../../../ThirdParty/json.hpp"

namespace ParisItemTrace {
constexpr unsigned RequestInterval = 250, CacheLifetime = 5000;
constexpr std::size_t RequestBytes = 45, MaximumTextBytes = 400;
struct Binding { std::uint32_t Key; int Slot, Type, Level; std::string Id; };
struct Origin {
    bool Known=false;
    std::string Kind, Source, Map, Player, Date;
    int X=-1, Y=-1;
};
struct Cached { Origin Value; unsigned Received=0; bool Enabled=false; };
struct Line { std::string Text; int Color=0; };

class State {
    std::map<std::uint32_t,Binding> bindings;
    std::map<std::uint32_t,Cached> records;
    unsigned sequence=0,lastRequest=0,pendingRequest=0;
    Binding pending{};
    static std::string Text(const nlohmann::json& j,const char* name) {
        const auto it=j.find(name);
        if(it==j.end()||it->is_null())return {};
        auto value=it->get<std::string>();
        if(value.size()>MaximumTextBytes)throw std::invalid_argument("Item trace string too long");
        for(unsigned char ch:value)if(ch<32)throw std::invalid_argument("Invalid item trace text");
        return value;
    }
public:
    void Reset(){bindings.clear();records.clear();pendingRequest=0;lastRequest=0;}
    void Synchronize(const std::vector<Binding>& items) {
        std::map<std::uint32_t,Binding> next;
        for(const auto& item:items) {
            if(item.Id.size()!=32||item.Slot<0||item.Slot>255||item.Type<0||item.Type>=8192||item.Level<0||item.Level>255)continue;
            auto copy=item;
            auto previous=bindings.find(item.Key);
            if(item.Id==std::string(32,'0')&&previous!=bindings.end()&&previous->second.Type==item.Type&&previous->second.Level==item.Level)
                copy.Id=previous->second.Id;
            next.emplace(item.Key,std::move(copy));
        }
        for(auto it=records.begin();it!=records.end();) {
            auto a=bindings.find(it->first),b=next.find(it->first);
            if(a==bindings.end()||b==next.end()||a->second.Id!=b->second.Id)it=records.erase(it);else ++it;
        }
        bindings=std::move(next);
    }
    bool Prepare(std::uint32_t key,int type,int level,unsigned now,std::array<unsigned char,RequestBytes>& packet) {
        auto it=bindings.find(key);
        if(it==bindings.end()||it->second.Type!=type||it->second.Level!=level)return false;
        auto cached=records.find(key);
        if(cached!=records.end()&&now-cached->second.Received<CacheLifetime)return false;
        if(lastRequest&&now-lastRequest<RequestInterval)return false;
        lastRequest=now;pending=it->second;pendingRequest=++sequence;
        packet={0xC1,static_cast<unsigned char>(RequestBytes),0xFA,0xDC,1};
        packet[5]=static_cast<unsigned char>(pending.Slot);
        packet[6]=static_cast<unsigned char>(type);packet[7]=static_cast<unsigned char>(type>>8);
        packet[8]=static_cast<unsigned char>(level);
        for(int i=0;i<4;++i)packet[9+i]=static_cast<unsigned char>(pendingRequest>>(8*i));
        std::memcpy(packet.data()+13,pending.Id.data(),32);
        return true;
    }
    bool Receive(const nlohmann::json& data,unsigned now) {
        if(!data.value("itemTrace",false))return false;
        const auto responseId=Text(data,"id");
        if(!pendingRequest||data.at("request").get<unsigned>()!=pendingRequest||responseId.size()!=32)return false;
        for(char ch:responseId)if(!((ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')))return false;
        if(pending.Id!=std::string(32,'0')&&responseId!=pending.Id)return false;
        auto current=bindings.find(pending.Key);
        if(current==bindings.end()||current->second.Id!=pending.Id||current->second.Slot!=pending.Slot)return false;
        Cached result;result.Enabled=data.at("enabled").get<bool>();result.Received=now;
        const auto& source=data.at("trace");
        if(result.Enabled&&!source.is_null()) {
            auto& value=result.Value;value.Kind=Text(source,"Kind");
            if(value.Kind!="monster"&&value.Kind!="box"&&value.Kind!="make"&&value.Kind!="editor"&&value.Kind!="npc"&&value.Kind!="craft")return false;
            value.Source=Text(source,"Source");value.Map=Text(source,"Map");value.Player=Text(source,"Player");
            value.Date=Text(source,"CreatedUtc");
            if(source.contains("X")&&!source["X"].is_null())value.X=source["X"].get<int>();
            if(source.contains("Y")&&!source["Y"].is_null())value.Y=source["Y"].get<int>();
            if(value.X < -1||value.X>255||value.Y < -1||value.Y>255)return false;
            if(value.Date.rfind("0001-",0)==0)value.Date.clear();
            value.Known=true;
        }
        current->second.Id=responseId;
        records[pending.Key]=std::move(result);pendingRequest=0;return true;
    }
    const Cached* Find(std::uint32_t key,int type,int level) const {
        auto binding=bindings.find(key);auto record=records.find(key);
        if(binding==bindings.end()||record==records.end()||binding->second.Type!=type||binding->second.Level!=level)return nullptr;
        return &record->second;
    }
};

inline std::vector<Line> Lines(const Cached* record,int language) {
    // Language indexes: English, Portuguese, Spanish, Vietnamese.
    if(!record||!record->Enabled)return {};
    if(language<0||language>3)language=0;
    constexpr const char* headers[]={"[ Item Trace Information ]","[ Origem do item ]","[ Item Trace Information ]","[ Nguon goc vat pham ]"};
    constexpr const char* unknown[]={"Origin: Unknown","Origem: Desconhecida","Origen: Desconocido","Nguon: Khong ro"};
    constexpr const char* source[]={"Origin: ","Origem: ","Origen: ","Nguon: "};
    constexpr const char* mob[]={"Monster: ","Monstro: ","Monstruo: ","Quai: "};
    constexpr const char* place[]={"Dropped in: ","Caiu em: ","Cayó en: ","Roi tai: "};
    constexpr const char* created[]={"Created in: ","Criado em: ","Creado en: ","Tao tai: "};
    constexpr const char* recipient[]={"Dropped for: ","Caiu para: ","Le cayó a: ","Nguoi nhan: "};
    constexpr const char* author[]={"Created by: ","Criado por: ","Creado por: ","Nguoi tao: "};
    constexpr const char* vendor[]={"NPC: ","NPC: ","NPC: ","NPC: "};
    constexpr const char* purchased[]={"Bought in: ","Comprado em: ","Comprado en: ","Mua tai: "};
    constexpr const char* buyer[]={"Bought by: ","Comprado por: ","Comprado por: ","Nguoi mua: "};
    constexpr const char* crafted[]={"Crafted in: ","Combinado em: ","Combinado en: ","Che tao tai: "};
    constexpr const char* crafter[]={"Crafted by: ","Combinado por: ","Combinado por: ","Nguoi che tao: "};
    constexpr const char* chaos[]={"Chaos Machine","Maquina Chaos","Máquina Chaos","May Chaos"};
    constexpr const char* date[]={"Date: ","Data: ","Fecha: ","Ngay: "};
    std::vector<Line> result{{headers[language],0}};
    const auto& value=record->Value;
    if(!value.Known){result.push_back({unknown[language],1});return result;}
    const bool administrative=value.Kind=="make"||value.Kind=="editor";
    std::string origin=value.Kind=="npc" ? std::string(vendor[language])+value.Source : value.Kind=="monster" ? std::string(mob[language])+value.Source :
        std::string(source[language])+(value.Kind=="craft"&&value.Source=="Chaos Machine"?chaos[language]:value.Kind=="editor"?"Editor":value.Kind=="make"?"Make (Admin)":value.Source);
    result.push_back({origin,1});
    if(!value.Map.empty()) {
        std::string location=value.Map;
        if(value.X>=0&&value.Y>=0)location+=" ("+std::to_string(value.X)+", "+std::to_string(value.Y)+")";
        result.push_back({std::string(administrative?created[language]:value.Kind=="craft"?crafted[language]:value.Kind=="npc"?purchased[language]:place[language])+location,0});
    }
    if(!value.Player.empty())result.push_back({std::string(administrative?author[language]:value.Kind=="craft"?crafter[language]:value.Kind=="npc"?buyer[language]:recipient[language])+value.Player,2});
    if(value.Date.size()>=19) {
        const auto& d=value.Date;
        result.push_back({std::string(date[language])+d.substr(8,2)+"/"+d.substr(5,2)+"/"+d.substr(0,4)+" "+d.substr(11,8)+" UTC",3});
    }
    return result;
}
} // namespace ParisItemTrace
