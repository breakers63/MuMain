#pragma once
#include <cstdint>
namespace Network::Paris
{
struct NoticeStyle
{
    std::uint32_t rgba = 0xFFFFFFFF;
    std::uint16_t background = 0;
    std::uint8_t backgroundAlpha = 128;
    bool blink = false;
    bool enabled = false;
};

inline constexpr bool DecodeNoticeStyle(int result, std::uint8_t count, std::uint16_t background,
    std::uint32_t rgba, std::uint8_t backgroundAlpha, NoticeStyle& style)
{
    constexpr std::uint8_t CustomFlag = 0x80, BlinkFlag = 0x40, ReservedMask = 0x38, CategoryMask = 7;
    constexpr int CategoryCount = 5;
    if ((result != 0 && result != 2) || (count & CustomFlag) == 0 || (count & ReservedMask) != 0
        || (count & CategoryMask) >= CategoryCount)
        return false;
    style = {rgba, background, backgroundAlpha, (count & BlinkFlag) != 0, true};
    return true;
}
}
