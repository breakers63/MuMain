#include "Network/Paris/StatLimitsSnapshot.h"
#include <limits>
namespace Network::Paris {
std::optional<StatLimitsSnapshot> DecodeStatLimits(std::wstring_view text)
{
    constexpr std::wstring_view Prefix = L"[#PSL1:";
    if (!text.starts_with(Prefix)) return std::nullopt;
    text.remove_prefix(Prefix.size());
    StatLimitsSnapshot snapshot;
    for (int index = 0; index < 6; ++index)
    {
        const std::uint64_t maximum = index == 0 ? std::numeric_limits<std::uint32_t>::max() : 65535;
        std::uint64_t value = 0;
        if (text.empty() || text.front() < L'0' || text.front() > L'9') return std::nullopt;
        while (!text.empty() && text.front() >= L'0' && text.front() <= L'9')
        {
            value = value * 10 + (text.front() - L'0');
            if (value > maximum) return std::nullopt;
            text.remove_prefix(1);
        }
        if (text.empty() || text.front() != (index == 5 ? L']' : L'|')) return std::nullopt;
        text.remove_prefix(1);
        if (index == 0) snapshot.request = static_cast<std::uint32_t>(value);
        else snapshot.values[index - 1] = static_cast<int>(value);
    }
    return text.empty() ? std::optional(snapshot) : std::nullopt;
}
}
