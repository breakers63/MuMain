#include "ActiveInvasionProtocol.h"
#include <array>
#include <charconv>
#include <vector>
namespace Network::Paris::ActiveInvasion
{
namespace
{
std::optional<int> Number(std::wstring_view text)
{
    if (text.empty() || text.size() > 9)
        return {};
    std::string digits;
    for (wchar_t c : text)
    {
        if (c < L'0' || c > L'9')
            return {};
        digits.push_back(static_cast<char>(c));
    }
    int value = 0;
    if (std::from_chars(digits.data(), digits.data() + digits.size(), value).ec != std::errc())
        return {};
    return value;
}
} // namespace
std::optional<Update> Decode(std::wstring_view text)
{
    constexpr std::wstring_view Prefix = L"[#AINV:";
    if (!text.starts_with(Prefix) || !text.ends_with(L"#]") || text.size() > 512)
        return {};
    if (text == L"[#AINV:C#]") {Update update; update.cleared = true; return update;}
    text.remove_prefix(Prefix.size());
    text.remove_suffix(2);
    std::vector<std::wstring_view> fields;
    while (true)
    {
        const auto split = text.find(L'|');
        fields.push_back(text.substr(0, split));
        if (split == text.npos)
            break;
        text.remove_prefix(split + 1);
    }
    if (fields.size() != 2 && fields.size() != 8)
        return {};
    const auto event = Number(fields[1]);
    if (!event || *event == 0)
        return {};
    Update update;
    update.event = *event;
    if (fields[0] == L"E" && fields.size() == 2)
    {
        update.ended = true;
        return update;
    }
    if (fields[0] != L"U" || fields.size() != 8)
        return {};
    const auto monster = Number(fields[2]), alive = Number(fields[3]), total = Number(fields[4]),
               seconds = Number(fields[5]);
    constexpr int MaximumCount = 65535, MaximumSeconds = 7 * 24 * 60 * 60;
    if (!monster || !alive || !total || !seconds || *monster > MaximumCount || *total > MaximumCount ||
        *alive > *total || *seconds > MaximumSeconds || fields[6].empty() || fields[7].empty() ||
        fields[6].size() > 64 || fields[7].size() > 64)
        return {};
    update.monster = *monster;
    update.alive = *alive;
    update.total = *total;
    update.seconds = *seconds;
    update.monsterName = fields[6];
    update.eventName = fields[7];
    return update;
}
} // namespace Network::Paris::ActiveInvasion
