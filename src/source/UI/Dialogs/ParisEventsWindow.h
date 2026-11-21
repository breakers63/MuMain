#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "Network/Paris/EventGuideProtocol.h"
#include "Render/Models/MonsterPreviewTarget.h"
#include <array>
#include <mutex>

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
    struct Row
    {
        Rml::String key, name, value;
        int state = -1;
        bool selected = false;
        bool operator==(const Row&) const = default;
    };
    struct Model
    {
        Rml::String status, title, phase, countdown, players, boss;
        int page = 1, pages = 1, monsterPage = 1, monsterPages = 1;
        bool empty = true, hasDetail = false, canJoin = false;
        std::vector<Row> tabs, events, requirements, monsters, rewards;
    };
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void BindActions(Rml::DataModelConstructor& constructor);
    void Request(bool join = false);
    bool Receive(std::wstring_view text);
    void SyncModel();
    void SyncDetail(Model& model, bool fresh);
    void SyncPreviews();
    void DisablePreviews();

    CManager* m_manager = nullptr;
    std::uint64_t m_handler = 0;
    std::mutex m_mutex;
    Network::Paris::EventGuide::Decoder m_decoder;
    std::optional<Network::Paris::EventGuide::Snapshot> m_received;
    std::optional<Network::Paris::EventGuide::Snapshot> m_snapshot;
    std::uint32_t m_sequence = 0;
    bool m_pending = false;
    std::wstring m_character;
    Rml::ElementList m_previewCards;
    DWORD m_lastRequest = 0, m_nextSync = 0;
    std::string m_selected = "-", m_category = "all";
    int m_page = 0, m_monsterPage = 0;
    std::array<std::unique_ptr<Render::MonsterPreviews::MonsterPreviewTarget>, 4> m_previews;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_events",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_events.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document,
         .beforeUnload = [this] { DisablePreviews(); }}};
};
extern CParisEventsWindow* g_pParisEventsWindow;
} // namespace mu::ui::window

