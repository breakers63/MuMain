#pragma once
#include <cstdio>
#include <cstddef>

namespace Network::ServerListScript {
// Matches the new Crossplay GameServerDefinition.ServerID in the Paris server configuration.
inline constexpr int CrossplayServerId = 1;
inline constexpr wchar_t CrossplayName[] = L"Crossplay";

template<std::size_t Capacity>
bool ReadDescription(std::FILE* stream, int length, char (&buffer)[Capacity])
{
    if(!stream || length<0 || static_cast<std::size_t>(length)>Capacity)return false;
    return length==0 || std::fread(buffer,1,static_cast<std::size_t>(length),stream)==static_cast<std::size_t>(length);
}
}
