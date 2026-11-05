#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
namespace mu::ui::window
{
class CManager;
class CGestureWheelWindow final : public CObject
{
public:
    ~CGestureWheelWindow() override;
    bool Create(CManager* manager);
    void Show(bool show) override;
    void Toggle();
    bool UpdateMouseEvent() override { return !IsVisible(); }
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override { return true; }
    float GetLayerDepth() override { return 10.68f; }
    float GetKeyEventOrder() override { return 102.f; }
private:
    struct Model
    {
        Rml::String caption = "GESTOS", song, status, time;
        int selected = -1;
        bool music = false, wheel = true, loop = false;
    };
    void SyncMusic();
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void Pointer(Rml::Event& event, bool activate);
    CManager* manager = nullptr;
    UI::RmlBridge::ThemedView<Model> view{"gesture_wheel",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/gesture_wheel.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};
extern CGestureWheelWindow* g_pGestureWheelWindow;
}
