#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
namespace Network::Paris {
struct StatLimitsSnapshot { std::uint32_t request = 0; std::array<int,5> values{}; };
std::optional<StatLimitsSnapshot> DecodeStatLimits(std::wstring_view text);
}
