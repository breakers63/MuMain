#pragma once
#include <string_view>
#include <cstdint>
#include <string>
namespace UI::Chat
{
inline std::string PostColor = "rgba(0,0,0,255)";
inline std::string PostBackground = "rgba(0,200,255,150)";
inline void UpdatePostNoticeStyle(std::uint32_t rgba, std::uint16_t background, std::uint8_t alpha)
{
    PostColor = "rgba(" + std::to_string((rgba >> 24) & 255) + "," + std::to_string((rgba >> 16) & 255)
        + "," + std::to_string((rgba >> 8) & 255) + "," + std::to_string(rgba & 255) + ")";
    PostBackground = "rgba(" + std::to_string(((background >> 11) & 31) * 255 / 31) + ","
        + std::to_string(((background >> 5) & 63) * 255 / 63) + ","
        + std::to_string((background & 31) * 255 / 31) + "," + std::to_string(alpha) + ")";
}
inline constexpr bool IsPostMessage(std::wstring_view sender, std::wstring_view text)
{
    for (const auto marker : {L"[POST]", L"[Post]", L"[post]"})
        if (sender.find(marker) != std::wstring_view::npos || text.find(marker) != std::wstring_view::npos)
            return true;
    return false;
}
}
