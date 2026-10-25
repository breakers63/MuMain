#include "stdafx.h"
#include "UI/SocialMusic/SocialMusic.h"
#include "GameLogic/Music/MidiSong.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/AI/ZzzAI.h"
#include "Character/CharacterManager.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzTexture.h"
#include "Render/Effects/ZzzEffect.h"
#include "World/MapInfra/MapManager.h"
#include "Scenes/SceneCore.h"
#include "Network/Server/WSclient.h"
#include "Core/Math/ZzzMathLib.h"
#include <commdlg.h>
#include <mmsystem.h>
#include <array>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cmath>
#include <future>
#include <chrono>
#include "Render/Renderer/MuRenderer.h"
namespace UI::SocialMusic
{
namespace
{
constexpr DWORD EmojiDurationMs=2600;
constexpr std::size_t MaximumMidiBytes=2*1024*1024;
constexpr std::array<const wchar_t*,7> EmojiTokens={L">:(",L"<3",L"zzz",L"(!)",L"??",L"?!",L"x_x"};
constexpr std::array<const wchar_t*,12> Names={L"Enfado",L"Amor",L"Sueno",L"Alerta",L"Duda",L"Sorpresa",L"Mareo",L"Guitarra",L"Flauta",L"Ocarina",L"Arpa",L"Bateria"};
constexpr std::array<const wchar_t*,6> MeshNames={L"Guitar",L"Flute",L"Ocarina",L"Harp",L"Drumstick",L"DrumKit"};
constexpr std::array<int,5> Programs={24,73,79,46,0};
struct Bubble { CHARACTER* owner=nullptr; int emoji=-1; DWORD start=0; std::wstring name; };
struct AssetVertex { float x,y,z,u,v; };
struct Mesh { std::vector<AssetVertex> vertices; GLuint texture=BITMAP_UNKNOWN; };
std::array<Bubble,64> bubbles;
std::array<GLuint,7> emojiTextures;
std::array<Mesh,6> meshes;
GameLogic::Music::MidiSong song;
HMIDIOUT output=nullptr;
std::future<HMIDIOUT> pendingOutput;
bool wantPlay=false;
int instrument=-1,map=-1;
bool playing=false,paused=false,loop=false;
DWORD start=0,position=0,lastEmoji=0;
std::size_t nextEvent=0;
std::wstring status=L"Carga un archivo MIDI (.mid)",songName;
std::vector<mu::Vertex3D> drawVertices;
void Silence()
{
    if(!output)return;
    midiOutReset(output);
}
void CloseOutput()
{
    if(!output)return;
    Silence();midiOutClose(output);output=nullptr;
}
bool OpenOutput()
{
    if(output)return true;
    if(pendingOutput.valid())
    {
        if(pendingOutput.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return false;
        output=pendingOutput.get();
        if(!output){status=L"No hay un sintetizador MIDI disponible";wantPlay=false;return false;}
        return true;
    }
    status=L"Iniciando sintetizador MIDI...";
    pendingOutput=std::async(std::launch::async,[]
    {
        HMIDIOUT device=nullptr;
        if(midiOutOpen(&device,MIDI_MAPPER,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR)return static_cast<HMIDIOUT>(nullptr);
        return device;
    });
    return false;
}
void Send(std::uint32_t message)
{
    if(!output)return;
    const int kind=message&0xf0;
    // The chosen instrument owns the timbre. Drums play only MIDI percussion.
    if(kind==0xc0)return;
    const int channel=message&15;
    if((instrument==4)!=(channel==9))return;
    midiOutShortMsg(output,message);
}
void Program()
{
    if(!OpenOutput())return;
    for(int channel=0;channel<16;++channel)
        if(channel!=9)midiOutShortMsg(output,0xc0|channel|(Programs[instrument]<<8));
}
void LoadMesh(int index)
{
    auto& mesh=meshes[index];if(!mesh.vertices.empty())return;
    const auto base=std::filesystem::path(L"Data/Interface/Instruments")/MeshNames[index];
    std::ifstream file(base.wstring()+L".pin",std::ios::binary);
    char magic[4]{};std::uint32_t count=0;
    file.read(magic,4);file.read(reinterpret_cast<char*>(&count),4);
    if(!file || std::memcmp(magic,"PIN1",4)!=0 || count>30000 || count%3)return;
    mesh.vertices.resize(count);file.read(reinterpret_cast<char*>(mesh.vertices.data()),count*sizeof(AssetVertex));
    if(!file){mesh.vertices.clear();return;}
    mesh.texture=Bitmaps.LoadImage(base.wstring()+L".tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
    g_ErrorReport.Write(L"[SocialMusic] Instrument asset %ls: %u vertices, texture %u\r\n",MeshNames[index],count,mesh.texture);
}
void Pose()
{
    if(!Hero || instrument<0 || !Hero->Object.BoneTransform)return;
    auto& object=Hero->Object;
    const int base=instrument==0||instrument==4?PLAYER_HUSTLE:instrument==3?PLAYER_CLAP1:PLAYER_AGAIN1;
    const int clip=base==PLAYER_HUSTLE?base:base+(gCharacterManager.IsFemale(Hero->Class)?1:0);
    const auto& model=Models[object.Type];
    if(!model.Actions || clip>=model.NumActions)return;
    if(object.CurrentAction!=clip)SetAction(&object,clip);
    const float keys=static_cast<float>(model.Actions[clip].NumAnimationKeys-1);
    // Sample a stable playing pose instead of mixing it with locomotion frames.
    constexpr float PlayingFrames[5]={0.f,.5f,.5f,0.f,.3f};
    object.AnimationFrame=keys*PlayingFrames[instrument];
    object.PriorAnimationFrame=object.AnimationFrame;
    object.PriorAction=clip;
}
void TransformInstrumentVertex(int index,const float* local,const float* matrixData,
                               int bone,const float* offset,float* world)
{
    auto& object=Hero->Object;
    if(index==1 || index==2)
    {
        // Wind instruments sit outside the helmet at the mouth. Hand-local
        // offsets from the web rig can bury their small models in native armor.
        constexpr int HeadBone=20;
        constexpr float MouthClearance=45.f;
        constexpr float MouthDrop=8.f;
        vec3_t instrumentAngle={0.f,index==1?90.f:0.f,0.f};
        float instrumentMatrix[3][4],actorMatrix[3][4];
        AngleMatrix(instrumentAngle,instrumentMatrix);
        AngleMatrix(object.Angle,actorMatrix);
        vec3_t posed,rotated;
        VectorRotate(local,instrumentMatrix,posed);
        posed[1]-=MouthClearance;
        posed[2]-=MouthDrop;
        VectorRotate(posed,actorMatrix,rotated);
        for(int axis=0;axis<3;++axis)
            world[axis]=(object.BoneTransform[HeadBone][axis][3]+rotated[axis])*object.Scale;
        return;
    }
    const auto matrix=reinterpret_cast<const float (*)[4]>(matrixData);
    vec3_t rotated;
    VectorRotate(local,matrix,rotated);
    for(int axis=0;axis<3;++axis)rotated[axis]+=offset[axis];
    VectorTransform(rotated,object.BoneTransform[bone],world);
    for(int axis=0;axis<3;++axis)world[axis]*=object.Scale;
}
void BuildInstrumentTransform(int index,int bone,const float* angle,const float* offset,float matrix[3][4])
{
    vec3_t rotation={angle[0],angle[1],angle[2]};
    float localRotation[3][4];
    AngleMatrix(rotation,localRotation);
    constexpr float InstrumentScales[6]={1.f,1.4f,1.5f,1.f,1.f,1.f};
    const float scale=InstrumentScales[index];
    const vec3_t origin={0.f,0.f,0.f};
    vec3_t translation;
    TransformInstrumentVertex(index,origin,&localRotation[0][0],bone,offset,translation);
    // Compose the affine transform once, rather than recomputing rotation
    // matrices and trigonometry for every triangle vertex.
    for(int axis=0;axis<3;++axis)
    {
        vec3_t basis={0.f,0.f,0.f},transformed;
        basis[axis]=scale;
        TransformInstrumentVertex(index,basis,&localRotation[0][0],bone,offset,transformed);
        for(int row=0;row<3;++row)matrix[row][axis]=transformed[row]-translation[row];
    }
    for(int row=0;row<3;++row)matrix[row][3]=translation[row];
}
void DrawMesh(int index,int bone,const float* angle,const float* offset)
{
    const auto& mesh=meshes[index];if(mesh.vertices.empty()||mesh.texture==BITMAP_UNKNOWN)return;
    auto& object=Hero->Object;auto& model=Models[object.Type];
    if(!object.BoneTransform || bone>=model.NumBones)return;
    float matrix[3][4];
    BuildInstrumentTransform(index,bone,angle,offset,matrix);
    drawVertices.clear();drawVertices.reserve(mesh.vertices.size());
    for(const auto& vertex:mesh.vertices)
    {
        vec3_t local={vertex.x,vertex.y,vertex.z},world;
        VectorTransform(local,matrix,world);
        VectorAdd(world,object.Position,world);
        drawVertices.push_back({world[0],world[1],world[2],0,0,1,vertex.u,vertex.v,0xffffffffu});
    }
    const bool hadCull=CullFaceEnable; DisableCullFace();
    EnableAlphaTest(); BindTexture(mesh.texture); mu::GetRenderer().RenderTriangles(drawVertices,0u);
    if(hadCull)EnableCullFace();
}
void RenderInstrument()
{
    if(!Hero || instrument<0 || !Hero->Object.Visible)return;
    static constexpr float angles[6][3]={{-84,-35,-90},{-59,15,160},{-94,26,41},{0,0,-173},{-145,4,53},{-1,-5,7}};
    static constexpr float offsets[6][3]={{-11,-1,-1},{9,-28,10},{25,14,-4},{56,25,-107},{0,0,0},{14,4,-99}};
    static constexpr int bones[6]={42,33,33,0,33,0};
    DrawMesh(instrument,bones[instrument],angles[instrument],offsets[instrument]);
    if(instrument==4)
    {
        constexpr float leftAngle[3]={-148,4,-61},leftOffset[3]={0,0,0};
        DrawMesh(4,42,leftAngle,leftOffset);DrawMesh(5,0,angles[5],offsets[5]);
    }
}
}
void Load()
{
    emojiTextures.fill(BITMAP_UNKNOWN);
    for(int i=0;i<7;++i)emojiTextures[i]=Bitmaps.LoadImage(L"Data/Interface/Emoji_"+std::to_wstring(i)+L".tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
    map=gMapManager.WorldActive;
}
void Unload()
{
    PutAway();CloseOutput();
    if(pendingOutput.valid()){auto device=pendingOutput.get();if(device)midiOutClose(device);}
    for(auto& id:emojiTextures){if(id!=BITMAP_UNKNOWN)DeleteBitmap(id);id=BITMAP_UNKNOWN;}
    for(auto& mesh:meshes){if(mesh.texture!=BITMAP_UNKNOWN)DeleteBitmap(mesh.texture);mesh={};}
    bubbles.fill({});song={};
}
bool ReceiveEmoji(CHARACTER* owner,const wchar_t* text)
{
    if(!owner || !text)return false;
    int emoji=-1;for(int i=0;i<7;++i)if(std::wcscmp(text,EmojiTokens[i])==0){emoji=i;break;}
    if(emoji<0)return false;
    for(auto& bubble:bubbles)
        if(bubble.owner==owner || !bubble.owner || timeGetTime()-bubble.start>EmojiDurationMs)
        {bubble={owner,emoji,timeGetTime(),owner->ID};return true;}
    return true;
}
const wchar_t* Label(int index)
{
    return index>=25&&index<37?Names[index-25]:L"GESTOS";
}
void Activate(int index)
{
    if(!Hero || Hero->Dead || SceneFlag!=MAIN_SCENE)return;
    if(index>=25&&index<32)
    {
        if(lastEmoji && timeGetTime()-lastEmoji<1000)return;
        lastEmoji=timeGetTime();const auto text=EmojiTokens[index-25];
        SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(Hero->ID),MU_C16(text));
        ReceiveEmoji(Hero,text);return;
    }
    if(index<32||index>=37)return;
    PutAway();instrument=index-32;map=gMapManager.WorldActive;
    LoadMesh(instrument);if(instrument==4)LoadMesh(5);
    status=L"Reproduccion local - carga un MIDI";Program();
}
void OpenMidi()
{
    if(instrument<0)return;
    wchar_t path[MAX_PATH]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=g_hWnd;
    dialog.lpstrFilter=L"Archivos MIDI (*.mid;*.midi)\0*.mid;*.midi\0\0";
    dialog.lpstrFile=path;dialog.nMaxFile=MAX_PATH;dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(!GetOpenFileNameW(&dialog))return;
    std::error_code ec;const auto size=std::filesystem::file_size(path,ec);
    if(ec||size>MaximumMidiBytes){status=L"El MIDI supera 2 MB o no se puede leer";return;}
    std::ifstream file(path,std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
    GameLogic::Music::MidiSong parsed;std::string error;
    if(!GameLogic::Music::ParseMidi(bytes,parsed,error)){status=std::wstring(error.begin(),error.end());return;}
    Stop();song=std::move(parsed);songName=std::filesystem::path(path).filename().wstring();status=L"MIDI listo - pulsa Tocar";
}
void Play()
{
    if(instrument<0||song.events.empty())return;
    if(!OpenOutput()){wantPlay=true;return;}
    wantPlay=false;
    Program();if(!paused){position=0;nextEvent=0;}start=timeGetTime()-position;playing=true;paused=false;status=L"Tocando (audio local)";
}
void Pause()
{
    if(!playing){if(paused)Play();return;}
    position=timeGetTime()-start;playing=false;paused=true;Silence();status=L"Pausado";
}
void Stop(){wantPlay=false;playing=paused=false;position=0;nextEvent=0;Silence();}
void PutAway(){Stop();instrument=-1;if(Hero)SetPlayerStop(Hero);}
void ToggleLoop(){loop=!loop;}
bool HasInstrument(){return instrument>=0;}
bool HasSong(){return !song.events.empty();}
bool IsPlaying(){return playing;}
bool IsLooping(){return loop;}
int Instrument(){return instrument;}
std::wstring Status(){return status;}
std::wstring SongName(){return songName.empty()?L"Ningun MIDI cargado":songName;}
std::wstring Time()
{
    auto seconds=(playing?timeGetTime()-start:position)/1000;auto total=song.durationMs/1000;
    wchar_t text[64]{};swprintf(text,64,L"%u:%02u / %u:%02u",seconds/60,seconds%60,total/60,total%60);return text;
}
void Update()
{
    if(SceneFlag!=MAIN_SCENE || !Hero || Hero->Dead || map!=gMapManager.WorldActive)
    {if(instrument>=0)PutAway();bubbles.fill({});map=gMapManager.WorldActive;return;}
    if(instrument>=0 && Hero->Movement){PutAway();return;}
    if(pendingOutput.valid()) {if(OpenOutput() && wantPlay)Play();}
    if(playing)
    {
        position=timeGetTime()-start;
        int sent=0;
        while(nextEvent<song.events.size() && song.events[nextEvent].timeMs<=position && sent++<512)Send(song.events[nextEvent++].message);
        if(nextEvent==song.events.size() && position>=song.durationMs) {Stop();if(loop)Play();else status=L"Cancion terminada";}
    }
    if(instrument>=0)Pose();
}
void PrepareCharacterPose()
{
    if(!Hero || instrument<0 || !Hero->Object.Live || !Hero->Object.BoneTransform)return;
    Pose();
    auto& object=Hero->Object;
    auto& model=Models[object.Type];
    // Character palettes are relative to the actor. BMD state may still belong
    // to the last actor rendered, so never inherit its origin or scale here.
    vec3_t savedOrigin;
    VectorCopy(model.BodyOrigin,savedOrigin);
    const float savedScale=model.BodyScale;
    Vector(0.f,0.f,0.f,model.BodyOrigin);
    model.BodyScale=1.f;
    // Run after animation workers finish so body, hands and instrument share one palette.
    model.Animation(object.BoneTransform,object.AnimationFrame,object.AnimationFrame,
                    object.CurrentAction,object.Angle,object.HeadAngle,false,true,nullptr,
                    object.CurrentAction);
    VectorCopy(savedOrigin,model.BodyOrigin);
    model.BodyScale=savedScale;
    object.EnableBoneMatrix=true;
}
void RenderWorld()
{
    RenderInstrument();
    for(auto& bubble:bubbles)
    {
        if(!bubble.owner)continue;
        auto& object=bubble.owner->Object;const auto age=timeGetTime()-bubble.start;
        if(age>EmojiDurationMs || !object.Live || bubble.name!=bubble.owner->ID){bubble={};continue;}
        if(!object.Visible)continue;
        const auto texture=emojiTextures[bubble.emoji];if(texture==BITMAP_UNKNOWN)continue;
        vec3_t position={object.Position[0],object.Position[1],object.Position[2]+object.BoundingBoxMax[2]+180.f},light={1.f,1.f,1.f};
        CreateSprite(texture,position,.30f,light,&object);
    }
}
}
