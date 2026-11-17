#pragma once
struct tagITEM;
namespace ParisItemTrace { struct Cached; }
namespace Network::Paris::ItemTrace {
void Reset();
const ParisItemTrace::Cached* Query(const tagITEM& item);
bool Receive(const unsigned char* packet,int size);
}
