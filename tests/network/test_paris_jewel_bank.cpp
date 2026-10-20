#include "doctest.h"
#include "Network/Paris/JewelBankProtocol.h"

TEST_CASE("Paris bank accepts the original snapshot messages")
{
    Network::Paris::JewelBankMessage message;
    CHECK(Network::Paris::JewelBank::Decode(L"[#PJB1:7|0|1|1]", 7, message));
    CHECK(message.automatic);
    CHECK(message.count == 1);
    CHECK(Network::Paris::JewelBank::Decode(L"[#PJB2:7|0|7181|0|3|9000000000|100|Bless]", 7, message));
    CHECK(message.balance == 9000000000LL);
    CHECK(message.name == L"Bless");
    CHECK(Network::Paris::JewelBank::Decode(L"[#PJB3:7]", 7, message));
    CHECK(message.kind == Network::Paris::JewelBankMessage::Kind::Complete);
}

TEST_CASE("Paris bank rejects malformed and unrelated replies")
{
    Network::Paris::JewelBankMessage message;
    for (const auto text : {
        L"[#PJB", L"[#PJB1:7|0|1|25]", L"[#PJB1:7|0|2|1]",
        L"[#PJB1:7|0|1|1]extra", L"[#PJB3:8]", L"[#PJB3:7|unexpected]",
        L"[#PJB2:7|24|7181|0|3|0|100|Bless]", L"[#PJB2:7|0|7181|0|-1|0|100|Bless]",
        L"[#PJB2:7|0|7181|0|3|9223372036854775808|100|Bless]",
        L"[#PJB2:7|0|7181|0|3|0|100|]", L"[#PJB3:42949672967]" })
        CHECK_FALSE(Network::Paris::JewelBank::Decode(text, 7, message));
    CHECK_FALSE(Network::Paris::JewelBank::Decode(L"[#PJB3:0]", 0, message));
}
