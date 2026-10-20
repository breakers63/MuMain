#include "stdafx.h"
#include "CustomMonsterColorRenderer.h"
#include "World/Monsters/CustomMonsterModels.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "Render/Models/ZzzBMD.h"

namespace Render::Models::CustomMonsterColor
{
    void Render(CHARACTER* character)
    {
        if (character == nullptr || character->Object.Type == MODEL_PLAYER) return;
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
