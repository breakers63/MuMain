#include "stdafx.h"
#include "GameConfig.h"

void GameConfig::LoadVisualReductions()
{
    using namespace Data::Graphics;
    for (std::size_t index = 0; index < VisualReductionCount; ++index)
        m_visualReductions[index] = ReadBool(VisualReductionSection, VisualReductionKeys[index], false);
    LoadHdSettings();
}

void GameConfig::SaveVisualReductions()
{
    using namespace Data::Graphics;
    for (std::size_t index = 0; index < VisualReductionCount; ++index)
        WriteBool(VisualReductionSection, VisualReductionKeys[index], m_visualReductions[index]);
    SaveHdSettings();
}

bool GameConfig::IsVisualReductionEnabled(Data::Graphics::VisualReduction option) const
{
    const auto index = static_cast<std::size_t>(option);
    return index < m_visualReductions.size() && m_visualReductions[index];
}

void GameConfig::SetVisualReductionEnabled(Data::Graphics::VisualReduction option, bool enabled)
{
    const auto index = static_cast<std::size_t>(option);
    if (index >= m_visualReductions.size() || m_visualReductions[index] == enabled) return;
    m_visualReductions[index] = enabled;
    SaveVisualReductions();
}
void GameConfig::LoadHdSettings()
{
    constexpr const wchar_t* Section = L"ShadersHD";
    m_hdSettings.Enabled = ReadBool(Section, L"Enabled", false);
    m_hdSettings.ColorGrading = ReadBool(Section, L"ColorGrading", true);
    m_hdSettings.Sharpen = ReadBool(Section, L"Sharpen", true);
    m_hdSettings.Bloom = ReadBool(Section, L"Bloom", true);
    m_hdSettings.Sharpness = ReadInt(Section, L"Sharpness", 35);
    m_hdSettings.BloomStrength = ReadInt(Section, L"BloomStrength", 25);
    m_hdSettings.Exposure = ReadInt(Section, L"Exposure", 100);
    m_hdSettings.Profile = ReadInt(Section, L"Profile", 0);
    m_hdSettings = Data::Graphics::NormalizeHdSettings(m_hdSettings);
}
void GameConfig::SaveHdSettings()
{
    constexpr const wchar_t* Section = L"ShadersHD";
    WriteBool(Section, L"Enabled", m_hdSettings.Enabled);
    WriteBool(Section, L"ColorGrading", m_hdSettings.ColorGrading);
    WriteBool(Section, L"Sharpen", m_hdSettings.Sharpen);
    WriteBool(Section, L"Bloom", m_hdSettings.Bloom);
    WriteInt(Section, L"Sharpness", m_hdSettings.Sharpness);
    WriteInt(Section, L"BloomStrength", m_hdSettings.BloomStrength);
    WriteInt(Section, L"Exposure", m_hdSettings.Exposure);
    WriteInt(Section, L"Profile", m_hdSettings.Profile);
}
void GameConfig::SetHdSettings(Data::Graphics::HdGraphicsSettings settings)
{
    m_hdSettings = Data::Graphics::NormalizeHdSettings(settings);
    SaveHdSettings();
}
