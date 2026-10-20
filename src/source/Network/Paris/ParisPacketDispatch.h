#pragma once
#include <cstdint>
namespace Network::Paris
{
void Dispatch(const std::uint8_t* packet, int size);
}
