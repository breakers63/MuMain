#include "stdafx.h"
#include "ActivityHistoryHooks.h"
#include "ParisActivityLogs.h"
#include "GameLogic/Social/PostedItemRecord.h"
#include "Network/Server/WSclient.h"
#include "World/MapInfra/MapManager.h"

namespace UI::ActivityLogs
{
void RecordPostedItem(const std::wstring& author, const std::wstring& text)
{
    const auto record = GameLogic::Social::DecodePostedItem(text);
    if (!record || record->Type >= MAX_ITEM) return;
    const std::wstring caption = std::wstring(ItemAttribute[record->Type].Name) + L" +" + std::to_wstring(record->Level);
    AddPost(author, caption, record->Text, record->Type, record->Level, record->Excellent,
        record->Ancient, record->Luck, record->AdditionalOption);
}
void RecordPlayerDeath(const BYTE* packet, int size)
{
    if (!packet || size < sizeof(PHEADER_DEFAULT_DIE)) return;
    const auto* data = reinterpret_cast<const PHEADER_DEFAULT_DIE*>(packet);
    const int victim = FindCharacterIndex((data->KeyH << 8) | data->KeyL);
    if (victim < 0 || victim >= MAX_CHARACTERS_CLIENT) return;
    const auto& character = CharactersClient[victim];
    if (character.Object.Kind != KIND_PLAYER || character.Dead) return;
    AddKill(character.ID, gMapManager.WorldActive);
    const int killer = FindCharacterIndex((data->TKeyH << 8) | data->TKeyL);
    if (killer < 0 || killer >= MAX_CHARACTERS_CLIENT || killer == victim) return;
    if (CharactersClient[killer].Object.Live && CharactersClient[killer].Object.Kind == KIND_PLAYER)
        AddVictory(CharactersClient[killer].ID, character.ID, gMapManager.WorldActive);
}
}
