#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "MentorProtocol.h"
#include "Network/Server/WSclient.h"
#include "../../../ThirdParty/json.hpp"
#include <cstring>
#include <fstream>
#include <cmath>
namespace Network::Paris::Mentor
{
Snapshot Data;
std::vector<ActiveBuff> ActiveBuffs;
bool BuffActive=false;
DWORD Received=0; int BuffBonus=0; DWORD BuffReceived=0; std::wstring BuffCaption;
namespace {
std::wstring Wide(const std::string& text) { if(text.size()>1024) return {}; int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0); if(!count && !text.empty()) throw std::runtime_error("UTF8"); std::wstring value(count,L'\0'); if(count) MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),value.data(),count); return value; }
std::wstring ClassName(const nlohmann::json& row) {
 auto value=row.find("className");
 if(value==row.end())return {};
 if(value->is_string())return Wide(value->get<std::string>());
 // Compatibility with v35, which serialized LocalizedString instead of its text.
 if(value->is_object())return Wide(value->value("ValueInNeutralLanguage",std::string()));
 throw std::runtime_error("className");
}
std::vector<Entry> Entries(const nlohmann::json& rows) {
 std::vector<Entry> result; if(!rows.is_array() || rows.size()>6) throw std::runtime_error("history");
 for(const auto& row:rows){Entry e; e.Name=Wide(row.value("name",std::string()));e.Kind=Wide(row.value("kind",std::string()));e.Points=row.value("points",0);e.Time=row.value("time",0LL);
  if(row.contains("detail")&&!row["detail"].is_null()){const auto& d=row["detail"];e.HasDetail=true;e.MentorName=Wide(d.value("mentorName",std::string()));e.StudentName=Wide(d.value("noviceName",std::string()));e.State=Wide(d.value("state",std::string()));e.Seconds=static_cast<long long>(d.value("validSeconds",0.0));e.Awarded=d.value("awardedPoints",0);e.Created=d.value("created",0LL);e.Ended=d["graduatedAt"].is_null()?0:d["graduatedAt"].get<long long>();e.Resets=d.value("resets",0);e.Goal=d.value("goal",16);}
  result.push_back(e);
 }return result;
}
}
void Trace(const char* event, const char* detail)
{
 constexpr unsigned MaximumDiagnosticLines=64;
 static unsigned written=0;
 if(written++>=MaximumDiagnosticLines)return;
 std::ofstream log("ParisMentorStatus.log",std::ios::app);
 log<<timeGetTime()<<" "<<event<<" "<<detail<<'\n';
}
bool Receive(const BYTE* packet,int size)
{
 if(size<8 || size>60006 || packet[0]!=0xC2 || ((packet[1]<<8)|packet[2])!=size || packet[3]!=0xFA || packet[4]!=0xD3 || packet[5]!=1) return false;
 try {
  const auto json=nlohmann::json::parse(packet+6,packet+size);
  if(json.value("buffOnly",false)){int bonus=json.at("bonus");if(bonus<0||bonus>100000)return false;auto caption=Wide(json.at("text"));
   std::vector<ActiveBuff> buffs;
   if(json.contains("buffs")){
    if(!json.at("buffs").is_array()||json.at("buffs").size()>8)return false;
    for(const auto& item:json.at("buffs")){
     ActiveBuff buff;buff.Number=item.at("number");buff.Percent=item.at("percent");buff.Caption=Wide(item.at("text"));
     if(buff.Number<1||buff.Number>=200||!std::isfinite(buff.Percent)||buff.Percent<0||buff.Percent>100)return false;
     wchar_t percentage[24]{};swprintf_s(percentage,L" +%.1f%%",static_cast<double>(buff.Percent));buff.Caption+=percentage;buffs.push_back(std::move(buff));
    }
   }
   ActiveBuffs=std::move(buffs);BuffActive=json.value("active",bonus>0);BuffBonus=bonus;BuffCaption=std::move(caption);BuffReceived=timeGetTime();return true;}
  Snapshot next;
  next.Tab=json.at("tab"); next.Page=json.at("page"); if(next.Tab<0 || next.Tab>3 || next.Page<0 || next.Page>100) return false;
  next.Token=std::stoull(json.at("token").get<std::string>()); next.Enabled=json.at("enabled"); next.IsMentor=json.at("mentor"); next.IsNovice=json.at("novice");next.CanRequest=json.value("canRequest",next.IsNovice); next.Pending=json.at("pending");
  next.Points=json.at("points"); next.Graduated=json.at("graduated"); next.Daily=json.at("daily"); next.DailyLimit=json.at("dailyLimit"); next.Bonus=json.at("bonus"); next.NoviceBonus=json.at("noviceBonus"); next.Distance=json.at("distance"); next.Goal=json.at("goal"); next.Minimum=json.at("mentorMinimum");
  next.ActiveNovices=json.value("activeNovices",0); next.MaximumNovices=json.value("maximumNovices",8); next.HelpSeconds=json.value("helpSeconds",0.0); next.NoviceProgress=json.value("noviceProgress",0.0);
  next.RelationCount=json.at("relationCount"); next.MentorCount=json.at("mentorCount"); next.OfferCount=json.at("offerCount"); next.HistoryCount=json.at("historyCount"); next.Name=Wide(json.at("name")); next.Message=Wide(json.at("message"));
  for(auto it=json.at("labels").begin();it!=json.at("labels").end();++it) next.Labels[it.key()]=Wide(it.value());
  for(const auto& row:json.at("relations")) { Person value; value.Id=row.at("id"); value.Name=Wide(row.at("name")); value.Status=Wide(row.at("status")); value.Resets=row.at("resets"); value.Goal=row.at("goal"); value.Class=row.value("classNumber",-1); value.Level=row.value("level",0); value.Created=row.value("created",0LL); value.ClassName=ClassName(row); value.Accepted=row.at("accepted"); value.Graduated=row.at("graduated"); value.Incoming=row.at("incoming"); value.Online=row.at("online"); value.Party=row.at("sameParty"); value.Map=row.at("sameMap"); value.Near=row.at("near"); next.Relations.push_back(value); }
  next.AssignedMentor=Wide(json.value("assignedMentor",std::string())); next.Requests=json.value("requests",0);
  if(json.contains("top"))for(const auto& row:json.at("top")){Person value;value.Name=Wide(row.at("name"));value.Class=row.value("classNumber",-1);value.ClassName=ClassName(row);value.GuildName=Wide(row.value("guildName",std::string()));value.GraduatedCount=row.at("graduated");next.Top.push_back(value);}
  for(const auto& row:json.at("mentors")) { Person value; value.Name=Wide(row.at("name")); value.ClassName=ClassName(row); value.GuildName=Wide(row.value("guildName",std::string())); value.Resets=row.at("resets"); value.Class=row.value("classNumber",0); value.Id=row.value("requestId",std::string());value.Accepted=row.value("accepted",false);value.Status=Wide(row.value("status",std::string()));value.Online=row.value("online",false);value.Level=row.value("level",0);value.Novices=row.at("novices"); value.Maximum=row.at("maximum"); next.Mentors.push_back(value); }
  for(const auto& row:json.at("offers")) { Offer value; value.Id=row.at("id"); value.Name=Wide(row.at("name")); value.Cost=row.at("cost"); for(const auto& r:row.at("rewards")) value.Rewards.push_back({r.at("type"),r.at("amount"),r.at("itemGroup"),r.at("itemNumber"),r.at("level"),r.value("excellent",0),r.value("ancient",0),r.value("ancientBonus",0),r.value("option",0),r.value("durability",0),r.value("skill",false),r.value("luck",false)}); if(value.Rewards.size()>16) return false; next.Offers.push_back(value); }
  if(next.Relations.size()>7 || next.Mentors.size()>6 || next.Top.size()>6 || next.Offers.size()>6) return false;
  if(json.contains("link")&&!json.at("link").is_null()){const auto& link=json.at("link");next.LinkId=link.at("id");next.LinkName=Wide(link.at("mentorName"));next.LinkState=Wide(link.at("state"));next.LinkIncoming=link.value("incoming",false);next.LinkAccepted=link.value("accepted",false);}
  if(json.contains("ledger")){for(const auto& row:json.at("ledger")){Ledger entry;entry.MentorName=Wide(row.at("mentorName"));entry.StudentName=Wide(row.at("noviceName"));entry.State=Wide(row.at("state"));entry.Seconds=static_cast<long long>(row.value("validSeconds",0.0));entry.Points=row.at("awardedPoints");entry.GraduatedAt=row.at("graduatedAt").is_null()?0:row.at("graduatedAt").get<long long>();next.Records.push_back(entry);}if(next.Records.size()>3)return false;}
  next.History=Entries(json.at("history")); next.Recent=Entries(json.at("recent")); if(!Received)Trace("snapshot-accepted"); Data=std::move(next); Received=timeGetTime(); return true;
 } catch(const std::exception& error) { Trace("snapshot-rejected",error.what()); return false; }
}
void Request(BYTE action,BYTE tab,BYTE page,const std::string& identity,const wchar_t* target)
{
 if(!SocketClient || action>6 || tab>3 || page>100 || identity.size()>36){Trace("request-rejected");return;}
 if(!Received)Trace("request-sent");
 BYTE packet[96]={0xC1,96,0xFA,0xD2,1,action,tab,page}; std::memcpy(packet+8,&Data.Token,8); std::memcpy(packet+16,identity.data(),identity.size());
 int length=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,target,-1,nullptr,0,nullptr,nullptr); if(length<1 || length>32) return;
 WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,target,-1,reinterpret_cast<char*>(packet+56),32,nullptr,nullptr); SocketClient->Send(packet,sizeof(packet));
}
} // namespace Network::Paris::Mentor
