#include "stdafx.h"
#include "ItemHighlights.h"
#include "../../../ThirdParty/json.hpp"
#include "Render/Models/ZzzBMD.h"
#include "Render/Models/ItemDyeAppearance.h"
#include "Engine/Object/ZzzObject.h"
#include "UI/Core/WindowSystem.h"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace Render::Items::Highlights
{
    namespace
    {
        constexpr char ConfigurationPath[] = "Data/Custom/ParisSetAppearance.json";
        constexpr int MinimumEquipmentLevel = 13;
        constexpr float MaximumStrength = 0.3f;
        float Strength = 0.16f;

        bool CanRender(OBJECT* object, int type, int level, float alpha, int flags)
        {
            if (!object || object->EnableShadow || level < MinimumEquipmentLevel || alpha < 0.95f
                || Strength <= 0.0f || (flags & RENDER_WAVE) || !g_pOption->GetRenderAllEffects()
                || g_pOption->GetRenderLevel() < 4 || type < MODEL_HELM || type >= MODEL_BOOTS + MAX_ITEM_INDEX)
                return false;
            vec3_t white = {1.0f, 1.0f, 1.0f}, tint;
            return !Render::ItemDye::Light(object, type, white, tint);
        }
    }

    void Load()
    {
        std::ifstream file(ConfigurationPath);
        if (!file) return;
        try
        {
            const auto data = nlohmann::json::parse(file);
            const float value = data.value("neutralHighlightStrength", 0.16f);
            Strength = std::isfinite(value) ? std::clamp(value, 0.0f, MaximumStrength) : 0.0f;
            if (!data.value("enabled", true)) Strength = 0.0f;
        }
        catch (const std::exception&)
        {
            Strength = 0.0f;
        }
    }

    void Render(OBJECT* object, int modelType, int itemLevel, float alpha, int renderFlags)
    {
        if (!CanRender(object, modelType, itemLevel, alpha, renderFlags)) return;
        auto& model = Models[modelType];
        vec3_t saved;
        VectorCopy(model.BodyLight, saved);
        Vector(Strength, Strength, Strength, model.BodyLight);
        model.RenderBody(RENDER_CHROME4 | RENDER_BRIGHT | (renderFlags & RENDER_EXTRA),
            alpha, -1, 1.0f, object->BlendMeshTexCoordU, object->BlendMeshTexCoordV, object->HiddenMesh);
        VectorCopy(saved, model.BodyLight);
    }
}
