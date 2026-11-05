#pragma once
#include <string>
#include <vector>
namespace Network::Paris::ItemDye
{
struct Entry { std::string Id; std::wstring Name; int Slot=0, Type=0, Level=0, Color=-1; bool Eligible=false; };
extern std::vector<Entry> Inventory;
extern unsigned Received, Revision;
extern int Result;
extern bool Pending;
extern bool RequireReagents;
extern int PotionsPerColor;
void Reset();
void Request();
void Apply(const Entry& item, const int* reagentSlots, int color);
void RestoreOriginal(const Entry& item);
bool Receive(const BYTE* bytes, int size);
}
