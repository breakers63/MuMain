#pragma once
#include <array>
#include <cwchar>
#include <cstdint>

namespace Network::Paris::ServerLimits
{
extern std::array<int, 5> Values;
extern unsigned int Request;
extern bool Ready;
extern bool Pending;

bool Receive(const wchar_t* text);
} // namespace Network::Paris::ServerLimits
