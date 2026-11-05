#pragma once
#include <string>
#include <vector>
#include <map>
namespace Network::Paris::Mentor
{
struct Person { std::string Id; std::wstring Name, Status, ClassName, GuildName; long long Created=0; int Resets=0, Goal=16, Class=0, Novices=0, Maximum=0, GraduatedCount=0, Level=0; bool Accepted=false, Graduated=false, Incoming=false, Online=false, Party=false, Map=false, Near=false; };
struct Reward { int Type=0, Amount=0, Group=0, Number=0, Level=0, Excellent=0, Ancient=0, AncientBonus=0, Option=0, Durability=0; bool Skill=false, Luck=false; };
struct Offer { std::string Id; std::wstring Name; int Cost=0; std::vector<Reward> Rewards; };
struct Entry { std::wstring Name, Kind, MentorName, StudentName, State; int Points=0, Awarded=0, Resets=0, Goal=0; long long Time=0, Created=0, Ended=0, Seconds=0; bool HasDetail=false; };
struct Ledger { std::wstring MentorName, StudentName, State; long long Seconds=0, GraduatedAt=0; int Points=0; };
struct Snapshot { bool CanRequest=false, LinkIncoming=false, LinkAccepted=false, Enabled=false, IsMentor=false, IsNovice=false, Pending=false; double HelpSeconds=0, NoviceProgress=0; int ActiveNovices=0, MaximumNovices=0; int Tab=0, Page=0, Points=0, Graduated=0, Daily=0, DailyLimit=0, Bonus=0, NoviceBonus=0, Distance=0, Goal=0, Minimum=0, RelationCount=0, MentorCount=0, OfferCount=0, HistoryCount=0, Requests=0; unsigned long long Token=0; std::wstring Name, Message, AssignedMentor, LinkName, LinkState; std::string LinkId; std::map<std::string,std::wstring> Labels; std::vector<Person> Relations, Mentors, Top; std::vector<Offer> Offers; std::vector<Entry> History, Recent; std::vector<Ledger> Records; };
extern Snapshot Data;
struct ActiveBuff { int Number=0; float Percent=0; std::wstring Caption; };
extern std::vector<ActiveBuff> ActiveBuffs;
extern bool BuffActive;
extern DWORD Received;
extern int BuffBonus; extern DWORD BuffReceived; extern std::wstring BuffCaption;
void Trace(const char* event, const char* detail="");
bool Receive(const BYTE* packet,int size);
void Request(BYTE action,BYTE tab,BYTE page,const std::string& identity={},const wchar_t* target=L"");
} // namespace Network::Paris::Mentor
