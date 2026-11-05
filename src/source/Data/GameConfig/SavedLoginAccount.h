#pragma once

#include <cstddef>
#include <string>

inline constexpr std::size_t SavedLoginAccountLimit = 5;

struct SavedLoginAccount
{
    std::wstring username;
    std::wstring password;
};
