#include "stdafx.h"
#include "SetTrails.h"
#include "../../../ThirdParty/json.hpp"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "Render/Effects/ZzzEffect.h"
#include "UI/Core/WindowSystem.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace Render::Items::SetTrails
{
    namespace
    {
        constexpr char ConfigPath[] = "Data/Custom/ParisSetAppearance.json";
        constexpr float ReferenceFps = 25.0f;
        constexpr int Subtype = 18;
        bool Enabled = true;
        int MinimumLevel = 13;
        float PairsPerSecond = 1.25f;
        float Width = 23.0f;
        float SpeedMultiplier = 1.35f;

        float ReadRange(const nlohmann::json& data, const char* key, float fallback, float minimum, float maximum)
        {
            const float value = data.value(key, fallback);
            if (!std::isfinite(value) || value < minimum || value > maximum)
                throw std::invalid_argument("Invalid set trail configuration");
            return value;
        }

        bool CanRender(OBJECT* object)
        {
            return object->Type == MODEL_PLAYER && object->SubType == 0 && !object->EnableShadow
                && object->Alpha >= 0.95f && (object->Kind == KIND_PLAYER || object->Kind == KIND_NPC)
                && g_pOption->GetRenderAllEffects() && g_pOption->GetRenderLevel() >= 3
                && !g_isCharacterBuff(object, eBuff_Cloaking) && !g_isCharacterBuff(object, eDeBuff_Harden);
        }
    }

    void Load()
    {
        std::ifstream file(ConfigPath);
        if (!file) return;
        try
        {
            const auto data = nlohmann::json::parse(file);
            if (data.value("formatVersion", 0) != 1) throw std::invalid_argument("Invalid set trail format");
            MinimumLevel = static_cast<int>(ReadRange(data, "minimumLevel", 13.0f, 0.0f, 15.0f));
            PairsPerSecond = ReadRange(data, "trailPairsPerSecond", 1.25f, 0.0f, 3.0f);
            Width = ReadRange(data, "trailWidth", 23.0f, 0.0f, 30.0f);
            SpeedMultiplier = ReadRange(data, "trailSpeed", 1.35f, 1.0f, 2.0f);
            Enabled = data.value("enabled", true);
        }
        catch (const std::exception& error)
        {
            Enabled = false;
            std::fprintf(stderr, "Set trails disabled: %s\n", error.what());
        }
    }

    float Speed() { return SpeedMultiplier; }

    bool Render(CHARACTER* character, int minimumEquipmentLevel)
    {
        if (!Enabled || !character || minimumEquipmentLevel < MinimumLevel || PairsPerSecond <= 0.0f) return false;
        auto* object = &character->Object;
        if (!CanRender(object)) return true;
        const int interval = std::max(1, static_cast<int>(std::round(ReferenceFps / PairsPerSecond)));
        if (!rand_fps_check(interval)) return true;
        vec3_t savedLight;
        VectorCopy(object->Light, savedLight);
        Vector(1.0f, 1.0f, 1.0f, object->Light);
        for (int side = 0; side < 2; ++side)
            CreateJoint(BITMAP_FLARE, object->Position, object->Position, object->Angle,
                Subtype, object, Width, Marker, side);
        VectorCopy(savedLight, object->Light);
        return true;
    }
}
