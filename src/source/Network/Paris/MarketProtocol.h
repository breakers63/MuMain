#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Network::Paris::Market
{
struct Listing
{
    std::array<std::uint8_t, 16> id{};
    std::uint32_t revision = 0;
    std::wstring seller;
    std::uint8_t category = 0;
    std::uint8_t currency = 0;
    std::int32_t amount = 0;
    std::vector<std::uint8_t> itemData;
};

struct Snapshot
{
    bool supported = false;
    bool sellerSearch = false;
    bool currencyPricing = false;
    std::uint16_t page = 0;
    std::int32_t total = 0;
    std::wstring status = L"Mercado sin consultar";
    std::vector<Listing> listings;
};

extern Snapshot Data;
bool Receive(const std::uint8_t* packet, int size);
bool RequestCapability();
bool RequestSearch(std::uint16_t page, std::uint8_t category, std::uint8_t currency, const std::wstring& query, std::uint8_t sort = 0);
bool RequestBuy(const Listing& listing);
bool RequestSetPrice(int slot, std::uint8_t currency, int amount);
} // namespace Network::Paris::Market
