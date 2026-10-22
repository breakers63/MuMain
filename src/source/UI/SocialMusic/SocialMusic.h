#pragma once
#include <string>
class CHARACTER;
namespace UI::SocialMusic
{
void Load();
void Unload();
void Update();
void RenderWorld();
void PrepareCharacterPose();
bool ReceiveEmoji(CHARACTER* owner,const wchar_t* text);
void Activate(int wheelIndex);
const wchar_t* Label(int wheelIndex);
void OpenMidi();
void Play();
void Pause();
void Stop();
void PutAway();
void ToggleLoop();
bool HasInstrument();
bool HasSong();
bool IsPlaying();
bool IsLooping();
int Instrument();
std::wstring Status();
std::wstring SongName();
std::wstring Time();
}
