#include "doctest.h"
#include "Network/Server/MonsterViewportType.h"
#include "World/Monsters/CustomMonsterColors.h"
#include <sstream>
TEST_CASE("Configured NPC IDs retain their high bits")
{
    auto configured = [](int id) { return id == 1150 || id == 1700; };
    CHECK(Network::Viewport::ResolveMonsterType(4, 126, configured) == 1150);
    CHECK(Network::Viewport::ResolveMonsterType(6, 164, configured) == 1700);
    CHECK(Network::Viewport::ResolveMonsterType(0x84, 126, configured) == 126);
    CHECK(Network::Viewport::ResolveMonsterType(1, 64, configured) == 320);
}
TEST_CASE("Custom golden colours are bounded and separate from models")
{
    std::istringstream valid("0\n1150 0 \"NPC\" 2 \"Tower\" 0.8\nend\n1\n1150 1 0.6 0.1\nend\n");
    auto colors = World::Monsters::Custom::ReadGoldenColors(valid);
    REQUIRE(colors.size() == 1);
    CHECK(colors.at(1150)[1] == doctest::Approx(0.6f));
    std::istringstream invalid("1\n1150 2 0 0\nend\n");
    CHECK_THROWS_AS(World::Monsters::Custom::ReadGoldenColors(invalid), std::invalid_argument);
}
