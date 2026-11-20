#include <doctest.h>
#include "Network/Paris/EventGuideProtocol.h"

TEST_CASE("Event guide publishes complete current snapshots [network][events]")
{
    Network::Paris::EventGuide::Decoder decoder;
    decoder.Expect(7);
    CHECK_FALSE(decoder.Receive("[PG]B|7", 100));
    decoder.Receive("[PG]L|7|mini-2|Blood Castle|classic|open|58", 101);
    decoder.Receive("[PG]D|7|mini-2|Blood Castle 4|open|58|3|10|1", 102);
    decoder.Receive("[PG]R|7|Ticket|Cloak +4 x1|1", 103);
    auto snapshot = decoder.Receive("[PG]Z|7", 104);
    REQUIRE(snapshot); REQUIRE(snapshot->detail);
    CHECK(snapshot->detail->canJoin);
    CHECK(snapshot->detail->requirements.at(0).value == "Cloak +4 x1");
}
TEST_CASE("Event guide rejects stale malformed and oversized snapshots [network][events]")
{
    Network::Paris::EventGuide::Decoder decoder;
    decoder.Expect(8); decoder.Receive("[PG]B|7", 100);
    CHECK_FALSE(decoder.Receive("[PG]Z|7", 101));
    decoder.Receive("[PG]B|8", 100);
    decoder.Receive("[PG]L|8|mini-2|Blood Castle|classic|open|NaN", 101);
    CHECK_FALSE(decoder.Receive("[PG]Z|8", 102));
    decoder.Expect(9); decoder.Receive("[PG]B|9", 100);
    decoder.Receive("[PG]L|9|mini-2|Blood Castle|classic|open|58", 101);
    CHECK_FALSE(decoder.Receive("[PG]Z|9", 10101));
}
