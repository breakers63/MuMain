#include <doctest.h>
#include "Network/Paris/ParisBattlePassProtocol.h"
#include "GameLogic/Social/BattlePassRewardState.h"

namespace
{
std::vector<BYTE> Snapshot(int rewards = 1)
{
    std::vector<BYTE> packet(80 + rewards * 112);
    packet[0]=0xC2; packet[1]=static_cast<BYTE>(packet.size() >> 8); packet[2]=static_cast<BYTE>(packet.size());
    packet[3]=0xFA; packet[4]=0xB1; packet[5]=1; packet[6]=1;
    packet[8]=1; packet[10]=2; packet[20]=1; packet[44]=static_cast<BYTE>(rewards);
    packet[48]=1; packet[72]=1;
    for(int i=0;i<rewards;++i)
    {
        const int offset=80+i*112;
        packet[offset]=static_cast<BYTE>(i+1); packet[offset+4]=2; packet[offset+8]=100;
        packet[offset+48]='Z'; packet[offset+49]='e'; packet[offset+50]='n';
    }
    return packet;
}
}
TEST_CASE("Battle Pass decodes the shared server snapshot and authoritative quote")
{
    const auto parsed=Network::Paris::BattlePass::Decode(Snapshot());
    REQUIRE(parsed.has_value());
    CHECK(parsed->ConfigurationToken==1);
    CHECK(parsed->Level==1);
    REQUIRE(parsed->Rewards.size()==1);
    CHECK(parsed->Rewards[0].Name==L"Zen");
    CHECK(parsed->Rewards[0].Amount==100);
}
TEST_CASE("Battle Pass rejects every truncated snapshot")
{
    auto packet=Snapshot();
    for(std::size_t size=0;size<packet.size();++size)
        CHECK_FALSE(Network::Paris::BattlePass::Decode(std::span<const BYTE>(packet.data(),size)));
}
TEST_CASE("Battle Pass rejects duplicate rewards and invalid quotes")
{
    auto packet=Snapshot(2);
    packet[192]=1;
    CHECK_FALSE(Network::Paris::BattlePass::Decode(packet));
    packet=Snapshot(); packet[72]=0;
    CHECK_FALSE(Network::Paris::BattlePass::Decode(packet));
}
TEST_CASE("Battle Pass rejects incomplete native items and invalid text")
{
    auto packet=Snapshot(); packet[84]=6; packet[85]=4;
    CHECK_FALSE(Network::Paris::BattlePass::Decode(packet));
    packet=Snapshot(); packet[128]=0xFF;
    CHECK_FALSE(Network::Paris::BattlePass::Decode(packet));
}
TEST_CASE("Battle Pass gates reward access on server level and premium entitlement")
{
    using GameLogic::Social::BattlePassRewardState::Unlocked;
    CHECK(Unlocked(true,2,1,false,false,false));
    CHECK_FALSE(Unlocked(false,2,1,false,false,false));
    CHECK_FALSE(Unlocked(true,0,1,false,false,false));
    CHECK_FALSE(Unlocked(true,2,1,true,false,false));
    CHECK(Unlocked(true,2,1,true,true,false));
}
TEST_CASE("Battle Pass reads current premium requirements and rejects invalid quotes")
{
    auto packet = Snapshot();
    packet[47] = 1;
    packet[56] = 9;
    packet[57] = 30;
    packet[59] = 1;
    const auto decoded = Network::Paris::BattlePass::Decode(packet);
    REQUIRE(decoded);
    CHECK(decoded->PremiumTermsKnown);
    CHECK(decoded->PremiumRequirementReady);
    CHECK(decoded->PremiumItemLevel == 9);
    packet[59] = 2;
    CHECK_FALSE(Network::Paris::BattlePass::Decode(packet));
    packet[59] = 1;
    packet[56] = 16;
    CHECK_FALSE(Network::Paris::BattlePass::Decode(packet));
}
