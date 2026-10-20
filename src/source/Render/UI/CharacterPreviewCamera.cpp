#include "stdafx.h"
#include "CharacterPreviewCamera.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/Scaling/UITransform.h"
#include "Camera/CameraProjection.h"
#include "Render/Textures/ZzzOpenglUtil.h"
namespace Render::UI
{
void BeginCharacterPreview(int x, int y, int width, int height, float verticalOffset)
{
    constexpr float FieldOfView = 1.f, NearPlane = 2000.f, FarPlane = 20000.f;
    auto& renderer = mu::GetRenderer();
    const auto viewport = ::UI::Scaling::ViewportForLogicalRect(
        ::UI::Scaling::GetActiveTransform(), x, y, width, height);
    SaveCameraPerspective();
    renderer.SetMatrixMode(GL_PROJECTION);
    renderer.PushMatrix();
    renderer.LoadIdentity();
    SetRenderViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    gluPerspective2(FieldOfView, static_cast<float>(viewport.width) / viewport.height, NearPlane, FarPlane);
    renderer.SetMatrixMode(GL_MODELVIEW);
    renderer.PushMatrix();
    renderer.LoadIdentity();
    CameraProjection::GetOpenGLMatrix(g_Camera.Matrix);
    EnableDepthTest();
    EnableDepthMask();
    renderer.Rotate(-90.f, 1.f, 0.f, 0.f);
    renderer.Rotate(-90.f, 0.f, 0.f, 1.f);
    renderer.Translate(-10000.f, 0.f, -75.f + verticalOffset);
}
}
