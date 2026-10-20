#include "doctest.h"
#include <array>
#include "GameLogic/Social/PostedItemRecord.h"
#include "Data/GameConfig/HdGraphicsSettings.h"
#include "Network/Server/MissionRankingPacket.h"
#include "Network/Server/GuildMembersPacket.h"
#include "UI/NewUI/HUD/BattlePassScroll.h"

TEST_CASE("Posted item history rejects malformed and out of range item metadata")
{
    using GameLogic::Social::DecodePostedItem;
    CHECK_FALSE(DecodePostedItem(L"normal chat"));
    CHECK_FALSE(DecodePostedItem(L"[#PI:8192,0,0,0#]invalid"));
    CHECK_FALSE(DecodePostedItem(L"[#PI:1,16,0,0#]invalid"));
    CHECK_FALSE(DecodePostedItem(L"[#PI:1,0,64,0#]invalid"));
    CHECK_FALSE(DecodePostedItem(L"[#PI:1,0,0,0,1#]partial"));
    CHECK_FALSE(DecodePostedItem(L"[#PI:1,0,0,0junk#]invalid"));
    CHECK_FALSE(DecodePostedItem(L"[#PI:1,0,0,0,0,0,3#]extra"));
    CHECK_FALSE(DecodePostedItem(L"[#PI:9999999999999999999999,0,0,0#]overflow"));
    const auto old = DecodePostedItem(L"[#PI:12,9,3,0#]  Espada");
    REQUIRE(old);
    CHECK(old->Type == 12);
    CHECK(old->Level == 9);
    CHECK(old->Text == L"Espada");
    const auto current = DecodePostedItem(L"[#PI:12,15,63,5,1,7#]Excelente");
    REQUIRE(current);
    CHECK(current->Luck == 1);
    CHECK(current->AdditionalOption == 7);
}

TEST_CASE("HD profiles keep classic disabled and clamp user configuration")
{
    using namespace Data::Graphics;
    CHECK_FALSE(HdProfile(0).Enabled);
    CHECK(HdProfile(1).Enabled);
    CHECK(HdProfile(2).Sharpness > HdProfile(1).Sharpness);
    HdGraphicsSettings settings;
    settings.Sharpness = 10000;
    settings.BloomStrength = -1;
    settings.Exposure = 999;
    const auto normalized = NormalizeHdSettings(settings);
    CHECK(normalized.Sharpness == 100);
    CHECK(normalized.BloomStrength == 0);
    CHECK(normalized.Exposure == 125);
}

TEST_CASE("New battle pass scroll reaches all sparse reward columns")
{
    using namespace UI::BattlePass::Scroll;
    CHECK(MaximumOffset(20, 6) == 14);
    CHECK(MaximumOffset(3, 6) == 0);
    const int thumb = ThumbWidth(496, 20, 6);
    CHECK(OffsetAt(-500, 496, thumb, 14) == 0);
    CHECK(OffsetAt(900, 496, thumb, 14) == 14);
}

TEST_CASE("Mission ranking validates packet size before reading entries")
{
    std::array<std::uint8_t, 29> packet{0xC2, 0, 29, 0xED, 0x61, 1};
    packet[10] = 1;
    CHECK(Network::Ranking::MissionEntryCount(packet) == 1);
    CHECK(Network::Ranking::MissionEntryCount(std::span(packet).first(10)) == -1);
    packet[10] = 2;
    CHECK(Network::Ranking::MissionEntryCount(packet) == -1);
}
