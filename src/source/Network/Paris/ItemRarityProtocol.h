#pragma once

namespace Network::Paris::ItemRarity
{
void Refresh();
bool Receive(const unsigned char* packet, int size);
}
