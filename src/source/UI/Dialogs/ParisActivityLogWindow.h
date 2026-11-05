#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <RmlUi/Core/Types.h>

#include <vector>
#include <array>
#include <memory>
#include "UI/Inventory/ItemCameraTarget.h"
struct tagITEM;

namespace mu::ui::window
{
class CManager;

class CParisActivityLogWindow final : public CObject
{
public:
    ~CParisActivityLogWindow() override;
    bool Create(CManager* manager);
    void Release();
    void ToggleWindow(bool kills);
    void OpenWindow(bool kills);
    void CloseWindow();
    void Show(bool show) override;
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override { return 62.5f; }
    float GetKeyEventOrder() override { return 101.f; }

private:
    struct Row
    {
        Rml::String time;
        Rml::String actor;
        Rml::String description;
        Rml::String detail;
        Rml::String map;
        bool winner = false;
        int slot = 0, mapTile = 0;
        bool operator==(const Row&) const = default;
    };
    struct Model
    {
        Rml::String title;
        Rml::String status;
        std::vector<Row> rows;
        bool empty = true, kills = true, all = true;
        int first = 0, maximum = 0, count = 0;
        Rml::String query, thumb;
    };

    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void SyncModel();
    CManager* m_manager = nullptr;
    bool m_kills = true, m_dirty = true, m_tooltipShown = false;
    unsigned long long m_revision = ~0ull;
    int m_first = -1;
    Rml::String m_query;
    bool m_all = true;
    std::vector<int> m_filtered;
    std::array<tagITEM*, 4> m_items{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, 4> m_targets;
    void ClearItems();
    void BuildRows();
    void UpdateItems();
    void Scroll(int delta);
    void DrawItem(int row, const Rml::Vector2f& offset, const Rml::Vector2f& size);
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_activity_log",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_activity_log.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisActivityLogWindow* g_pParisActivityLogWindow;
} // namespace mu::ui::window
