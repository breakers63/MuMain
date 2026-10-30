#include "stdafx.h"
#include "Network/Paris/BossDamageProtocol.h"
#include "World/MapInfra/MapManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "UI/Combat/TargetHealthView.h"
#include "UI/Combat/TargetHealthRmlModel.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlSyncField.h"
#include "Core/Utilities/StringUtils.h"
#include "Data/GameConfig/GameConfig.h"
#include <RmlUi/Core/ElementDocument.h>
#include <algorithm>
#include <cmath>

namespace
{
UI::RmlBridge::ThemedView<TargetHealthModel> targetView{
    "target_health", [](Rml::DataModelConstructor& constructor, TargetHealthModel& model)
    {
        constructor.Bind("name", &model.name);
        constructor.Bind("remaining", &model.remaining);
        constructor.Bind("percent", &model.percent);
        constructor.Bind("ranking", &model.ranking);
    }, {{"Data/Interface/RmlUi/target_health.rml"}}};

bool IsBoss(int number)
{
    return Network::Paris::BossDamage::IsBoss(number, gMapManager.WorldActive);
}

CHARACTER* Target()
{
    if (SelectedCharacter >= 0 && SelectedCharacter < MAX_CHARACTERS_CLIENT)
    {
        auto* selected = &CharactersClient[SelectedCharacter];
        if (selected->Object.Live && selected->Object.Visible && selected->Dead == 0
            && selected->Object.Kind == KIND_MONSTER && IsBoss(selected->MonsterIndex)) return selected;
    }
    CHARACTER* closest = nullptr;
    float distance = 0.f;
    if (!Hero) return nullptr;
    for (int index = 0; index < MAX_CHARACTERS_CLIENT; ++index)
    {
        auto* candidate = &CharactersClient[index];
        if (!candidate->Object.Live || !candidate->Object.Visible || candidate->Dead != 0
            || candidate->Object.Kind != KIND_MONSTER || !IsBoss(candidate->MonsterIndex)) continue;
        const float dx = candidate->Object.Position[0] - Hero->Object.Position[0];
        const float dy = candidate->Object.Position[1] - Hero->Object.Position[1];
        const float squared = dx * dx + dy * dy;
        if (!closest || squared < distance) { closest = candidate; distance = squared; }
    }
    return closest;
}
}

void UI::Combat::TargetHealthView::SyncTarget(bool visible)
{
    auto* target = visible ? Target() : nullptr;
    if (!target || GameConfig::GetInstance().GetHideMonsterHP()) { Hide(); return; }
    targetView.Ensure();
    if (!targetView.Document()) return;
    const float percent = std::clamp(target->HealthStatus < 0.f ? 1.f : target->HealthStatus, 0.f, 1.f) * 100.f;
    auto& binder = targetView.Binder();
    SyncField(binder, &TargetHealthModel::name, "name", StringUtils::WideToNarrow(target->ID));
    SyncField(binder, &TargetHealthModel::percent, "percent", percent);
    SyncField(binder, &TargetHealthModel::remaining, "remaining", Rml::String(std::to_string(static_cast<int>(std::ceil(percent))) + "%"));
    const auto* ranking = Network::Paris::BossDamage::Find(target->Key, target->MonsterIndex);
    SyncField(binder, &TargetHealthModel::ranking, "ranking", ranking ? Network::Paris::BossDamage::Markup(*ranking) : Rml::String{});
    if (ranking)
    {
        SyncField(binder, &TargetHealthModel::name, "name", ranking->name);
        SyncField(binder, &TargetHealthModel::remaining, "remaining", Rml::String(std::to_string(ranking->health) + " / " + std::to_string(ranking->maximum)));
    }
    UI::RmlBridge::SyncDocumentVisibility(targetView.Document(), true);
}
void UI::Combat::TargetHealthView::Hide() { UI::RmlBridge::SyncDocumentVisibility(targetView.Document(), false); }
void UI::Combat::TargetHealthView::Release() { targetView.Release(); }
