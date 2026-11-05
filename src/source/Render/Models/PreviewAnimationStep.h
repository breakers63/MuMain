#pragma once
#include <cstdint>
namespace Render::Animation
{
// A detached UI model follows its own presentation clock, without changing the world's clock.
class PreviewAnimationStep
{
public:
    explicit PreviewAnimationStep(std::uint64_t& lastTick);
    ~PreviewAnimationStep();
    PreviewAnimationStep(const PreviewAnimationStep&) = delete;
    PreviewAnimationStep& operator=(const PreviewAnimationStep&) = delete;
    static constexpr float Factor(std::uint64_t elapsedMs, bool firstFrame)
    {
        return FactorSeconds(elapsedMs / 1000.0, firstFrame);
    }
    static constexpr float FactorSeconds(double elapsedSeconds, bool firstFrame)
    {
        constexpr double ReferenceFramesPerSecond = 25.;
        constexpr double ResumeThresholdSeconds = .25;
        constexpr double FirstFrameSeconds = 1. / 60.;
        const double seconds = firstFrame || elapsedSeconds > ResumeThresholdSeconds ? FirstFrameSeconds : elapsedSeconds;
        return static_cast<float>(seconds * ReferenceFramesPerSecond);
    }
private:
    float m_previous;
};
}
