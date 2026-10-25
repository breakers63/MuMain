#include "stdafx.h"
#include "UI/SocialMusic/SocialMusic.h"
#include "UI/Dialogs/GestureWheelWindow.h"
#include "UI/Core/WindowManager.h"
#include "GameLogic/Social/Gestures.h"
#include "GameLogic/Social/GestureWheelGeometry.h"
#include "Core/Utilities/StringUtils.h"
#include "Scenes/SceneCore.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/Dialogs/ParisWindowGroup.h"
#include <RmlUi/Core/Event.h>
namespace mu::ui::window
{
namespace { constexpr DWORD InterfaceId = 0x47535452u; constexpr int GestureKey = 0xC0; }
CGestureWheelWindow* g_pGestureWheelWindow = nullptr;
CGestureWheelWindow::~CGestureWheelWindow()
{
    UI::SocialMusic::Unload();
    view.Release();
    if (manager) manager->RemoveUIObj(this);
}
bool CGestureWheelWindow::Create(CManager* owner)
{
    if (!owner) return false;
    UI::SocialMusic::Load();
    manager = owner;
    owner->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}
void CGestureWheelWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    c.BindEventCallback("music_capture", [](Rml::DataModelHandle, Rml::Event& e, const Rml::VariantList&) { e.StopPropagation(); });
    c.Bind("music", &model.music); c.Bind("wheel", &model.wheel);
    c.Bind("song", &model.song); c.Bind("status", &model.status); c.Bind("time", &model.time); c.Bind("loop", &model.loop);
    c.BindEventCallback("music_action", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if(args.empty())return;
        switch(args[0].Get<int>())
        {
        case 0:UI::SocialMusic::OpenMidi();break;case 1:UI::SocialMusic::Play();break;
        case 2:UI::SocialMusic::Pause();break;case 3:UI::SocialMusic::Stop();break;
        case 4:UI::SocialMusic::ToggleLoop();break;case 5:UI::SocialMusic::PutAway();Show(false);break;
        }
        SyncMusic();
    });
    c.Bind("caption", &model.caption);
    c.Bind("selected", &model.selected);
    c.BindEventCallback("gesture_hover", [this](Rml::DataModelHandle, Rml::Event& e, const Rml::VariantList&) { Pointer(e, false); });
    c.BindEventCallback("gesture_pick", [this](Rml::DataModelHandle, Rml::Event& e, const Rml::VariantList&) { e.StopPropagation(); Pointer(e, true); });
    c.BindEventCallback("gesture_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Show(false); });
}
void CGestureWheelWindow::Pointer(Rml::Event& event, bool activate)
{
    auto* panel = view.Document()->GetElementById("wheel");
    if (!panel) return;
    const auto origin = panel->GetAbsoluteOffset();
    const auto size = panel->GetBox().GetSize();
    if (size.x <= 0 || size.y <= 0) return;
    const double x = (event.GetParameter<float>("mouse_x", 0) - origin.x) / size.x * 2 - 1;
    const double y = (event.GetParameter<float>("mouse_y", 0) - origin.y) / size.y * 2 - 1;
    const int selected = UI::Gestures::Pick(x / 0.974, y / 0.974);
    auto& model = view.GetModel();
    if (model.selected != selected)
    {
        model.selected = selected;
        model.caption = selected < 0 ? "GESTOS" : StringUtils::WideToNarrow(selected < 25 ? GameLogic::Social::Gestures[selected].label : UI::SocialMusic::Label(selected));
        view.MarkDirty("selected"); view.MarkDirty("caption");
    }
    if (!activate) return;
    if (selected >= 0 && selected < 25) GameLogic::Social::PerformGesture(selected);
    else if(selected >= 25) UI::SocialMusic::Activate(selected);
    if(selected >= 32)
    {
        model.wheel = false; model.music = true;
        view.MarkDirty("wheel"); view.MarkDirty("music"); SyncMusic();
    }
    else Show(false);
}
void CGestureWheelWindow::Show(bool show)
{
    if (show && !view.Ensure()) return;
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!view.Document()) return;
    if (show)
    {
        view.GetModel().wheel = true; view.GetModel().music = false;
        view.MarkDirty("wheel"); view.MarkDirty("music");
        view.GetModel().selected = -1;
        view.GetModel().caption = "GESTOS";
        view.MarkDirty("selected"); view.MarkDirty("caption");
        view.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    }
    else view.Document()->Hide();
}
void CGestureWheelWindow::Toggle() { Show(!IsVisible()); }
bool CGestureWheelWindow::UpdateKeyEvent()
{
    if (!IsVisible()) return true;
    if (IsPress(VK_ESCAPE) || IsPress(GestureKey)) Show(false);
    return false;
}
bool CGestureWheelWindow::Update()
{
    if (IsVisible() && (SceneFlag != MAIN_SCENE || !Hero || Hero->Dead)) Show(false);
    if (IsVisible() && view.GetModel().music) SyncMusic();
    return true;
}
}

void mu::ui::window::CGestureWheelWindow::SyncMusic()
{
    auto& model=view.GetModel();
    const auto sync=[this](Rml::String& field,const char* key,const std::wstring& value)
    {auto text=StringUtils::WideToNarrow(value.c_str());if(field!=text){field=text;view.MarkDirty(key);}};
    sync(model.song,"song",UI::SocialMusic::SongName());sync(model.status,"status",UI::SocialMusic::Status());
    sync(model.time,"time",UI::SocialMusic::Time());
    sync(model.caption,"caption",UI::SocialMusic::Label(UI::SocialMusic::Instrument()+32));
    if(model.loop!=UI::SocialMusic::IsLooping()){model.loop=UI::SocialMusic::IsLooping();view.MarkDirty("loop");}
}
