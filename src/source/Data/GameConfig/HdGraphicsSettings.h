#pragma once
namespace Data::Graphics
{
struct HdGraphicsSettings
{
    bool Enabled = false;
    bool ColorGrading = true;
    bool Sharpen = true;
    bool Bloom = true;
    int Sharpness = 35;
    int BloomStrength = 25;
    int Exposure = 100;
    int Profile = 0;
};
HdGraphicsSettings NormalizeHdSettings(HdGraphicsSettings settings);
HdGraphicsSettings HdProfile(int profile);
}
