#include "stdafx.h"
#include "Render/Models/PreviewAnimationStep.h"
#include "Render/Models/PreviewFraming.h"
#include "Render/Models/PosedModelBounds.h"
#include <limits>
#include "Render/Models/MonsterPreviewTarget.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "Character/CSParts.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Camera/CameraProjection.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include <RmlUi/Core/Element.h>
#include <algorithm>
#include <cmath>

namespace Render::MonsterPreviews
{
MonsterPreviewTarget::MonsterPreviewTarget() : m_target([this](auto width, auto height) { Draw(width, height); }) {}
MonsterPreviewTarget::~MonsterPreviewTarget() { Clear(); }
void MonsterPreviewTarget::Clear()
{
    m_target.SetEnabled(false);
    if (m_character)
    {
        DeleteCloth(m_character.get(), &m_character->Object);
        DeleteParts(m_character.get());
        delete[] m_character->Object.BoneTransform;
        m_character->Object.BoneTransform = nullptr;
        m_character.reset();
    }
}
void MonsterPreviewTarget::Disable() { m_target.SetEnabled(false); }
bool MonsterPreviewTarget::Build(int monster)
{
    Clear(); m_monster = monster;
    if (!Hero || monster < 0 || monster > 32767) return false;
    // Match the original detached preview: obtain the native configured model once,
    // clone its appearance, then immediately remove the temporary world source.
    static int previewKey = -30000;
    if (previewKey <= -32000) previewKey = -30000;
    CHARACTER* source = CreateMonster(static_cast<EMonsterType>(monster), 1, 1, previewKey--);
    if (!source || source == &CharactersClient[MAX_CHARACTERS_CLIENT] || !source->Object.Live) return false;
    m_character = std::make_unique<CHARACTER>(); m_character->Initialize();
    CreateCharacterPointer(m_character.get(), source->Object.Type, 1, 1, 180.f);
    if (!m_character->Object.BoneTransform) { DeleteCharacter(source, &source->Object); Clear(); return false; }
    auto& preview = *m_character;
    preview.MonsterIndex = static_cast<EMonsterType>(monster); preview.Level = source->Level;
    preview.Weapon[0] = source->Weapon[0]; preview.Weapon[1] = source->Weapon[1];
    auto& object = preview.Object;
    object.Kind = KIND_MONSTER; object.Live = object.Visible = true; object.EnableShadow = false;
    object.HiddenMesh = source->Object.HiddenMesh; object.BlendMesh = source->Object.BlendMesh;
    object.BlendMeshLight = source->Object.BlendMeshLight; object.SubType = source->Object.SubType;
    object.Alpha = object.AlphaTarget = 1.f;
    VectorCopy(source->Object.BoundingBoxMin, object.BoundingBoxMin); VectorCopy(source->Object.BoundingBoxMax, object.BoundingBoxMax);
    preview.HideShadow = true; preview.SafeZone = false; preview.Dead = 0; preview.Movement = false;
    object.CurrentAction = object.PriorAction = MONSTER01_STOP1;
    const auto& model = Models[object.Type];
    constexpr int idleActions[] = {MONSTER01_STOP1, MONSTER01_STOP2, MONSTER01_WALK};
    for (const int action : idleActions)
    {
        if (action < model.NumActions && model.Actions[action].NumAnimationKeys > 1 && model.Actions[action].PlaySpeed > 0.f)
        {
            object.CurrentAction = object.PriorAction = static_cast<unsigned short>(action);
            break;
        }
    }
    Vector(1.f, 1.f, 1.f, object.Light); Vector(1.f, 1.f, 1.f, preview.Light);
    MeasureModel();
    DeleteCharacter(source, &source->Object);
    return true;
}
void MonsterPreviewTarget::MeasureModel()
{
    auto& object = m_character->Object;
    auto& model = Models[object.Type];
    vec3_t minimum, maximum;
    VectorCopy(object.BoundingBoxMin, minimum);
    VectorCopy(object.BoundingBoxMax, maximum);
    if (model.Actions && model.NumActions > 0 && model.Bones && model.NumBones > 0)
    {
        constexpr int PoseSamples = 5;
        constexpr float Infinity = std::numeric_limits<float>::infinity();
        vec3_t posedMinimum{Infinity, Infinity, Infinity}, posedMaximum{-Infinity, -Infinity, -Infinity};
        vec3_t angle{0.f, 0.f, 90.f}, head{};
        bool found = false;
        const int lastKey = std::max(0, model.Actions[object.CurrentAction].NumAnimationKeys - 1);
        for (int sample = 0; sample < PoseSamples; ++sample)
        {
            const float frame = static_cast<float>(lastKey * sample) / PoseSamples;
            model.Animation(object.BoneTransform, frame, frame, object.CurrentAction, angle, head,
                            false, false, nullptr, object.CurrentAction);
            found = Render::PreviewBounds::AccumulatePosedBounds(model, object.BoneTransform, posedMinimum, posedMaximum, object.HiddenMesh) || found;
        }
        if (found) { VectorCopy(posedMinimum, minimum); VectorCopy(posedMaximum, maximum); }
    }
    for (int axis = 0; axis < 3; ++axis)
    {
        m_center[axis] = (minimum[axis] + maximum[axis]) * .5f;
        m_halfExtents[axis] = std::max(1.f, (maximum[axis] - minimum[axis]) * .5f);
    }
}
void MonsterPreviewTarget::Sync(Rml::Element* image, int monster)
{
    if (!image || monster < 0) { Disable(); return; }
    Rml::Vector2f offset, size;
    if (!UI::RmlBridge::DrawnContentBox(*image, offset, size)) { Disable(); return; }
    if (monster != m_monster && !Build(monster)) { image->SetProperty("visibility", "hidden"); return; }
    const bool enabled = m_character && size.x > 1.f && size.y > 1.f;
    m_target.SetEnabled(enabled);
    if (!enabled) return;
    m_target.Resize(static_cast<std::uint32_t>(std::lround(size.x)), static_cast<std::uint32_t>(std::lround(size.y)));
    if (m_target.Source().empty()) image->SetProperty("visibility", "hidden");
    else
    {
        if (image->GetAttribute<Rml::String>("src", "") != m_target.Source())
            image->SetAttribute("src", m_target.Source());
        image->RemoveProperty("visibility");
    }
}
void MonsterPreviewTarget::Draw(std::uint32_t width, std::uint32_t height)
{
    if (!m_character || !width || !height) return;
    const Render::Animation::PreviewAnimationStep animationStep(m_previewTick);
    auto& character = *m_character; auto& object = character.Object;
    constexpr std::uint64_t RotationPeriodMilliseconds = 12000;
    constexpr float FullTurn = 360.f, DegreesToRadians = .01745329252f;
    const float rotation = static_cast<float>(GetTickCount64() % RotationPeriodMilliseconds) * FullTurn / RotationPeriodMilliseconds;
    const float sine = std::sin(rotation * DegreesToRadians), cosine = std::cos(rotation * DegreesToRadians);
    const float depth = std::abs(cosine) * m_halfExtents[0] + std::abs(sine) * m_halfExtents[1];
    const float horizontal = std::abs(sine) * m_halfExtents[0] + std::abs(cosine) * m_halfExtents[1];
    const float centerX = cosine * m_center[0] - sine * m_center[1];
    const float centerY = sine * m_center[0] + cosine * m_center[1];
    object.Scale = Framing::Scale(depth, horizontal, m_halfExtents[2], static_cast<float>(width) / height);
    Vector(0.f, 0.f, 90.f + rotation, object.Angle);
    auto& model = Models[object.Type];
    if (model.NumActions <= 0) return;
    if (object.CurrentAction >= model.NumActions) object.CurrentAction = object.PriorAction = 0;
    VectorCopy(object.Position, model.BodyOrigin); model.BodyScale = object.Scale;
    model.CurrentAction = object.CurrentAction;
    if (!CharacterAnimation(&character, &object))
    {
        object.AnimationFrame = object.PriorAnimationFrame = 0.f;
        object.PriorAction = object.CurrentAction;
    }
    SaveCameraPerspective();
    auto& renderer = mu::GetRenderer();
    renderer.SetMatrixMode(GL_PROJECTION); renderer.PushMatrix(); renderer.LoadIdentity();
    gluPerspective2(Framing::FieldOfView, static_cast<float>(width) / height, Framing::Distance * .2f, Framing::Distance * 2.f);
    renderer.SetMatrixMode(GL_MODELVIEW); renderer.PushMatrix(); renderer.LoadIdentity();
    CameraProjection::GetOpenGLMatrix(g_Camera.Matrix);
    renderer.Rotate(-90.f, 1.f, 0.f, 0.f); renderer.Rotate(-90.f, 0.f, 0.f, 1.f);
    renderer.Translate(-Framing::Distance - object.Position[0] - centerX * object.Scale,
                       -object.Position[1] - centerY * object.Scale,
                       -object.Position[2] - m_center[2] * object.Scale);
    renderer.SetAlphaTest(false); renderer.SetTexture2D(true); EnableDepthTest(); EnableCullFace(); EnableDepthMask();
    renderer.SetDepthFunc(GL_LEQUAL); renderer.SetFogEnabled(false);
    RenderCharacter(&character, &object);
    renderer.SetMatrixMode(GL_MODELVIEW); renderer.PopMatrix(); renderer.SetMatrixMode(GL_PROJECTION); renderer.PopMatrix();
    RestoreCameraPerspective();
}
}
