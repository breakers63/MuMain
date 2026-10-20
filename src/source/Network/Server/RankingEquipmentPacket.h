#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include "Network/Server/ExtendedItemData.h"

namespace Network::Ranking
{
    constexpr std::size_t EquipmentSlots = 12;
    constexpr std::size_t EquipmentRecordSize = 34;
    constexpr std::size_t EquipmentResponseSize = 427;
    constexpr std::uint8_t EquipmentRequest = 0x5C;
    constexpr std::uint8_t EquipmentResponse = 0x5D;

    inline bool IsEquipmentResponse(std::span<const std::uint8_t> packet)
    {
        if (packet.size() != EquipmentResponseSize || packet[0] != 0xC2
            || ((packet[1] << 8) | packet[2]) != EquipmentResponseSize
            || packet[3] != 0xED || packet[4] != EquipmentResponse
            || packet[5] != 1 || packet[6] > 1 || packet[7] > 1 || packet[18] > 1)
            return false;

        for (std::size_t slot = 0; slot < EquipmentSlots; ++slot)
        {
            const auto record = packet.subspan(19 + slot * EquipmentRecordSize, EquipmentRecordSize);
            if (record[0] > 1 || (record[0] && (record[1] < 5 || record[1] > 32)))
                return false;
            if (record[0] && !Network::Items::ExtendedItemLength(record.subspan(2, record[1])))
                return false;
        }
        return true;
    }
}
