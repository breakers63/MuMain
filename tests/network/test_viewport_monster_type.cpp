#include "doctest.h"
#include "Network/Server/ViewportMonsterType.h"
TEST_CASE("Viewport retains custom NPC identifiers above ten bits")
{
    using Network::Viewport::DecodeMonsterType;
    CHECK(DecodeMonsterType(0x04, 0x4D) == 1101);
    CHECK(DecodeMonsterType(0x04, 0x7E) == 1150);
    CHECK(DecodeMonsterType(0x02, 0x0E) == 526);
    CHECK(DecodeMonsterType(0x01, 0x7A) == 378);
    CHECK(DecodeMonsterType(0x03, 0xFF) == 1023);
    CHECK(DecodeMonsterType(0x00, 0x00) == 0);
}
TEST_CASE("Viewport flags preserve custom NPC identifiers")
{
    using Network::Viewport::DecodeMonsterType;
    for (unsigned flags = 0; flags <= 0xF0; flags += 0x10)
        CHECK(DecodeMonsterType(static_cast<std::uint8_t>(flags | 0x04), 0x7E) == 1150);
}
