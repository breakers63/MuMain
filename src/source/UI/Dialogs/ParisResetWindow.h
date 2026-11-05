#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisResetWindow final : public CObject
{
public:
    ~CParisResetWindow() override;
    bool Create(CManager* manager);
    void Release();
    void OpenWindow();
    void CloseWindow();
    void Show(bool show) override;
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override { return 62.f; }
    float GetKeyEventOrder() override { return 101.f; }

private:
    struct Line { Rml::String text; Rml::String state; bool operator==(const Line&) const = default; };
    struct Model
    {
        Rml::String status;
        Rml::String reset_count;
        Rml::String result;
        std::vector<Line> information;
        std::vector<Line> requirements;
        std::vector<Line> rewards;
        bool has_information = false;
        bool has_requirements = false;
        bool has_rewards = false;
        bool has_data = false;
        bool can_reset = false;
        bool pending = false;
    };
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void RequestStatus();
    void ConfirmReset();
    void SyncModel();

    CManager* m_manager = nullptr;
    unsigned int m_receivedBeforeRequest = 0;
    bool m_closeRequested = false;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_reset",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_reset.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisResetWindow* g_pParisResetWindow;
} // namespace mu::ui::window
