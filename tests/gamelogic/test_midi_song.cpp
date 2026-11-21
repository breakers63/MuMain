#include <doctest.h>
#include "GameLogic/Music/MidiSong.h"
TEST_CASE("MIDI decoding schedules notes and rejects truncated or unsupported files")
{
    std::vector<std::uint8_t> data={0x4d,0x54,0x68,0x64,0,0,0,6,0,0,0,1,1,0xe0,0x4d,0x54,0x72,0x6b,0,0,0,13,0,0x90,60,100,0x83,0x60,0x80,60,0,0,0xff,0x2f,0};
    GameLogic::Music::MidiSong song;std::string error;
    REQUIRE(GameLogic::Music::ParseMidi(data,song,error));REQUIRE(song.events.size()==2);
    CHECK(song.events[0].timeMs==0);CHECK(song.events[1].timeMs==500);CHECK(song.durationMs==500);CHECK(song.channels==1);
    data.pop_back();CHECK_FALSE(GameLogic::Music::ParseMidi(data,song,error));CHECK(song.events.empty());
    CHECK_FALSE(GameLogic::Music::ParseMidi({},song,error));
}
