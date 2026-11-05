#pragma once

#include <string>

namespace GameLogic::Social::ActivityHistoryUpdates
{
void RecordPostedItem(const std::wstring& author, const std::wstring& text);
void RecordPlayerDeath(const unsigned char* packet, int size);
} // namespace GameLogic::Social::ActivityHistoryUpdates
