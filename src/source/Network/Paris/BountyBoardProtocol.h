#pragma once

#include <array>
#include <string>
#include <vector>

namespace Network::Paris::BountyBoard
{
    struct Row
    {
        std::array<BYTE, 16> Id{};
        std::wstring Name;
        std::wstring Hunter;
        int Kills = 0;
        long long Amount = 0;
        BYTE Currency = 0;
        unsigned long Ends = 0;
    };

    struct Snapshot
    {
        bool Enabled = false;
        BYTE Operation = 0;
        std::wstring Message;
        std::vector<Row> Rows;
    };

    extern Snapshot Data;
    extern DWORD Received;

    bool Receive(const BYTE* packet, int size);
    bool Request(BYTE operation, const std::wstring& name = {}, const std::array<BYTE, 16>* id = nullptr,
        BYTE currency = 0, long long amount = 0);
    void Clear();
}
