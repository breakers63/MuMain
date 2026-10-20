#pragma once
#include <string>
namespace UI::ActivityLogs
{
void RecordPostedItem(const std::wstring& author, const std::wstring& text);
void RecordPlayerDeath(const BYTE* packet, int size);
}
