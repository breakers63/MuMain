#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisEventsWindow final : public CObject
{
public:
    ~CParisEventsWindow() override;
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
    struct Tab
    {
        Rml::String label;
        int index = 0;
        bool selected = false;
        bool operator==(const Tab&) const = default;
    };
    struct EventRow
    {
        Rml::String name;
        Rml::String state, remaining;
        int index = 0;
        int seconds = 0;
        bool active = false;
        bool operator==(const EventRow&) const = default;
    };
    struct Model
    {
        Rml::String status, title;
        int page = 1;
        std::vector<Tab> tabs;
        std::vector<EventRow> events;
        bool empty = true;
    };
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void SelectTab(int tab);
    void SyncModel();

    CManager* m_manager = nullptr;
    int m_selectedTab = 0;
    DWORD m_nextSync = 0;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_events",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_events.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisEventsWindow* g_pParisEventsWindow;
} // namespace mu::ui::window
