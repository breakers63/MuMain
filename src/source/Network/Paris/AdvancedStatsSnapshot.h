#pragma once

#include "Network/Paris/AdvancedStatsField.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace Network::Paris
{
struct AdvancedStatsSnapshot
{
    static constexpr int LegacyFieldCount = 52;
    static constexpr int FieldCount = static_cast<int>(AdvancedStatsField::Count);
    std::array<std::int32_t, FieldCount> values{};
    int receivedFields = 0;
};

std::optional<AdvancedStatsSnapshot> DecodeAdvancedStatsSnapshot(std::span<const std::uint8_t> packet);
}
