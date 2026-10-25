#pragma once

#include "Data/GameConfig/SavedLoginAccount.h"
#include <span>
#include <string_view>

namespace Data::Login
{
inline int ChooseAccountSlot(std::span<const SavedLoginAccount> accounts, std::wstring_view username,
                             int selected, bool allowReplace)
{
    if (username.empty())
        return -1;
    for (std::size_t slot = 0; slot < accounts.size(); ++slot)
        if (accounts[slot].username == username)
            return static_cast<int>(slot);
    const bool validSelection = selected >= 0 && static_cast<std::size_t>(selected) < accounts.size();
    if (validSelection && (allowReplace || accounts[selected].username.empty()))
        return selected;
    for (std::size_t slot = 0; slot < accounts.size(); ++slot)
        if (accounts[slot].username.empty())
            return static_cast<int>(slot);
    return -1;
}
}
