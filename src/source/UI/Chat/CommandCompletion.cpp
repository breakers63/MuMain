#include "UI/Chat/CommandCompletion.h"
#include <algorithm>
#include <cwctype>
namespace UI::Chat
{
bool CommandCompletion::Sync(std::wstring_view text, bool gm, bool focused)
{
    if (previous == text && previousGm == gm && previousFocus == focused) return false;
    previous.assign(text); previousGm = gm; previousFocus = focused;
    count = selected = first = 0;
    if (!focused || text.empty() || text.front() != L'/') return true;
    const auto space = text.find(L' ');
    std::wstring word(text.substr(0, space));
    std::transform(word.begin(), word.end(), word.begin(), [](wchar_t c) { return std::towlower(c); });
    for (int i = 0; i < static_cast<int>(std::size(CommandHints)); ++i)
    {
        const auto& command = CommandHints[i];
        if (command.gm && !gm) continue;
        const std::wstring_view name(command.name);
        if (space == std::wstring_view::npos ? name.starts_with(word) : name == word) matches[count++] = i;
    }
    return true;
}
void CommandCompletion::Move(int direction)
{
    if (!count) return;
    selected = (selected + direction + count) % count;
    if (selected < first) first = selected;
    if (selected >= first + VisibleRows) first = selected - VisibleRows + 1;
}
std::wstring CommandCompletion::Complete(int index) const
{
    if (index < 0 || index >= count || previous.find(L' ') != std::wstring::npos) return previous;
    return std::wstring(Row(index).name) + L' ';
}
}
