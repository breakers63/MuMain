#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace Network::Paris
{
using PacketHandler = std::function<void(const std::uint8_t*, int)>;
using TextHandler = std::function<bool(std::wstring_view)>;

// RmlUi systems register for only the response family they understand. The packet is validated
// before it reaches a feature parser; unknown extension subcodes are ignored.
std::uint64_t RegisterHandler(std::uint8_t head, std::uint8_t subcode, PacketHandler handler);
void UnregisterHandler(std::uint64_t registration);
std::uint64_t RegisterTextHandler(TextHandler handler);
void UnregisterTextHandler(std::uint64_t registration);
bool DispatchText(std::wstring_view text);
void Dispatch(const std::uint8_t* packet, int size);
} // namespace Network::Paris
