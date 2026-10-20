#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisGmEventsWindow final : public CObject
{
public:
    ~CParisGmEventsWindow() override;
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
    struct EventRow
    {
        Rml::String name;
        int index = 0;
        int phase = 0;
        bool selected = false;
        bool operator==(const EventRow&) const = default;
    };
    struct Model
    {
        Rml::String status;
        Rml::String query, announcement, countdown, duration_text, map_name;
        Rml::String selected_name;
        Rml::String phase;
        Rml::String requirement;
        std::vector<EventRow> events;
        int selected = 0;
        int participants = 0;
        int maximum = 0;
        int time_left = 0;
        int duration = 0;
        int map = 0;
        bool authorized = false;
        bool fresh = false;
        bool can_start = false;
        bool can_register = false;
        bool can_stop = false;
        bool empty = true;
    };
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void Select(int index);
    void SendAction(int action);
    void RequestAnnouncement();
    void SyncModel();

    CManager* m_manager = nullptr;
    DWORD m_lastRequest = 0;
    DWORD m_nextViewSync = 0, m_syncedReceived = 0;
    Rml::String m_syncedQuery;
    DWORD m_receivedBeforeRequest = 0;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_gm_events",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_gm_events.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisGmEventsWindow* g_pParisGmEventsWindow;
} // namespace mu::ui::window
