#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "Network/Paris/ActiveInvasionProtocol.h"
#include "Render/Models/MonsterPreviewTarget.h"
#include <map>
#include <mutex>
#include <vector>
namespace mu::ui::window
{
class CManager;
class CActiveInvasionWindow : public CObject
{
public:
    ~CActiveInvasionWindow();
    bool Create(CManager* manager);
    void Release();
    bool Update() override;
    bool Render() override;
    bool UpdateMouseEvent() override
    {
        return true;
    }
    bool UpdateKeyEvent() override
    {
        return true;
    }
    float GetLayerDepth() override
    {
        return 1.1f;
    }

private:
    struct Row
    {
        Rml::String name, count;
        int monster = 0;
        int alive = 0, total = 0;
    };
    struct Event
    {
        Rml::String name;
        std::vector<Row> rows;
        unsigned long long deadline = 0;
    };
    struct Model
    {
        bool active = false, open = false;
        Rml::String title, time;
        std::vector<Row> rows;
        bool previewVisible = false;
        Rml::String previewName, previewCount, summary;
    };
    void BindModel(Rml::DataModelConstructor& c, Model& model);
    void Apply(const Network::Paris::ActiveInvasion::Update& update);
    void Sync();
    void UpdatePreview();
    int m_hovered = -1;
    std::unique_ptr<Render::MonsterPreviews::MonsterPreviewTarget> m_preview;
    CManager* m_manager = nullptr;
    std::uint64_t m_registration = 0;
    std::mutex m_mutex;
    std::vector<Network::Paris::ActiveInvasion::Update> m_pending;
    std::map<int, Event> m_events;
    int m_selected = 0;
    unsigned long long m_nextSync = 0;
    UI::RmlBridge::ThemedView<Model> m_view{"active_invasion",
                                            [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
                                            {{"Data/Interface/RmlUi/active_invasion.rml"}}};
};
} // namespace mu::ui::window
