#pragma once
#include <cstdint>

namespace Network::Ranking
{
enum class GuildRosterRetry { Wait, LegacyRequest, Unavailable };
constexpr std::uint32_t GuildRosterLegacyDelay = 1500;
constexpr std::uint32_t GuildRosterResponseTimeout = 4000;

constexpr GuildRosterRetry GuildRosterRetryAction(std::uint32_t now, std::uint32_t sentAt, int version)
{
    const auto elapsed = now - sentAt;
    if (version == 2)
        return elapsed >= GuildRosterLegacyDelay ? GuildRosterRetry::LegacyRequest : GuildRosterRetry::Wait;
    return elapsed >= GuildRosterResponseTimeout ? GuildRosterRetry::Unavailable : GuildRosterRetry::Wait;
}
}
