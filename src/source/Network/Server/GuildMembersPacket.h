#pragma once
#include <span>
#include <cstdint>
namespace Network::Ranking {
    constexpr int GuildMembersHeaderSize=18, GuildMemberSize=28, MaximumGuildMembers=1000;
    inline int GuildMemberCount(std::span<const std::uint8_t> packet)
    {
        if(packet.size()<GuildMembersHeaderSize || packet[0]!=0xC2 || packet[3]!=0xED || packet[4]!=0x5F || (packet[5]!=1 && packet[5]!=2))return -1;
        const auto size=(packet[1]<<8)|packet[2];
        const int count=packet[16]|(packet[17]<<8);
        const int recordSize = packet[5] == 2 ? 50 : GuildMemberSize;
        if(count>MaximumGuildMembers || size!=packet.size() || size!=GuildMembersHeaderSize+count*recordSize)return -1;
        return count;
    }
}
