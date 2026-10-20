#pragma once

#include <string>
#include <utility>
#include <vector>

namespace Network::Paris::Reset
{
struct Snapshot
{
    bool Available = false;
    bool CanReset = false;
    unsigned long long Token = 0;
    int NextReset = 0;
    std::vector<std::wstring> Lines;
};

// The network callback receives snapshots off the UI thread. Consumers take an atomic copy so
// strings and line vectors are never read while a packet replaces them.
std::pair<Snapshot, unsigned int> GetSnapshot();
bool Receive(const unsigned char* packet, int size);
} // namespace Network::Paris::Reset
