#include "Network/Paris/EventTimerState.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cwchar>
namespace Network::Paris
{
EventTimerState EventTimers;
namespace
{
bool Number(const wchar_t*& cursor, wchar_t separator, int& result)
{
    if (*cursor < L'0' || *cursor > L'9')
        return false;
    wchar_t* end = nullptr;
    errno = 0;
    const long long value = std::wcstoll(cursor, &end, 10);
    if (errno == ERANGE || value > INT_MAX || *end != separator)
        return false;
    result = static_cast<int>(value);
    cursor = end + 1;
    return true;
}
} // namespace
bool EventTimerState::Receive(const wchar_t* text, std::uint32_t now)
{
    constexpr auto Prefix = L"[#EVT:";
    constexpr int PrefixSize = 6;
    if (!text || std::wcsncmp(text, Prefix, PrefixSize) != 0)
        return false;
    if (std::wcscmp(text + PrefixSize, L"C#]") == 0)
    {
        Clear();
        return true;
    }
    if (std::wcsncmp(text + PrefixSize, L"U|", 2) != 0)
        return true;
    const wchar_t* cursor = text + PrefixSize + 2;
    int id = 0, state = 0, seconds = 0;
    if (!Number(cursor, L'|', id) || !Number(cursor, L'|', state) || !Number(cursor, L'#', seconds) ||
        std::wcscmp(cursor, L"]") != 0 || id <= 0 || id >= EventCount || state > 1)
        return true;
    m_entries[id] = {true, state, seconds, now};
    return true;
}
bool EventTimerState::Get(int id, std::uint32_t now, int& state, int& seconds) const
{
    if (id <= 0 || id >= EventCount || !m_entries[id].valid)
        return false;
    constexpr int MillisecondsPerSecond = 1000;
    const auto& entry = m_entries[id];
    state = entry.state;
    seconds = std::max(0, entry.seconds - static_cast<int>((now - entry.tick) / MillisecondsPerSecond));
    return true;
}
void EventTimerState::Clear()
{
    m_entries = {};
}
} // namespace Network::Paris
