#include "stdafx.h"
#include "GameLogic/Social/ActivityHistoryUpdates.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInterface.h"
#include "GameLogic/Social/ActivityHistory.h"
#include "GameLogic/Social/PostedItemRecord.h"
#include "Network/Server/WSclient.h"
#include "World/MapInfra/MapManager.h"

namespace GameLogic::Social::ActivityHistoryUpdates
{
namespace
{
std::wstring LocalTime()
{
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t text[16]{};
    swprintf_s(text, L"%02u:%02u:%02u", now.wHour, now.wMinute, now.wSecond);
    return text;
}
} // namespace

void RecordPostedItem(const std::wstring& author, const std::wstring& text)
{
    const auto item = DecodePostedItem(text);
    if (!item || item->Type < 0 || item->Type >= MAX_ITEM || !ItemAttribute)
        return;
    const std::wstring description = std::wstring(ItemAttribute[item->Type].Name) + L" +" + std::to_wstring(item->Level);
    ActivityHistory::Posts.Add({LocalTime(), author, description, item->Text, item->Type,
        item->Level, item->Excellent, item->Ancient, gMapManager.WorldActive, false,
        item->Luck, item->AdditionalOption});
}

void RecordPlayerDeath(const unsigned char* packet, int size)
{
    if (!packet || size < static_cast<int>(sizeof(PHEADER_DEFAULT_DIE)))
        return;
    const auto* data = reinterpret_cast<const PHEADER_DEFAULT_DIE*>(packet);
    const int victimIndex = FindCharacterIndex((data->KeyH << 8) | data->KeyL);
    if (victimIndex < 0 || victimIndex >= MAX_CHARACTERS_CLIENT)
        return;
    const auto& victim = CharactersClient[victimIndex];
    if (victim.Object.Kind != KIND_PLAYER || victim.Dead)
        return;

    const int map = gMapManager.WorldActive;
    ActivityHistory::Kills.Add({LocalTime(), victim.ID, L"Derrotado",
        L"Muerte recibida en el area visible.", -1, 0, 0, 0, map});
    const int killerIndex = FindCharacterIndex((data->TKeyH << 8) | data->TKeyL);
    if (killerIndex < 0 || killerIndex >= MAX_CHARACTERS_CLIENT || killerIndex == victimIndex)
        return;
    const auto& killer = CharactersClient[killerIndex];
    if (killer.Object.Live && killer.Object.Kind == KIND_PLAYER)
        ActivityHistory::Kills.Add({LocalTime(), killer.ID, L"Ganador",
            std::wstring(L"Derroto a ") + victim.ID, -1, 0, 0, 0, map, true});
}
} // namespace GameLogic::Social::ActivityHistoryUpdates
