#include "stdafx.h"
#include "WeatherAudio.h"
#include "WeatherSettings.h"
#include "WeatherAudioMix.h"
#include "Core/Platform/IPlatformAudio.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <filesystem>
namespace Render::Weather
{
namespace
{
struct Channel
{
    ESound sound;
    const wchar_t* file;
    bool loaded = false, playing = false, attempted = false;
};
std::array<Channel, 5> channels{{{SOUND_PARIS_RAIN, L"Data/Music/lluvia.mp3"},
                                 {SOUND_PARIS_STORM, L"Data/Music/tormenta.mp3"},
                                 {SOUND_PARIS_DAY, L"Data/Music/dia.mp3"},
                                 {SOUND_PARIS_NIGHT, L"Data/Music/noche.mp3"},
                                 {SOUND_PARIS_THUNDER, L"Data/Music/trueno.mp3"}}};
bool thunderArmed = false;
std::uint64_t lastThunderCycle = 0;
bool Prepare(Channel& channel)
{
    if (!g_platformAudio)
        return false;
    if (!channel.attempted)
    {
        channel.attempted = true;
        if (!std::filesystem::exists(channel.file))
            return false;
        LoadWaveFile(channel.sound, channel.file, 1, false);
        channel.loaded = true;
    }
    return channel.loaded;
}
void Mix(Channel& channel, float gain)
{
    if (gain <= .005f)
    {
        if (channel.playing)
            StopBuffer(channel.sound, true);
        channel.playing = false;
        return;
    }
    if (!Prepare(channel))
        return;
    channel.playing = g_platformAudio->PlaySound(channel.sound, nullptr, true);
    const float volume = gain * g_platformAudio->GetSFXVolume();
    SetVolume(channel.sound, volume > 0.f ? static_cast<long>(2000.f * std::log10(std::clamp(volume, .00001f, 1.f))) : -10000);
}
void Thunder(const State& state)
{
    const bool storm = state.kind == Kind::Storm && state.outdoor && state.exposed;
    if (!storm)
    {
        thunderArmed = false;
        if (channels[4].playing)
            StopBuffer(channels[4].sound, true);
        channels[4].playing = false;
        return;
    }
    constexpr std::uint64_t LightningPeriodMs = 8200;
    const auto cycle = ElapsedMs() / LightningPeriodMs;
    if ((!thunderArmed || cycle != lastThunderCycle) && Prepare(channels[4]))
        channels[4].playing = g_platformAudio->PlaySound(channels[4].sound, nullptr, false);
    thunderArmed = true;
    lastThunderCycle = cycle;
    if (channels[4].playing)
    {
        const float volume = g_platformAudio->GetSFXVolume();
        SetVolume(channels[4].sound, volume > 0.f ? static_cast<long>(2000.f * std::log10(volume)) : -10000);
    }
}
} // namespace
void StopAudio()
{
    thunderArmed = false;
    for (auto& channel : channels)
    {
        if (channel.playing)
            StopBuffer(channel.sound, true);
        channel.playing = false;
    }
}
void UpdateAudio(const State& state)
{
    const auto mix = AudioMix(state, Options().dayNight);
    Mix(channels[0], mix.rain);
    Mix(channels[1], mix.snowWind);
    Mix(channels[2], mix.day);
    Mix(channels[3], mix.night);
    Thunder(state);
}
} // namespace Render::Weather
