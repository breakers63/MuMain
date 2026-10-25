#include "doctest.h"
#include "Network/Paris/AdvancedStatsSnapshot.h"
#include <bit>
#include <vector>

namespace
{
std::vector<std::uint8_t> Packet(bool extended, bool longHeader = false)
{
    const int count = extended ? 60 : 52;
    const int payload = longHeader ? 5 : 4;
    std::vector<std::uint8_t> packet(payload + 4 + count * 4);
    packet[0] = longHeader ? 0xC2 : 0xC1;
    if (longHeader) { packet[1] = static_cast<std::uint8_t>(packet.size() >> 8); packet[2] = static_cast<std::uint8_t>(packet.size()); }
    else packet[1] = static_cast<std::uint8_t>(packet.size());
    packet[payload - 2] = 0xED;
    packet[payload - 1] = 0x30;
    packet[payload] = extended ? 3 : 2;
    packet[payload + 1] = static_cast<std::uint8_t>(count);
    for (int index = 0; index < count; ++index)
    {
        const auto bits = std::bit_cast<std::uint32_t>(std::int32_t(index % 2 ? -100000 - index : 100000 + index));
        for (int byte = 0; byte < 4; ++byte) packet[payload + 4 + index * 4 + byte] = static_cast<std::uint8_t>(bits >> (byte * 8));
    }
    return packet;
}
}

TEST_CASE("Advanced stats decodes both supported versions and header sizes")
{
    for (bool extended : {false, true}) for (bool longHeader : {false, true})
    {
        auto packet = Packet(extended, longHeader);
        for (int encrypted : {0, 2})
        {
            packet[0] = static_cast<std::uint8_t>((longHeader ? 0xC2 : 0xC1) + encrypted);
            const auto decoded = Network::Paris::DecodeAdvancedStatsSnapshot(packet);
            REQUIRE(decoded.has_value());
            CHECK(decoded->receivedFields == (extended ? 60 : 52));
            CHECK(decoded->values[0] == 100000);
            CHECK(decoded->values[51] == -100051);
            CHECK(decoded->values[59] == (extended ? -100059 : 0));
        }
    }
}

TEST_CASE("Advanced stats rejects every truncated extended packet")
{
    auto packet = Packet(true);
    for (std::size_t size = 0; size < packet.size(); ++size)
    {
        auto cut = std::vector<std::uint8_t>(packet.begin(), packet.begin() + size);
        if (size > 1) cut[1] = static_cast<std::uint8_t>(size);
        CHECK_FALSE(Network::Paris::DecodeAdvancedStatsSnapshot(cut).has_value());
    }
}

TEST_CASE("Advanced stats rejects incompatible fields, versions and lengths")
{
    auto packet = Packet(true);
    for (const auto [offset, value] : {std::pair{0, 0xFF}, {1, 12}, {2, 0xFA}, {3, 0x31}, {4, 2}, {5, 52}})
    {
        auto invalid = packet;
        invalid[offset] = static_cast<std::uint8_t>(value);
        CHECK_FALSE(Network::Paris::DecodeAdvancedStatsSnapshot(invalid).has_value());
    }
    packet.push_back(0);
    packet[1] = static_cast<std::uint8_t>(packet.size());
    CHECK_FALSE(Network::Paris::DecodeAdvancedStatsSnapshot(packet).has_value());
}
