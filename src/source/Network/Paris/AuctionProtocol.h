#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

namespace Network::Paris::Auctions
{
    struct Row
    {
        std::string Id, DeliveryId, SourceId, State;
        std::wstring Name, Seller;
        std::vector<BYTE> ItemData;
        long long Revision = 0, Ends = 0, Time = 0;
        int Code = 0, Level = 0, Slot = 0, Currency = 0, Price = 0, BuyNow = 0, NextBid = 0, Amount = 0, Status = 0;
        bool Owned = false, CanCancel = false, Highest = false, ItemDelivery = false;
    };
    struct Snapshot
    {
        int Tab = 0, Page = 0, SubTab = 0, Decision = 0, Action = 0, Fee = 0, Group = 255;
        std::wstring Query;
        long long ServerTime = 0;
        bool Enabled = false, More = false;
        std::vector<int> Durations, Currencies;
        std::vector<Row> Rows;
        std::map<std::string, std::wstring> Labels;
    };
    extern Snapshot Data;
    extern DWORD Received;
    bool Receive(const BYTE* packet, int size);
    void Clear();
    const std::wstring& Label(const char* key);
    bool Request(BYTE action, BYTE tab, BYTE page, BYTE subTab, const std::wstring& query, BYTE group,
        const Row* row = nullptr, int price = 0, int buyNow = 0, int duration = 0, BYTE currency = 0);
}
