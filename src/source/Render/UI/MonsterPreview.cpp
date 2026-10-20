#include "stdafx.h"
#include "MonsterPreview.h"
#include "CharacterPreviewCamera.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzOpenData.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "World/MapInfra/MapManager.h"
#include "World/Monsters/CustomMonsterModels.h"
#include <memory>
namespace Render::UI::MonsterPreview
{
namespace
{
    std::unique_ptr<CHARACTER> character;
    int monster=-1, world=-1;
    float scale=.5f, offset=-18.f;
    static void AI_SetPreviewAction(OBJECT* object, int action)
    {
        if (object == nullptr || object->Type < 0)
        {
            return;
        }

        if (Models[object->Type].NumActions <= action)
        {
            action = 0;
        }

        object->CurrentAction = action;
        object->PriorAction = action;
        object->AnimationFrame = 0.0f;
        object->PriorAnimationFrame = 0.0f;
        Models[object->Type].CurrentAction = action;

        if (Models[object->Type].NumActions > action)
        {
            Models[object->Type].Actions[action].Loop = true;
        }
    }

    static bool AI_GetPreviewModelInfo(
        unsigned short monsterNumber,
        int& modelType,
        float& renderScale,
        float& translateZ)
    {
        modelType = -1;
        renderScale = 0.52f;
        translateZ = -18.0f;

        switch (monsterNumber)
        {
        case 43: // Golden Budge Dragon
            modelType = MODEL_BUDGE_DRAGON;
            renderScale = 0.62f;
            translateZ = -18.0f;
            break;
        case 44: // Red Dragon
            modelType = MODEL_DRAGON_;
            renderScale = 0.44f;
            translateZ = -30.0f;
            break;
        case 53: // Golden Titan
            modelType = MODEL_TITAN;
            renderScale = 0.43f;
            translateZ = -18.0f;
            break;
        case 54: // Golden Soldier
            modelType = MODEL_SOLDIER;
            renderScale = 0.62f;
            translateZ = -10.0f;
            break;
        case 78: // Golden Goblin
            modelType = MODEL_GOBLIN;
            renderScale = 0.76f;
            translateZ = -6.0f;
            break;
        case 79: // Golden Dragon
            modelType = MODEL_DRAGON_;
            renderScale = 0.47f;
            translateZ = -28.0f;
            break;
        case 80: // Golden Lizard King
            modelType = MODEL_LIZARD;
            renderScale = 0.48f;
            translateZ = -13.0f;
            break;
        case 81: // Golden Vepar
            modelType = MODEL_VEPAR;
            renderScale = 0.58f;
            translateZ = -10.0f;
            break;
        case 82: // Golden Tantallos
            modelType = MODEL_TANTALLOS;
            renderScale = 0.40f;
            translateZ = -20.0f;
            break;
        case 83: // Golden Wheel
            modelType = MODEL_GOLDEN_WHEEL;
            renderScale = 0.48f;
            translateZ = -15.0f;
            break;
        case 135: // White Wizard
            modelType = MODEL_CURSED_KING;
            renderScale = 0.48f;
            translateZ = -18.0f;
            break;
        case 136: // Destructive Ogre Soldier
            modelType = MODEL_ORC;
            renderScale = 0.60f;
            translateZ = -10.0f;
            break;
        case 137: // Destructive Ogre Archer
    modelType = MODEL_ORC_ARCHER;
    renderScale = 0.60f;
    translateZ = -10.0f;
    break;

case 295: // Erohim
    modelType = MODEL_EROHIM;
    renderScale = 0.50f;
    translateZ = -18.0f;
    break;

case 275: // Lord of Kundun
    modelType = MODEL_ILLUSION_OF_KUNDUN;
    renderScale = 0.48f;
    translateZ = -18.0f;
    break;

default:
    return false;
        }

        return modelType >= 0;
    }

    static void AI_LoadPreviewModel(unsigned short monsterNumber)
    {
        switch (monsterNumber)
        {
        case 43:
            OpenMonsterModel(MONSTER_MODEL_BUDGE_DRAGON);
            break;
        case 44:
        case 79:
            OpenMonsterModel(MONSTER_MODEL_DRAGON);
            break;
        case 53:
            OpenMonsterModel(MONSTER_MODEL_TITAN);
            break;
        case 54:
            OpenMonsterModel(MONSTER_MODEL_SOLDIER);
            break;
        case 78:
            OpenMonsterModel(MONSTER_MODEL_GOBLIN);
            break;
        case 80:
            OpenMonsterModel(MONSTER_MODEL_LIZARD);
            break;
        case 81:
            OpenMonsterModel(MONSTER_MODEL_VEPAR);
            break;
        case 82:
            OpenMonsterModel(MONSTER_MODEL_TANTALLOS);
            break;
        case 83:
            OpenMonsterModel(MONSTER_MODEL_GOLDEN_WHEEL);
            break;
        case 135:
            OpenMonsterModel(MONSTER_MODEL_CURSED_KING);
            break;
        case 136:
            OpenMonsterModel(MONSTER_MODEL_ORC);
            break;
case 137:
    OpenMonsterModel(MONSTER_MODEL_ORC_ARCHER);
    break;

case 295:
    OpenMonsterModel(MONSTER_MODEL_EROHIM);
    break;

case 275:
    OpenMonsterModel(MONSTER_MODEL_ILLUSION_OF_KUNDUN);
    break;

default:
    break;
        }
    }

    static void AI_ConfigurePreviewCharacter(
        CHARACTER* character,
        unsigned short monsterNumber)
    {
        if (character == nullptr)
        {
            return;
        }

        OBJECT* object = &character->Object;
        character->MonsterIndex = static_cast<EMonsterType>(monsterNumber);
        character->HideShadow = true;
        character->SafeZone = false;
        character->Dead = 0;
        character->Movement = false;
        character->NotRotateOnMagicHit = false;
        character->Weapon[0].Type = -1;
        character->Weapon[0].Level = 0;
        character->Weapon[0].ExcellentFlags = 0;
        character->Weapon[1].Type = -1;
        character->Weapon[1].Level = 0;
        character->Weapon[1].ExcellentFlags = 0;

        object->Kind = KIND_MONSTER;
        object->Live = true;
        object->Visible = true;
        object->EnableShadow = false;
        object->m_bRenderShadow = false;
        object->HiddenMesh = -1;
        object->BlendMesh = -1;
        object->BlendMeshLight = 1.0f;
        object->SubType = 0;

        switch (monsterNumber)
        {
        case 43:
            object->Scale = 0.7f;
            break;
        case 44:
            object->Scale = 1.3f;
            Vector(200.0f, 150.0f, 280.0f, object->BoundingBoxMax);
            break;
        case 53:
            object->Scale = 1.8f;
            object->BlendMesh = 2;
            object->BlendMeshLight = 1.0f;
            break;
        case 54:
            object->Scale = 1.1f;
            character->Weapon[1].Type = MODEL_AQUAGOLD_CROSSBOW;
            break;
        case 78:
            object->Scale = 0.8f;
            character->Weapon[0].Type = MODEL_AXE;
            character->Weapon[0].Level = 9;
            break;
        case 79:
            object->Scale = 0.9f;
            Vector(200.0f, 150.0f, 280.0f, object->BoundingBoxMax);
            break;
        case 80:
            object->Scale = 1.4f;
            character->Weapon[0].Type = MODEL_CHAOS_LIGHTNING_STAFF;
            character->Weapon[0].ExcellentFlags = 63;
            break;
        case 81:
            object->Scale = 1.0f;
            break;
        case 82:
            object->Scale = 1.8f;
            object->BlendMesh = 2;
            object->BlendMeshLight = 1.0f;
            character->Weapon[0].Type = MODEL_SWORD_OF_DESTRUCTION;
            character->Weapon[0].ExcellentFlags = 63;
            break;
        case 83:
            object->Scale = 1.4f;
            character->Weapon[0].Type = MODEL_AQUAGOLD_CROSSBOW;
            character->Weapon[0].ExcellentFlags = 63;
            break;
        case 135:
            object->Scale = 1.7f;
            break;
        case 136:
            object->Scale = 1.3f;
            object->HiddenMesh = 2;
            break;

        case 137:
            object->Scale = 1.2f;
            object->HiddenMesh = 1;
            character->Weapon[1].Type = MODEL_BATTLE_BOW;
            character->Weapon[1].Level = 5;
            break;

        // MUETRO_ACTIVE_INVASIONS_EROHIM_KUNDUN_PREVIEW_V1
        case 275: // Lord of Kundun
            object->Scale = 2.0f;
            object->HiddenMesh = -1;
            object->BlendMesh = -1;
            object->SubType = 0;

            character->Weapon[0].Type = -1;
            character->Weapon[0].Level = 0;
            character->Weapon[0].LinkBone = 29;

            character->Weapon[1].Type = MODEL_STAFF_OF_KUNDUN;
            character->Weapon[1].Level = 0;
            character->Weapon[1].ExcellentFlags = 0;
            character->Weapon[1].LinkBone = 49;
            break;

        case 295: // Erohim
            object->Scale = 2.0f;
            object->HiddenMesh = -1;
            object->BlendMesh = -1;
            object->SubType = 0;

            character->Weapon[0].Type = -1;
            character->Weapon[0].Level = 0;

            character->Weapon[1].Type = -1;
            character->Weapon[1].Level = 0;
            break;

        default:
            break;
        }

        AI_SetPreviewAction(object, MONSTER01_STOP1);
        Vector(1.0f, 1.0f, 1.0f, object->Light);
        Vector(1.0f, 1.0f, 1.0f, character->Light);
    }


}
void Clear()
{
    if(character) { delete[] character->Object.BoneTransform; character->Object.BoneTransform=nullptr; }
    character.reset();monster=-1;world=-1;
}
bool Prepare(int id)
{
    if(character&&monster==id&&world==gMapManager.WorldActive) return true;
    Clear();int model=-1;
    if(!AI_GetPreviewModelInfo(static_cast<unsigned short>(id),model,scale,offset)) return false;
    AI_LoadPreviewModel(static_cast<unsigned short>(id));
    if(model<0||Models[model].NumBones<=0||Models[model].NumBones>MAX_BONES||Models[model].NumMeshs<=0) return false;
    character=std::make_unique<CHARACTER>();
    CreateCharacterPointer(character.get(),model,1,1,180.f);
    if(!character->Object.BoneTransform) {Clear();return false;}
    AI_ConfigurePreviewCharacter(character.get(),static_cast<unsigned short>(id));
    Vector(0,0,0,character->Object.Position);
    character->Object.m_bRenderShadow=false;
    monster=id;world=gMapManager.WorldActive;return true;
}
void Draw(int x,int y,int width,int height)
{
    if(!character||monster<0) return;
    auto* object=&character->Object;
    CharacterAnimation(character.get(),object);
    BeginCharacterPreview(x,y,width,height,75.f+offset);
    auto& renderer=mu::GetRenderer();
    renderer.SetAlphaTest(false);renderer.SetTexture2D(true);renderer.SetFogEnabled(false);
    renderer.SetDepthFunc(GL_LEQUAL);renderer.SetAlphaFunc(GL_GREATER,.25f);
    EnableDepthTest();EnableDepthMask();EnableCullFace();renderer.ClearDepthBuffer();
    object->Scale=scale;Vector(0,0,180.f,object->Angle);
    RenderCharacter(character.get(),object,0);
    renderer.SetMatrixMode(GL_MODELVIEW);renderer.PopMatrix();
    renderer.SetMatrixMode(GL_PROJECTION);renderer.PopMatrix();
    SetRenderViewport(0,0,WindowWidth,WindowHeight);RestoreCameraPerspective();
    renderer.SetMatrixMode(GL_MODELVIEW);
}
}
