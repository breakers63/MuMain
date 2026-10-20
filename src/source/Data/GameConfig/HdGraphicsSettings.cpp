#include "HdGraphicsSettings.h"
#include <algorithm>
namespace Data::Graphics
{
HdGraphicsSettings NormalizeHdSettings(HdGraphicsSettings settings)
{
    settings.Profile = std::clamp(settings.Profile, 0, 2);
    settings.Sharpness = std::clamp(settings.Sharpness, 0, 100);
    settings.BloomStrength = std::clamp(settings.BloomStrength, 0, 100);
    settings.Exposure = std::clamp(settings.Exposure, 75, 125);
    return settings;
}
HdGraphicsSettings HdProfile(int profile)
{
    HdGraphicsSettings settings;
    settings.Profile = std::clamp(profile, 0, 2);
    settings.Enabled = settings.Profile != 0;
    settings.Sharpness = settings.Profile == 2 ? 55 : 35;
    settings.BloomStrength = settings.Profile == 2 ? 40 : 20;
    settings.Exposure = settings.Profile == 2 ? 105 : 100;
    return settings;
}
}
