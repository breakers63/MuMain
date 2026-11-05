#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace GameLogic::Music
{
struct MidiNote { std::uint32_t timeMs=0,message=0; };
struct MidiSong { std::vector<MidiNote> events; std::uint32_t durationMs=0; std::uint16_t channels=0; };
bool ParseMidi(std::span<const std::uint8_t> data, MidiSong& song, std::string& error);
}
