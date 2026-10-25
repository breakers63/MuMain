#include "stdafx.h"

#include "Render/PostProcessing/PostProcessingSetting.h"

#include "Core/Utilities/Log/MuLogger.h"
#include "Render/Look/LookProfileSetting.h"
#include "Render/Weather/WeatherSettings.h"
#include "Render/Weather/WeatherRenderer.h"
#include "Render/Renderer/MuRenderer.h"

namespace Render::PostProcessing
{
namespace
{
bool ApplyToRenderer(bool enabled)
{
    if (mu::GetRenderer().SetPostProcessing(enabled))
    {
        mu::log::Get("render")->info("Post-processing {}", enabled ? "on" : "off");
        return true;
    }
    mu::log::Get("render")->warn("Post-processing requested but unavailable; frames stay unprocessed");
    return false;
}
} // namespace

bool SetPostProcessingEnabled(bool enabled)
{
    return ApplyToRenderer(enabled);
}

bool SetPostProcessingView(std::wstring_view name)
{
    struct NamedView
    {
        std::wstring_view name;
        const char* logName;
        mu::PostProcessingView view;
    };
    static constexpr NamedView views[] = {
        {L"scene", "scene", mu::PostProcessingView::Scene},
        {L"source", "bloom source", mu::PostProcessingView::BloomSource},
        {L"bloom", "bloom", mu::PostProcessingView::Bloom},
    };
    for (const NamedView& entry : views)
    {
        if (entry.name == name)
        {
            mu::GetRenderer().SetPostProcessingView(entry.view);
            mu::log::Get("render")->info("Post-processing view: {}", entry.logName);
            return true;
        }
    }
    mu::log::Get("render")->warn("$post view expects scene, source or bloom");
    return false;
}

void SubmitPostProcessingLook()
{
    using Data::Maps::LookValue;
    const auto& weather = Render::Weather::Options();
    const bool outdoor = Render::Weather::Current().outdoor;
    const float bloom = weather.bloomEnabled ? Render::Look::Value(LookValue::BloomStrength) + (outdoor ? weather.bloom * 0.01f : 0.0f) : 0.f;
    const bool pegasusColor = outdoor && weather.naturalColors;
    mu::GetRenderer().SetPostProcessingLook({bloom,
                                             Render::Look::Value(LookValue::BloomLevelFalloff),
                                             pegasusColor ? 1.f : Render::Look::Value(LookValue::GradingStrength),
                                             outdoor ? weather.sharpness * 0.002667f : 0.0f,
                                             0.0f});
}

} // namespace Render::PostProcessing
