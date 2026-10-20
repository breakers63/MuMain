#pragma once
#include <array>
#include <cstdint>
namespace Network::Paris
{
class EventTimerState
{
public:
    static constexpr int EventCount = 18;
    bool Receive(const wchar_t* text, std::uint32_t now);
    bool Get(int id, std::uint32_t now, int& state, int& seconds) const;
    void Clear();

private:
    struct Entry
    {
        bool valid = false;
        int state = 0;
        int seconds = 0;
        std::uint32_t tick = 0;
    };
    std::array<Entry, EventCount> m_entries{};
};
extern EventTimerState EventTimers;
} // namespace Network::Paris
