#include "doctest.h"
#include "Network/Paris/StatLimitsSnapshot.h"

TEST_CASE("Stat limits preserve server request correlation and bounded attribute limits")
{
    const auto result = Network::Paris::DecodeStatLimits(L"[#PSL1:4294967295|65535|32767|0|100|500]");
    REQUIRE(result.has_value());
    CHECK(result->request == 4294967295u);
    CHECK(result->values == std::array<int, 5>{65535, 32767, 0, 100, 500});
}

TEST_CASE("Stat limits reject incomplete oversized signed and trailing payloads")
{
    for (const auto* text : {L"", L"[#PSL1:1|1|2|3|4]", L"[#PSL1:1|1|2|3|4|5|6]",
        L"[#PSL1:4294967296|1|2|3|4|5]", L"[#PSL1:1|65536|2|3|4|5]",
        L"[#PSL1:-1|1|2|3|4|5]", L"[#PSL1:1|1|2|3|4|-5]", L"[#PSL1:1|1|2|3|4|5]extra",
        L"[#PSL1:1|1|2|3|4|999999999999999999999999999999]", L"[#PSL1:1|1|2|3|4|]"})
        CHECK_FALSE(Network::Paris::DecodeStatLimits(text).has_value());
}
