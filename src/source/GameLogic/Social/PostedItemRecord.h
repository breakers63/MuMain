#pragma once
#include <optional>
#include <string>

namespace GameLogic::Social
{
struct PostedItemRecord
{
    int Type = -1, Level = 0, Excellent = 0, Ancient = 0, Luck = 0, AdditionalOption = 0;
    std::wstring Text;
};
std::optional<PostedItemRecord> DecodePostedItem(const std::wstring& text);
}
