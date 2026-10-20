#pragma once
#include <string_view>
namespace UI::Chat
{
inline constexpr bool IsPostMessage(std::wstring_view sender, std::wstring_view text)
{
    for (const auto marker : {L"[POST]", L"[Post]", L"[post]"})
        if (sender.find(marker) != std::wstring_view::npos || text.find(marker) != std::wstring_view::npos)
            return true;
    return false;
}
}
