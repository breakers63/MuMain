#pragma once
#include <array>
#include <string>
#include <algorithm>

namespace GameLogic::Social::ActivityHistory
{
    inline constexpr int Capacity = 128;
    struct Record
    {
        std::wstring Time, Actor, Description, Detail;
        int ItemType = -1, ItemLevel = 0, Excellent = 0, Ancient = 0;
        int Map = -1;
        bool Winner = false;
        int Luck = 0, AdditionalOptionLevel = 0;
    };
    class History
    {
        std::array<Record, Capacity> records{};
        int count = 0, next = 0;
        unsigned long long revision = 0;
    public:
        void Add(Record record)
        {
            record.Actor.resize((std::min)(record.Actor.size(), std::size_t(32)));
            record.Description.resize((std::min)(record.Description.size(), std::size_t(96)));
            record.Detail.resize((std::min)(record.Detail.size(), std::size_t(180)));
            records[next] = std::move(record);
            next = (next + 1) % Capacity;
            count = (std::min)(count + 1, Capacity);
            ++revision;
        }
        const Record* At(int newestIndex) const
        {
            if (newestIndex < 0 || newestIndex >= count) return nullptr;
            return &records[(next - newestIndex - 1 + Capacity) % Capacity];
        }
        int Count() const { return count; }
        unsigned long long Revision() const { return revision; }
        void Clear() { records = {}; count = next = 0; ++revision; }
    };
    inline History Kills, Posts;
}
