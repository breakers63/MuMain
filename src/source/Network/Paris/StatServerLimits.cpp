#include "Network/Paris/StatServerLimits.h"
namespace Network::Paris::ServerLimits
{
std::array<int, 5> Values{};
unsigned int Request = 0;
bool Ready = false, Pending = false;
bool Receive(const wchar_t* text)
{
    constexpr const wchar_t* prefix = L"[#PSL1:";
    if (text == nullptr)
        return false;
    if (std::wcsncmp(text, prefix, 7) != 0)
        return false;
    if (!Pending)
        return true;
    const wchar_t* cursor = text + 7;
    std::array<unsigned int, 6> fields{};
    for (int i = 0; i < 6; ++i)
    {
        if (*cursor < L'0' || *cursor > L'9')
            return true;
        std::uint64_t value = 0;
        while (*cursor >= L'0' && *cursor <= L'9')
        {
            value = value * 10 + (*cursor++ - L'0');
            if (value > (i == 0 ? UINT32_MAX : 65535u))
                return true;
        }
        fields[i] = static_cast<unsigned int>(value);
        if (*cursor++ != (i == 5 ? L']' : L'|'))
            return true;
    }
    if (*cursor != L'\0' || fields[0] != Request)
        return true;
    for (int i = 0; i < 5; ++i)
        Values[i] = static_cast<int>(fields[i + 1]);
    Ready = true;
    Pending = false;
    return true;
}
} // namespace Network::Paris::ServerLimits
