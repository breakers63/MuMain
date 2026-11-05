#pragma once
#include <array>
#include <string>
#include <sstream>
namespace Network::Paris::Entry
{
struct Row { std::wstring Key, Label; bool Enabled=false; };
inline std::array<Row,7> Rows{};
inline std::wstring Title;
inline bool Active=false, Complete=false;
inline DWORD Received=0;
inline std::wstring Mode,Phase,Ticket,Requirement;
inline int RewardCount=3,RewardPage=0;
inline int Clans=0,MaxClans=0,MinClans=0,MinMembers=0,Laps=0,RequireMount=0;
inline std::array<std::wstring,32> Rewards{};
inline int Participants=0,Maximum=0,Minimum=0,Seconds=0,Duration=0,Stages=0,MinLevel=0,MaxLevel=0,MinReset=0,MaxReset=0;
inline bool Rich(){return Active&&(Mode==L"sky"||Mode==L"tvt"||Mode==L"generals"||Mode==L"guild-battle"||Mode==L"race");}
inline std::wstring Clock(){int seconds=std::max(0,Seconds-static_cast<int>((timeGetTime()-Received)/1000));wchar_t text[32];swprintf_s(text,L"%02d:%02d",seconds/60,seconds%60);return text;}
inline std::wstring Status(){return Phase==L"registration"?L"INSCRIPCIONES ABIERTAS":Phase==L"running"?L"EVENTO EN CURSO":Phase==L"prepare"?L"PREPARANDO EVENTO":L"INSCRIPCIONES CERRADAS";}
inline std::wstring Limits(){return MinLevel==0&&MaxLevel>=1000&&MinReset==0&&MaxReset>=100000?L"Sin limite de nivel ni resets":L"Nivel "+std::to_wstring(MinLevel)+L" - "+std::to_wstring(MaxLevel)+L" | Reset "+std::to_wstring(MinReset)+L" - "+std::to_wstring(MaxReset);}

inline bool Receive(const wchar_t* text)
{
    const std::wstring value=text?text:L"";
    if(value.rfind(L"[PARIS-ENTRY]",0)!=0)return false;
    const auto body=value.substr(13);
    if(body.rfind(L"B|",0)==0){Rows={};RewardCount=3;RewardPage=0;Mode.clear();Phase.clear();Ticket.clear();Requirement.clear();Clans=MaxClans=MinClans=MinMembers=Laps=RequireMount=0;Rewards={};Title=body.substr(2,70);Active=false;Complete=false;Received=timeGetTime();return true;}
    if(body.rfind(L"M|",0)==0)
    {
        std::wistringstream stream(body.substr(2));std::array<std::wstring,12> f{};
        for(auto& field:f)if(!std::getline(stream,field,L'|'))return true;
        int n[11]{};int at=0;
        for(int i=1;i<12;i++)if(i!=6){if(f[i].empty()||f[i].size()>6)return true;for(auto c:f[i])if(c<L'0'||c>L'9')return true;try{n[at++]=std::stoi(f[i]);}catch(...){return true;}}
        if((f[0]!=L"sky"&&f[0]!=L"tvt"&&f[0]!=L"generals"&&f[0]!=L"guild-battle"&&f[0]!=L"race")||n[0]>500||n[1]>500||n[2]>500||n[3]>86400||n[4]>86400||n[5]>30)return true;
        Mode=f[0];Phase=f[6];Participants=n[0];Maximum=n[1];Minimum=n[2];Seconds=n[3];Duration=n[4];Stages=n[5];MinLevel=n[6];MaxLevel=n[7];MinReset=n[8];MaxReset=n[9];Received=timeGetTime();return true;
    }
    if(body.rfind(L"G|",0)==0)
    {
        std::wistringstream stream(body.substr(2));std::array<int,6> values{};std::wstring field;
        for(auto& value:values){if(!std::getline(stream,field,L'|')||field.empty()||field.size()>3)return true;for(auto c:field)if(c<L'0'||c>L'9')return true;value=std::stoi(field);}
        if(values[0]>500||values[1]>500||values[2]>20||values[3]>50||values[4]>20||values[5]>1)return true;
        Clans=values[0];MaxClans=values[1];MinClans=values[2];MinMembers=values[3];Laps=values[4];RequireMount=values[5];return true;
    }
    if(body.rfind(L"U|",0)==0){Requirement=body.substr(2,100);return true;}
    if(body.rfind(L"T|",0)==0){Ticket=body.substr(2,100);return true;}
    if(body.rfind(L"W|",0)==0){auto count=body.substr(2);if(count.empty()||count.size()>2)return true;for(auto c:count)if(c<L'0'||c>L'9')return true;int n=std::stoi(count);if(n>32)return true;RewardCount=n;RewardPage=std::min(RewardPage,std::max(0,(n-1)/2));for(int i=n;i<32;i++)Rewards[i].clear();return true;}
    if(body.rfind(L"P|",0)==0){auto separator=body.find(L'|',2);if(separator==std::wstring::npos||separator<3||separator>4)return true;auto index=body.substr(2,separator-2);for(auto c:index)if(c<L'0'||c>L'9')return true;int i=std::stoi(index);if(i<32)Rewards[i]=body.substr(separator+1,100);return true;}

    if(timeGetTime()-Received>5000)return true;
    if(body==L"E"){Active=true;Complete=true;return true;}
    if(body.rfind(L"R|",0)!=0)return true;
    std::wistringstream input(body.substr(2));std::wstring idx,enabled,key,label;
    if(!std::getline(input,idx,L'|')||!std::getline(input,enabled,L'|')||!std::getline(input,key,L'|')||!std::getline(input,label))return true;
    if(idx.size()!=1||idx[0]<L'0'||idx[0]>L'6'||key.empty()||key.size()>32||label.size()>100)return true;
    for(auto ch:key)if(!((ch>=L'a'&&ch<=L'z')||(ch>=L'0'&&ch<=L'9')||ch==L'-'))return true;
    Rows[idx[0]-L'0']={key,label,enabled==L"1"};return true;
}
inline bool Available(int i){return Active&&i>=0&&i<7&&Rows[i].Enabled&&!Rows[i].Key.empty();}
}
