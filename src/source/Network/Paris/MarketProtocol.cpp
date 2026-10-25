#include "stdafx.h"
#include "GameLogic/Items/MarketPrice.h"
#include "Network/Paris/MarketProtocol.h"
#include "Network/Server/ExtendedItemData.h"

#include "Network/Server/SocketSystem.h"
#include "Network/Server/WSclient.h"

#include <algorithm>
#include <cstring>
#include <objbase.h>

namespace Network::Paris::Market
{
namespace
{
constexpr std::uint8_t Head = 0xFA;
constexpr std::uint8_t Version = 1;
constexpr std::uint8_t Capability = 0xE2;
constexpr std::uint8_t Open = 0xE3;
constexpr std::uint8_t Search = 0xE4;
constexpr std::uint8_t SearchResults = 0xE5;
constexpr std::uint8_t Buy = 0xE6;
constexpr std::uint8_t BuyResult = 0xE7;
constexpr std::uint8_t Error = 0xEB;
constexpr std::uint8_t AnyFilter = 0xFF;
constexpr int ListingHeaderSize = 54;

bool Connected()
{
    return SocketClient && SocketClient->IsConnected();
}

bool ReadUtf8(const std::uint8_t* data, int size, std::wstring& output)
{
    if (!data || size <= 0)
        return false;
    int textSize = 0;
    while (textSize < size && data[textSize] != 0)
        ++textSize;
    if (textSize == 0)
        return false;
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char*>(data), textSize, nullptr, 0);
    if (length <= 0 || length > 64)
        return false;
    output.resize(static_cast<std::size_t>(length));
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char*>(data), textSize, output.data(), length) == length;
}

bool SendOpen()
{
    constexpr std::uint8_t packet[] = {0xC1, 5, Head, Open, Version};
    if (!Connected())
        return false;
    SocketClient->Send(packet, sizeof(packet));
    Data.status = L"Abriendo Market…";
    return true;
}
} // namespace

Snapshot Data;

bool RequestCapability()
{
    constexpr std::uint8_t packet[] = {0xC1, 5, Head, Capability, Version};
    if (!Connected())
    {
        Data.status = L"Sin conexión con el servidor";
        return false;
    }
    Data = {};
    Data.status = L"Negociando Market…";
    SocketClient->Send(packet, sizeof(packet));
    return true;
}

bool RequestSearch(std::uint16_t page, std::uint8_t category, std::uint8_t currency, const std::wstring& query, std::uint8_t sort)
{
    if (!Connected() || !Data.supported || query.size() > 64 || sort > 1
        || (category != AnyFilter && category > 15) || (currency != AnyFilter && currency > 9))
        return false;
    const int utf8Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, query.c_str(),
        static_cast<int>(query.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Length < 0 || utf8Length > 64)
        return false;
    std::string utf8(static_cast<std::size_t>(utf8Length), '\0');
    if (utf8Length > 0 && WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, query.c_str(),
            static_cast<int>(query.size()), utf8.data(), utf8Length, nullptr, nullptr) != utf8Length)
        return false;

    const int packetSize = 12 + utf8Length;
    std::vector<std::uint8_t> packet(static_cast<std::size_t>(packetSize));
    packet[0] = 0xC1;
    packet[1] = static_cast<std::uint8_t>(packetSize);
    packet[2] = Head;
    packet[3] = Search;
    packet[4] = Version;
    packet[5] = static_cast<std::uint8_t>(page & 0xFF);
    packet[6] = static_cast<std::uint8_t>(page >> 8);
    packet[7] = 5;
    packet[8] = category;
    packet[9] = currency;
    packet[10] = sort;
    packet[11] = static_cast<std::uint8_t>(utf8Length);
    std::copy(utf8.begin(), utf8.end(), packet.begin() + 12);
    SocketClient->Send(packet.data(), packetSize);
    Data.status = L"Buscando publicaciones…";
    return true;
}

bool RequestBuy(const Listing& listing)
{
    if (!Connected() || !Data.supported || listing.amount <= 0)
        return false;
    GUID requestId{};
    if (FAILED(CoCreateGuid(&requestId)))
        return false;
    std::uint8_t packet[41] = {0xC1, 41, Head, Buy, Version};
    std::memcpy(packet + 5, &requestId, sizeof(requestId));
    std::memcpy(packet + 21, listing.id.data(), listing.id.size());
    std::memcpy(packet + 37, &listing.revision, sizeof(listing.revision));
    SocketClient->Send(packet, sizeof(packet));
    Data.status = L"Compra enviada; esperando confirmación del servidor";
    return true;
}

bool RequestSetPrice(int slot, std::uint8_t currency, int amount)
{
    if (!Connected() || !Data.supported || !Data.currencyPricing || slot < 0 || slot > 255
        || !GameLogic::Items::MarketPrice::ToPrice(currency, amount)) return false;
    constexpr std::uint8_t SetPrice = 0xE8;
    std::uint8_t packet[11] = {0xC1, 11, Head, SetPrice, Version, static_cast<std::uint8_t>(slot), currency};
    const auto value = static_cast<std::uint32_t>(amount);
    std::memcpy(packet + 7, &value, sizeof(value));
    SocketClient->Send(packet, sizeof(packet));
    Data.status = L"Guardando precio y moneda…";
    return true;
}

bool Receive(const std::uint8_t* packet, int size)
{
    if (!packet || size < 5)
        return false;
    const bool longHeader = packet[0] == 0xC2 || packet[0] == 0xC4;
    const bool shortHeader = packet[0] == 0xC1 || packet[0] == 0xC3;
    if (!longHeader && !shortHeader)
        return false;
    const int headOffset = longHeader ? 3 : 2;
    const int subOffset = headOffset + 1;
    const int payloadOffset = subOffset + 1;
    const int declaredSize = longHeader ? (packet[1] << 8) | packet[2] : packet[1];
    if (declaredSize != size || size <= payloadOffset || packet[headOffset] != Head)
        return false;

    const std::uint8_t subcode = packet[subOffset];
    if (packet[payloadOffset] != Version)
        return false;
    if (subcode == Capability)
    {
        if (size < payloadOffset + 2)
            return false;
        Data.supported = packet[payloadOffset + 1] != 0;
        Data.sellerSearch = size >= payloadOffset + 3 && (packet[payloadOffset + 2] & 1) != 0;
        Data.currencyPricing = size >= payloadOffset + 3 && (packet[payloadOffset + 2] & 2) != 0;
        Data.status = Data.supported ? L"Market disponible" : L"El servidor no habilitó Market";
        return Data.supported ? SendOpen() : true;
    }
    if (subcode == SearchResults)
    {
        const auto* payload = packet + payloadOffset;
        const int payloadSize = size - payloadOffset;
        if (payloadSize < 9)
            return false;
        const int count = payload[8];
        if (count > 5)
            return false;
        Snapshot next = Data;
        next.page = static_cast<std::uint16_t>(payload[1] | (payload[2] << 8));
        std::memcpy(&next.total, payload + 4, sizeof(next.total));
        if (next.total < 0 || next.total > 1000000)
            return false;
        next.listings.clear();
        int offset = 9;
        for (int i = 0; i < count; ++i)
        {
            if (offset + ListingHeaderSize > payloadSize)
                return false;
            Listing listing;
            std::copy(payload + offset, payload + offset + 16, listing.id.begin());
            std::memcpy(&listing.revision, payload + offset + 16, sizeof(listing.revision));
            if (!ReadUtf8(payload + offset + 36, 10, listing.seller))
                listing.seller = L"Vendedor";
            listing.category = payload[offset + 47];
            listing.currency = payload[offset + 48];
            std::memcpy(&listing.amount, payload + offset + 49, sizeof(listing.amount));
            const int itemLength = payload[offset + 53];
            if (listing.amount < 0 || itemLength < 5 || offset + ListingHeaderSize + itemLength > payloadSize)
                return false;
            const auto item = std::span<const std::uint8_t>(payload + offset + ListingHeaderSize, itemLength);
            const auto itemSize = Network::Items::ExtendedItemLength(item);
            if (!itemSize || itemLength > 32)
                return false;
            listing.itemData.assign(item.begin(), item.end());
            next.listings.push_back(std::move(listing));
            offset += ListingHeaderSize + itemLength;
        }
        if (offset != payloadSize)
            return false;
        next.status = next.listings.empty() ? L"No hay publicaciones" : L"Publicaciones actualizadas";
        Data = std::move(next);
        return true;
    }
    if (subcode == BuyResult)
    {
        Data.status = size > payloadOffset + 1 && packet[payloadOffset + 1] <= 1
            ? L"Compra completada" : L"Compra rechazada o publicación actualizada";
        return true;
    }
    if (subcode == 0xE9)
    {
        if (size < payloadOffset + 4) return false;
        Data.status = packet[payloadOffset + 1] == 1 ? L"Precio y moneda guardados" : L"El servidor rechazó el precio";
        return true;
    }
    if (subcode == Error)
    {
        Data.status = L"Solicitud de Market inválida o no autorizada";
        return true;
    }
    return false;
}
} // namespace Network::Paris::Market
