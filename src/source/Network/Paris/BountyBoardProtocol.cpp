#include "stdafx.h"

#include "BountyBoardProtocol.h"

#include "Network/Server/WSclient.h"

#include "Core/Utilities/Log/MuLogger.h"



#include <algorithm>

#include <chrono>

#include <cstring>



namespace Network::Paris::BountyBoard

{

namespace

{

constexpr int RequestLength = 64;

constexpr int HeaderLength = 104;

constexpr int RowLength = 80;

constexpr int MaximumRows = 20;



std::wstring ReadText(const BYTE* bytes, int length)

{

    const auto end = std::find(bytes, bytes + length, BYTE{});

    const int size = static_cast<int>(end - bytes);

    if (size <= 0) return {};

    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,

        reinterpret_cast<const char*>(bytes), size, nullptr, 0);

    if (count <= 0 || count > 256) return {};

    std::wstring result(static_cast<std::size_t>(count), L'\0');

    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,

        reinterpret_cast<const char*>(bytes), size, result.data(), count)) return {};

    return result;

}



int ReadInt32LE(const BYTE* bytes)

{

    return static_cast<int>(static_cast<std::uint32_t>(bytes[0])

        | (static_cast<std::uint32_t>(bytes[1]) << 8)

        | (static_cast<std::uint32_t>(bytes[2]) << 16)

        | (static_cast<std::uint32_t>(bytes[3]) << 24));

}



long long ReadInt64LE(const BYTE* bytes)

{

    std::uint64_t value = 0;

    for (int i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);

    return static_cast<long long>(value);

}

}



Snapshot Data;

DWORD Received = 0;



bool Receive(const BYTE* packet, int size)

{

    if (!packet || size < HeaderLength || packet[0] != 0xC2

        || ((packet[1] << 8) | packet[2]) != size || packet[3] != 0xFA || packet[4] != 0xCF

        || packet[5] != 2 || packet[6] > 1 || packet[7] > MaximumRows

        || size != HeaderLength + packet[7] * RowLength || packet[8] > 4)

        return false;



    Snapshot next;

    next.Enabled = packet[6] != 0;

    next.Operation = packet[8];

    next.Message = ReadText(packet + 9, 95);

    next.Rows.reserve(packet[7]);

    for (int index = 0; index < packet[7]; ++index)

    {

        const BYTE* source = packet + HeaderLength + index * RowLength;

        Row row;

        std::memcpy(row.Id.data(), source, row.Id.size());

        row.Name = ReadText(source + 16, 24);

        row.Hunter = ReadText(source + 40, 23);

        row.Kills = ReadInt32LE(source + 64);

        row.Amount = ReadInt64LE(source + 68);

        row.Currency = source[63];

        row.Ends = static_cast<unsigned long>(source[76])

            | (static_cast<unsigned long>(source[77]) << 8)

            | (static_cast<unsigned long>(source[78]) << 16)

            | (static_cast<unsigned long>(source[79]) << 24);

        if (row.Kills < 0 || row.Kills > 1000000 || row.Amount < 0 || row.Currency > 4)

            return false;

        next.Rows.push_back(std::move(row));

    }



    Data = std::move(next);

    Received = timeGetTime();

    return true;

}



bool Request(BYTE operation, const std::wstring& name, const std::array<BYTE, 16>* id,

    BYTE currency, long long amount)

{

    if (!SocketClient || !SocketClient->IsConnected() || operation > 4 || currency > 4 || amount < 0)

        return false;



    BYTE packet[RequestLength]{};

    packet[0] = 0xC1;

    packet[1] = RequestLength;

    packet[2] = 0xFA;

    packet[3] = 0xCF;

    packet[4] = 2;

    packet[5] = operation;

    if (id) std::memcpy(packet + 6, id->data(), id->size());

    std::memcpy(packet + 22, &amount, sizeof(amount));

    const auto sequence = std::chrono::duration_cast<std::chrono::milliseconds>(

        std::chrono::system_clock::now().time_since_epoch()).count();

    std::memcpy(packet + 30, &sequence, sizeof(sequence));

    packet[38] = currency;



    if (!name.empty())

    {

        const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name.c_str(),

            static_cast<int>(name.size()), nullptr, 0, nullptr, nullptr);

        // Keep one byte for the server-side NUL terminator in the 24-byte name field.

        if (bytes <= 0 || bytes >= 24) return false;

        if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name.c_str(),

            static_cast<int>(name.size()), reinterpret_cast<char*>(packet + 39), 24, nullptr, nullptr))

            return false;

    }

    SocketClient->Send(packet, sizeof(packet));

    if (operation == 3) mu::log::Get("dotnet")->info("Cazarrecompensas: search request sent");

    return true;

}



void Clear()

{

    Data = Snapshot{};

    Received = 0;

}

}

