#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisVipWindow final : public CObject
{
public:
    ~CParisVipWindow() override;
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
    struct Line
    {
        Rml::String text;
        Rml::String label;
        Rml::String value;
        bool operator==(const Line&) const = default;
    };
    struct Model
    {
        Rml::String status;
        Rml::String plan_name;
        Rml::String duration;
        std::vector<Line> benefits;
        std::vector<Line> costs;
        Rml::String current_tier;
        int selected_days = 0;
        int plan = 1;
        bool has_data = false;
        bool can_buy = false;
        bool empty = true;
    };
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void RequestPlan(int plan);
    void ConfirmPurchase();
    void SyncModel();
    static Line ParseLine(const Rml::String& text);

    CManager* m_manager = nullptr;
    int m_plan = 1;
    std::uint32_t m_receivedBeforeRequest = 0;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_vip",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_vip.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisVipWindow* g_pParisVipWindow;
} // namespace mu::ui::window
