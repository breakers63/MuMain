#include "Network/Paris/JewelBankProtocol.h"
#include <limits>

namespace Network::Paris::JewelBank
{
namespace
{
constexpr std::uint64_t DecimalBase = 10;
constexpr std::uint64_t MaximumItem = 8191;
constexpr std::uint64_t MaximumLevel = 15;
constexpr std::uint64_t MaximumStatus = 255;

bool Number(std::wstring_view& text, std::uint64_t limit, std::uint64_t& value)
{
    if (text.empty())
        return false;
    value = 0;
    std::size_t length = 0;
    while (length < text.size() && text[length] >= L'0' && text[length] <= L'9')
    {
        const auto digit = static_cast<std::uint64_t>(text[length] - L'0');
        if (digit > limit || value > (limit - digit) / DecimalBase)
            return false;
        value = value * DecimalBase + digit;
        ++length;
    }
    if (length == 0 || (length < text.size() && text[length] != L'|'))
        return false;
    text.remove_prefix(length < text.size() ? length + 1 : length);
    return true;
}

bool Header(std::wstring_view text, JewelBankMessage& message)
{
    std::uint64_t status, automatic, count;
    if (!Number(text, MaximumStatus, status) || !Number(text, 1, automatic) || !Number(text, MaximumEntries, count) ||
        !text.empty())
        return false;
    message.kind = JewelBankMessage::Kind::Header;
    message.status = static_cast<int>(status);
    message.automatic = automatic != 0;
    message.count = static_cast<int>(count);
    return true;
}

bool Entry(std::wstring_view text, JewelBankMessage& message)
{
    constexpr auto IntegerLimit = static_cast<std::uint64_t>(std::numeric_limits<int>::max());
    constexpr auto BalanceLimit = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    std::uint64_t row, item, level, inventory, balance, maximum;
    if (!Number(text, MaximumEntries - 1, row) || !Number(text, MaximumItem, item) ||
        !Number(text, MaximumLevel, level) || !Number(text, IntegerLimit, inventory) ||
        !Number(text, BalanceLimit, balance) || !Number(text, IntegerLimit, maximum) || text.empty() ||
        text.size() > MaximumNameLength || text.find_first_of(L"|[]") != std::wstring_view::npos)
        return false;
    message.kind = JewelBankMessage::Kind::Entry;
    message.row = static_cast<int>(row);
    message.item = static_cast<int>(item);
    message.level = static_cast<int>(level);
    message.inventory = static_cast<int>(inventory);
    message.balance = static_cast<std::int64_t>(balance);
    message.maximum = static_cast<int>(maximum);
    message.name = text;
    return true;
}
} // namespace

bool Decode(std::wstring_view text, std::uint32_t expectedRequest, JewelBankMessage& message)
{
    constexpr std::size_t PrefixLength = 7;
    if (expectedRequest == 0 || text.size() <= PrefixLength || text.back() != L']')
        return false;
    const auto prefix = text.substr(0, PrefixLength);
    if (prefix != L"[#PJB1:" && prefix != L"[#PJB2:" && prefix != L"[#PJB3:")
        return false;
    text.remove_prefix(PrefixLength);
    text.remove_suffix(1);
    std::uint64_t request;
    if (!Number(text, std::numeric_limits<std::uint32_t>::max(), request) || request != expectedRequest)
        return false;
    JewelBankMessage decoded;
    decoded.request = static_cast<std::uint32_t>(request);
    bool valid;
    if (prefix == L"[#PJB1:")
        valid = Header(text, decoded);
    else if (prefix == L"[#PJB2:")
        valid = Entry(text, decoded);
    else
        valid = text.empty();
    if (valid)
        message = decoded;
    return valid;
}
} // namespace Network::Paris::JewelBank
