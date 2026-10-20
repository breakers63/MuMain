#pragma once
#include <cstdint>
#include <string_view>

namespace Network::Paris
{
struct JewelBankMessage
{
    enum class Kind
    {
        Header,
        Entry,
        Complete
    };
    Kind kind = Kind::Complete;
    std::uint32_t request = 0;
    int status = 0;
    bool automatic = false;
    int count = 0;
    int row = 0;
    int item = 0;
    int level = 0;
    int inventory = 0;
    std::int64_t balance = 0;
    int maximum = 0;
    std::wstring_view name;
};
} // namespace Network::Paris
