#pragma once
#include "ParisBattlePassProtocol.h"
namespace Network::Paris::BattlePass {
inline constexpr int MissionHeaderSize=60,MissionRowSize=320,MaximumMissions=64;
struct Mission {int Id=0,Progress=0,Target=1,Points=0;bool Weekly=false,Claimed=false;std::wstring Title,Objective;};
struct MissionSnapshot {bool Active=false;std::array<BYTE,16> Season{};std::uint64_t Token=0;std::int64_t ServerTime=0,DailyReset=0,WeeklyReset=0;std::vector<Mission> Missions;};
inline std::optional<MissionSnapshot> Missions;
inline DWORD MissionsReceived = 0;
inline std::optional<MissionSnapshot> DecodeMissions(std::span<const BYTE> packet) {
 if(packet.size()<MissionHeaderSize||packet.size()>MissionHeaderSize+MaximumMissions*MissionRowSize)return std::nullopt;
 const BYTE* p=packet.data();const int count=U16(p+56);
 if(p[0]!=0xC2||((p[1]<<8)|p[2])!=packet.size()||p[3]!=0xFA||p[4]!=0xC9||p[5]!=1||p[6]>1||count>MaximumMissions||packet.size()!=MissionHeaderSize+count*MissionRowSize)return std::nullopt;
 MissionSnapshot s;s.Active=p[6]!=0;std::copy(p+8,p+24,s.Season.begin());s.Token=Number<std::uint64_t>(p+24);s.ServerTime=Number<std::int64_t>(p+32);s.DailyReset=Number<std::int64_t>(p+40);s.WeeklyReset=Number<std::int64_t>(p+48);
 if(!s.Token||s.ServerTime<0||s.DailyReset<=s.ServerTime||s.WeeklyReset<=s.ServerTime||s.DailyReset-s.ServerTime>86400||s.WeeklyReset-s.ServerTime>604800)return std::nullopt;
 for(int i=0;i<count;i++){const BYTE* row=p+MissionHeaderSize+i*MissionRowSize;Mission m;m.Id=U16(row);m.Weekly=(row[2]&1)!=0;m.Claimed=(row[2]&2)!=0;m.Progress=Number<int>(row+4);m.Target=Number<int>(row+8);m.Points=Number<int>(row+12);
 if(!m.Id||row[2]>3||m.Target<1||m.Target>10000000||m.Progress<0||m.Progress>m.Target||m.Points<1||m.Points>1000000||std::any_of(s.Missions.begin(),s.Missions.end(),[&](const Mission& other){return other.Id==m.Id;}))return std::nullopt;
 const BYTE* a=std::find(row+16,row+112,0);const BYTE* b=std::find(row+112,row+320,0);
 if(a==row+112||b==row+320||!Utf8(row+16,static_cast<int>(a-row-16),m.Title)||!Utf8(row+112,static_cast<int>(b-row-112),m.Objective))return std::nullopt;s.Missions.push_back(std::move(m));}
 return s;
}
}
