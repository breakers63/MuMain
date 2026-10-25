#include "doctest.h"
#include "Network/Paris/PartyFinderProtocol.h"
#include <string>
#include <vector>
namespace
{
std::vector<std::uint8_t> Packet(const std::string& json)
{
    std::vector<std::uint8_t> bytes{0xC2, 0, 0, 0xFA, 0xE1, 1};
    bytes.insert(bytes.end(), json.begin(), json.end());
    bytes[1] = static_cast<std::uint8_t>(bytes.size() >> 8);
    bytes[2] = static_cast<std::uint8_t>(bytes.size());
    return bytes;
}
}
TEST_CASE("party finder accepts public parties and validated companion positions")
{
    using namespace Network::Paris::PartyFinder;
    Reset();
    auto packet = Packet(R"({"request":7,"status":0,"isPublic":true,"parties":[{"id":"0123456789abcdef0123456789abcdef","leader":"CONDOR","map":8,"members":2,"capacity":5}],"target":{"name":"loki","map":8,"x":120,"y":133}})");
    REQUIRE(Receive(packet.data(), static_cast<int>(packet.size())));
    REQUIRE(Data.parties.size() == 1);
    CHECK(Data.parties[0].leader == "CONDOR");
    CHECK(Data.isPublic);
    CHECK(Data.hasTarget);
    CHECK(Data.targetX == 120);
    CHECK(Data.targetName == "loki");
}
TEST_CASE("malformed party snapshots cannot replace accepted state")
{
    using namespace Network::Paris::PartyFinder;
    auto valid = Packet(R"({"request":7,"status":0,"isPublic":false,"parties":[],"target":null})");
    REQUIRE(Receive(valid.data(), static_cast<int>(valid.size())));
    for (const std::string json : {
        R"({"request":8,"status":0,"isPublic":true,"parties":[],"target":{"name":"loki","map":0,"x":256,"y":1}})",
        R"({"request":8,"status":0,"isPublic":true,"parties":{},"target":null})",
        R"({"request":8,"status":0,"isPublic":true,"parties":[{"id":"0123456789abcdef0123456789abcdef","leader":"x","map":0,"members":6,"capacity":5}],"target":null})",
        R"({"request":8,"status":-1,"isPublic":true,"parties":[],"target":null})",
        R"({"request":8,"status":0,"isPublic":true,"parties":[],"target":{"name":"loki","map":-1,"x":1,"y":1}})",
        R"({"request":8,"status":0,"isPublic":true})"})
    {
        auto invalid = Packet(json);
        CHECK_FALSE(Receive(invalid.data(), static_cast<int>(invalid.size())));
        CHECK(Data.request == 7);
        CHECK_FALSE(Data.isPublic);
    }
    valid[2]--;
    CHECK_FALSE(Receive(valid.data(), static_cast<int>(valid.size())));
    CHECK_FALSE(Receive(nullptr, 10));
}

TEST_CASE("party settings snapshot preserves disabled follow authorization")
{
    using namespace Network::Paris::PartyFinder;
    auto disabled = Packet(R"({"request":91,"status":0,"isPublic":false,"followEnabled":false,"autoAcceptParty":true,"parties":[],"target":null})");
    REQUIRE(Receive(disabled.data(), static_cast<int>(disabled.size())));
    CHECK_FALSE(Data.followEnabled);
    CHECK(Data.autoAcceptParty);
    auto invalid = Packet(R"({"request":92,"status":0,"isPublic":true,"followEnabled":"true","parties":[],"target":null})");
    CHECK_FALSE(Receive(invalid.data(), static_cast<int>(invalid.size())));
    CHECK_FALSE(Data.followEnabled);
    CHECK(Data.request == 91);
}

TEST_CASE("party member vitals and personal preferences reject malformed updates")
{
    using namespace Network::Paris::PartyFinder;
    const std::string prefix = R"({"request":101,"status":0,"isPublic":false,"showBuffs":false,"blockSummon":true,"parties":[],"target":null,"members":[)";
    const std::string row = R"({"name":"loki","level":400,"resets":7,"health":100,"maximumHealth":200,"mana":4200,"maximumMana":10000,"buffs":[3,4]})";
    auto valid = Packet(prefix + row + "]}");
    REQUIRE(Receive(valid.data(), static_cast<int>(valid.size())));
    REQUIRE(FindMember("loki") != nullptr);
    CHECK(FindMember("loki")->mana == 4200);
    CHECK(Data.blockSummon);
    CHECK_FALSE(Data.showBuffs);
    CHECK(FindMember("outsider") == nullptr);
    for (const std::string bad : std::vector<std::string>{
        R"({"name":"loki","level":400,"resets":7,"health":100,"maximumHealth":200,"mana":-1,"maximumMana":10000,"buffs":[]})",
        R"({"name":"loki","level":400,"resets":7,"health":100,"maximumHealth":200,"mana":1,"maximumMana":10000,"buffs":[161]})",
        row + "," + row})
    {
        auto invalid = Packet(prefix + bad + "]}");
        CHECK_FALSE(Receive(invalid.data(), static_cast<int>(invalid.size())));
        CHECK(FindMember("loki")->mana == 4200);
    }
}
