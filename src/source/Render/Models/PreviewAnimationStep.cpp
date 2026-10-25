#include "stdafx.h"
#include "Render/Models/PreviewAnimationStep.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include <SDL3/SDL_timer.h>
namespace Render::Animation
{
PreviewAnimationStep::PreviewAnimationStep(std::uint64_t& lastTick) : m_previous(FPS_ANIMATION_FACTOR)
{
    const auto now = SDL_GetPerformanceCounter();
    FPS_ANIMATION_FACTOR = FactorSeconds(static_cast<double>(now - lastTick) / SDL_GetPerformanceFrequency(), lastTick == 0);
    lastTick = now;
}
PreviewAnimationStep::~PreviewAnimationStep() { FPS_ANIMATION_FACTOR = m_previous; }
}
