#include "stdafx.h"
#include "CustomMonsterColorRenderer.h"
#include "World/Monsters/CustomMonsterModels.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "Render/Models/ZzzBMD.h"
#include "../../../ThirdParty/json.hpp"
#include "UI/Core/WindowSystem.h"
#include <array>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cmath>

namespace Render::Models::CustomMonsterColor
{
    namespace
    {
        struct MeshPass
        {
            int mesh, flags;
            float strength;
            std::array<float, 3> color;
        };
        std::unordered_map<int, std::vector<MeshPass>> MeshRules;
        constexpr char MeshConfig[] = "Data/Custom/ParisMonsterMesh.json";

        void RenderMeshEffects(CHARACTER* character)
        {
            const auto found = MeshRules.find(character->MonsterIndex);
            if (found == MeshRules.end()
                || !g_pOption->GetRenderAllEffects()) return;
            auto* object = &character->Object;
            if (object->EnableShadow || object->Alpha < 0.95f) return;
            auto& model = ::Models[object->Type];
            vec3_t saved;
            VectorCopy(model.BodyLight, saved);
            for (const auto& pass : found->second)
            {
                if (pass.mesh >= model.NumMeshs) continue;
                Vector(pass.color[0], pass.color[1], pass.color[2], model.BodyLight);
                model.RenderMesh(pass.mesh, pass.flags, object->Alpha * pass.strength, -1, 1.0f, 0.0f, 0.0f);
            }
            VectorCopy(saved, model.BodyLight);
        }
    }

    void LoadMeshEffects()
    {
        MeshRules.clear();
        std::ifstream file(MeshConfig);
        if (!file) return;
        try
        {
            const auto data = nlohmann::json::parse(file);
            const auto& rows = data.at("rules");
            if (data.value("formatVersion", 0) != 1 || !rows.is_array() || rows.size() > 512)
                throw std::invalid_argument("Invalid monster mesh configuration");
            for (const auto& row : rows)
            {
                const int monster = row.at("monster").get<int>();
                MeshPass pass{row.at("mesh").get<int>(), row.at("flags").get<int>(), row.value("strength", 0.25f),
                    row.at("color").get<std::array<float, 3>>()};
                if (monster < 0 || monster > 32767 || pass.mesh < 0 || pass.mesh > 127
                    || pass.flags != (RENDER_CHROME6 | RENDER_BRIGHT | RENDER_TEXTURE)
                    || !std::isfinite(pass.strength) || pass.strength < 0.0f || pass.strength > 1.0f)
                    throw std::invalid_argument("Unsupported monster mesh pass");
                for (float value : pass.color)
                    if (!std::isfinite(value) || value < 0.0f || value > 1.0f)
                        throw std::invalid_argument("Invalid monster mesh color");
                MeshRules[monster].push_back(pass);
            }
        }
        catch (const std::exception& error)
        {
            MeshRules.clear();
            std::fprintf(stderr, "Monster mesh effects disabled: %s\n", error.what());
        }
    }

    void Render(CHARACTER* character)
    {
        if (character == nullptr || character->Object.Type == MODEL_PLAYER) return;
        RenderMeshEffects(character);
        const auto* color = World::Monsters::Custom::FindColor(character->MonsterIndex);
        if (color == nullptr) return;
        OBJECT* object = &character->Object;
        BMD& model = ::Models[object->Type];
        vec3_t previousLight;
        VectorCopy(model.BodyLight, previousLight);
        Vector((*color)[0], (*color)[1], (*color)[2], model.BodyLight);
        // Use the same metallic overlay as native golden monsters. Restore
        // shared BMD state so the next character keeps its own illumination.
        RenderPartObjectBodyColor(&model, object, object->Type, object->Alpha,
            RENDER_CHROME | RENDER_BRIGHT | RENDER_EXTRA, 1.0f);
        VectorCopy(previousLight, model.BodyLight);
    }
}
