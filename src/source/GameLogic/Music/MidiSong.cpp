#include "GameLogic/Music/MidiSong.h"
#include <algorithm>
#include <array>
namespace GameLogic::Music
{
namespace
{
constexpr std::size_t MaxFileBytes=2*1024*1024,MaxEvents=200000;
constexpr std::uint32_t MaximumDurationMs=30*60*1000;
struct Raw { std::uint32_t tick=0,message=0,tempo=0; };
struct Reader
{
    std::span<const std::uint8_t> data;std::size_t at=0;bool valid=true;
    std::uint8_t Byte(){if(at>=data.size()){valid=false;return 0;}return data[at++];}
    std::uint32_t Big(int n){std::uint32_t v=0;while(n--)v=(v<<8)|Byte();return v;}
    std::uint32_t Variable(){std::uint32_t v=0;for(int i=0;i<4;++i){auto b=Byte();v=(v<<7)|(b&127);if(!(b&128))return v;}valid=false;return 0;}
    void Skip(std::size_t n){if(n>data.size()-std::min(at,data.size())){valid=false;return;}at+=n;}
};
bool Track(Reader& r,std::vector<Raw>& out)
{
    std::uint32_t tick=0;std::uint8_t running=0;
    while(r.valid && r.at<r.data.size())
    {
        const auto delta=r.Variable();if(delta>0x7fffffff-tick)return false;tick+=delta;
        auto status=r.Byte();bool inherited=status<128;
        if(inherited){if(!running)return false;--r.at;status=running;}
        if(status==255)
        {
            running=0;const auto type=r.Byte();const auto size=r.Variable();
            if(type==0x51 && size==3){auto tempo=r.Big(3);if(!tempo)return false;out.push_back({tick,0,tempo});}
            else r.Skip(size);
            if(type==0x2f)break;
        }
        else if(status==0xf0 || status==0xf7){running=0;r.Skip(r.Variable());}
        else if(status>=0x80 && status<0xf0)
        {
            running=status;const auto d1=r.Byte();const auto kind=status&0xf0;
            const auto d2=(kind==0xc0 || kind==0xd0)?0:r.Byte();
            if(d1>127 || d2>127)return false;
            out.push_back({tick,static_cast<std::uint32_t>(status|(d1<<8)|(d2<<16)),0});
        }
        else return false;
        if(out.size()>MaxEvents)return false;
    }
    return r.valid;
}
}
bool ParseMidi(std::span<const std::uint8_t> data,MidiSong& song,std::string& error)
{
    song={};error="Archivo MIDI no valido o demasiado grande";
    if(data.size()>MaxFileBytes || data.size()<14)return false;
    Reader r{data};if(r.Big(4)!=0x4d546864)return false;const auto header=r.Big(4);
    if(header<6 || header>data.size()-8)return false;
    const auto format=r.Big(2),tracks=r.Big(2),division=r.Big(2);
    if(format>1 || !tracks || tracks>256 || !division || (division&0x8000))return false;
    r.Skip(header-6);std::vector<Raw> events;
    for(unsigned t=0;t<tracks;++t)
    {
        if(r.Big(4)!=0x4d54726b)return false;const auto size=r.Big(4);
        if(!r.valid || size>data.size()-r.at)return false;
        Reader track{data.subspan(r.at,size)};if(!Track(track,events))return false;r.Skip(size);
    }
    std::stable_sort(events.begin(),events.end(),[](auto a,auto b){return a.tick<b.tick;});
    std::uint32_t tick=0,tempo=500000;double micros=0;
    MidiSong result;
    for(const auto& event:events)
    {
        micros+=static_cast<double>(event.tick-tick)*tempo/division;tick=event.tick;
        if(micros/1000>MaximumDurationMs){error="La cancion supera 30 minutos";return false;}
        if(event.tempo){tempo=event.tempo;continue;}
        result.events.push_back({static_cast<std::uint32_t>(micros/1000),event.message});
        result.channels|=1u<<(event.message&15);
    }
    if(result.events.empty())return false;
    result.durationMs=result.events.back().timeMs;song=std::move(result);error.clear();return true;
}
}
