#pragma once

#include <array>
#include <map>
#include <string>

namespace Network::Paris::Soul
{
    constexpr int AbilityCount = 16;
    struct Ability
    {
        bool Enabled = false;
        int MaximumRank = 1, Cost = 1;
        double MaximumBenefit = 0;
        std::array<int, 3> Milestones{};
        std::array<double, 3> Bonuses{};
        std::wstring Name;
    };
    struct Snapshot
    {
        bool Enabled = false, AllowReallocation = false, AllowReset = false;
        int Decision = 0, ResetFee = 0, ResetCurrency = 0;
        long long Total = 0, Spent = 0, Revision = 0;
        unsigned long long Token = 0;
        std::array<int, AbilityCount> Ranks{};
        std::array<long long, AbilityCount> Paid{};
        std::array<Ability, AbilityCount> Abilities{};
        std::map<std::string, std::wstring> Labels;
    };
    extern Snapshot Data;
    extern DWORD Received;
    bool Receive(const BYTE* packet, int size);
    void Clear();
    bool Request(BYTE action, BYTE category, const std::array<int, AbilityCount>& ranks);
    const std::wstring& Label(const char* key);
}
