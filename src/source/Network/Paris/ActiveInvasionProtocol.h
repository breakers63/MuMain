#pragma once
#include <optional>
#include <string>
#include <string_view>
namespace Network::Paris::ActiveInvasion
{
struct Update
{
    int event = 0, monster = 0, alive = 0, total = 0, seconds = 0;
    bool ended = false;
    bool cleared = false;
    std::wstring monsterName, eventName;
};
std::optional<Update> Decode(std::wstring_view text);
} // namespace Network::Paris::ActiveInvasion
