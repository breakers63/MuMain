#pragma once
#include <span>
#include <cstdint>
namespace Network::Ranking
{
inline int MissionEntryCount(std::span<const std::uint8_t> packet)
{
    if(packet.size()<11||packet[0]!=0xC2||packet[3]!=0xED||packet[4]!=0x61||packet[5]!=1)return -1;
    const int count=packet[10],size=(packet[1]<<8)|packet[2];
    return count<=50&&size==packet.size()&&size==11+count*18?count:-1;
}
}
