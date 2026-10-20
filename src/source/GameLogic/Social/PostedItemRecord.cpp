#include "PostedItemRecord.h"
#include <cwchar>
#include <cerrno>
#include <climits>

namespace GameLogic::Social
{
std::optional<PostedItemRecord> DecodePostedItem(const std::wstring& text)
{
    constexpr std::size_t MaximumTagLength = 80;
    constexpr int MaximumItemType = 8191, MaximumLevel = 15;
    if (text.compare(0, 5, L"[#PI:") != 0) return std::nullopt;
    const auto end = text.find(L"#]", 5);
    if (end == std::wstring::npos || end > MaximumTagLength) return std::nullopt;
    const auto tag = text.substr(0, end + 2);
    PostedItemRecord record;
    int* fields[]{&record.Type, &record.Level, &record.Excellent, &record.Ancient, &record.Luck, &record.AdditionalOption};
    const wchar_t* cursor = tag.c_str() + 5;
    int count = 0;
    for (int* field : fields)
    {
        errno = 0;
        wchar_t* next = nullptr;
        const long value = std::wcstol(cursor, &next, 10);
        if (next == cursor || errno == ERANGE || value < INT_MIN || value > INT_MAX) return std::nullopt;
        *field = static_cast<int>(value);
        ++count;
        cursor = next;
        if (*cursor != L',') break;
        if (count == 6) return std::nullopt;
        ++cursor;
    }
    if ((count != 4 && count != 6) || std::wcscmp(cursor, L"#]") != 0) return std::nullopt;
    if (record.Type < 0 || record.Type > MaximumItemType || record.Level < 0 || record.Level > MaximumLevel
        || record.Excellent < 0 || record.Excellent > 63 || record.Ancient < 0 || record.Ancient > 255
        || record.Luck < 0 || record.Luck > 1 || record.AdditionalOption < 0 || record.AdditionalOption > 7) return std::nullopt;
    record.Text = text.substr(end + 2);
    const auto first = record.Text.find_first_not_of(L' ');
    record.Text.erase(0, first == std::wstring::npos ? record.Text.size() : first);
    return record;
}
}
