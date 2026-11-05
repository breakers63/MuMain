#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Network::Paris::Vip
{
struct Snapshot
{
    int CurrentTier = 0;
    int RequestedPlan = 0;
    int RemainingDays = 0;
    bool CanBuy = false;
    std::uint64_t Quote = 0;
    std::vector<std::wstring> Lines;
};

extern Snapshot Data;
extern std::uint32_t Received;
bool Receive(const std::uint8_t* packet, int size);
} // namespace Network::Paris::Vip
