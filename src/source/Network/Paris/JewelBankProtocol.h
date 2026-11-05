#pragma once
#include "Network/Paris/JewelBankMessage.h"

namespace Network::Paris::JewelBank
{
constexpr int MaximumEntries = 24;
constexpr int MaximumNameLength = 47;
bool Decode(std::wstring_view text, std::uint32_t expectedRequest, JewelBankMessage& message);
} // namespace Network::Paris::JewelBank
