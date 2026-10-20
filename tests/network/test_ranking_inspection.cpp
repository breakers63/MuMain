#include "doctest.h"
#include "Network/Paris/RankingInspectionProtocol.h"
#include <vector>
#include <cstring>
using namespace Network::Paris::RankingInspection;
namespace
{
std::vector<std::uint8_t> Packet(int size, std::uint8_t subcode)
{
    std::vector<std::uint8_t> p(size);p[0]=0xC2;p[1]=static_cast<std::uint8_t>(size>>8);p[2]=static_cast<std::uint8_t>(size);p[3]=0xED;p[4]=subcode;p[5]=1;return p;
}
void Number(std::vector<std::uint8_t>& p,int offset,int value){std::memcpy(p.data()+offset,&value,sizeof(value));}
}
TEST_CASE("Ranking rejects equipment records with malformed item payloads and stale names")
{
    Data={};Data.equipmentName=L"Test123";
    auto p=Packet(427,0x5D);p[6]=1;p[7]=0;p[18]=1;std::memcpy(p.data()+8,"Test123",7);
    REQUIRE(Receive(p.data(),static_cast<int>(p.size())));CHECK(Data.publicEquipment);
    const auto revision=Data.revision;p[19]=1;p[20]=33;
    CHECK_FALSE(Receive(p.data(),static_cast<int>(p.size())));CHECK(Data.revision==revision);
    p[19]=0;p[6]=2;CHECK_FALSE(Receive(p.data(),static_cast<int>(p.size())));
    p[6]=0;p[8]='X';REQUIRE(Receive(p.data(),static_cast<int>(p.size())));CHECK(Data.publicEquipment);
}
TEST_CASE("Guild roster preserves attributes and rejects truncated or negative data")
{
    Data={};Data.guildName=L"ANIQUILA";
    auto p=Packet(68,0x5F);p[5]=2;std::memcpy(p.data()+6,"ANIQUILA",8);p[16]=1;
    std::memcpy(p.data()+18,"loki",4);p[28]=16;Number(p,30,400);Number(p,38,131068);Number(p,42,9);
    for(int i=0;i<5;++i)Number(p,46+i*4,32767);p[66]=1;p[67]=1;
    REQUIRE(Receive(p.data(),static_cast<int>(p.size())));REQUIRE(Data.members.size()==1);
    CHECK(Data.members[0].attributes[0]==32767);CHECK(Data.members[0].online);CHECK(Data.members[0].level==400);
    const auto revision=Data.revision;Number(p,30,-1);CHECK_FALSE(Receive(p.data(),static_cast<int>(p.size())));CHECK(Data.revision==revision);
    p.pop_back();CHECK_FALSE(Receive(p.data(),static_cast<int>(p.size())));
}
TEST_CASE("Mission catalog decodes real server targets and correlates ranking replies")
{
    Data={};Data.catalogLoading=true;auto p=Packet(669,0x11);p[5]=2;Number(p,6,1);Number(p,23,20);p[27]=3;std::memcpy(p.data()+34,"Paris #01",9);std::memcpy(p.data()+290,"Spider",6);p[33]=1;Number(p,355,10);
    REQUIRE(Receive(p.data(),static_cast<int>(p.size())));REQUIRE(Data.missions.size()==1);
    CHECK(Data.missions[0].required==20);CHECK(Data.missions[0].monster==3);CHECK(Data.missions[0].reward==10);
    Data.missionId=1;auto ranks=Packet(29,0x61);Number(ranks,6,1);ranks[10]=1;std::memcpy(ranks.data()+11,"Test123",7);Number(ranks,21,1);
    REQUIRE(Receive(ranks.data(),static_cast<int>(ranks.size())));REQUIRE(Data.missionRanks.size()==1);
    Number(ranks,6,2);CHECK_FALSE(Receive(ranks.data(),static_cast<int>(ranks.size())));
    CHECK(Data.missionRanks[0].name==L"Test123");
}
