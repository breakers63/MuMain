#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include <functional>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
struct tagITEM;
namespace mu::ui::window
{
class CManager;
class CParisStorePriceWindow final : public CObject
{
public:
    ~CParisStorePriceWindow() override;
    bool Create(CManager* manager);
    void Release();
    bool OpenWindow(tagITEM* item, std::function<void(std::uint8_t, int)> apply, std::function<void()> cancel);
    void Show(bool show) override;
    bool UpdateMouseEvent() override { return !IsVisible(); }
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override { return 63.f; }
    float GetKeyEventOrder() override { return 102.f; }
private:
    struct Row { Rml::String name, amount = "0"; int index = 0; bool enabled = true; };
    struct Model { Rml::String status; std::vector<Row> rows; };
    void BindModel(Rml::DataModelConstructor& c, Model& model);
    void Adjust(int row, int delta);
    void Resolve(bool apply);
    void ClearItem();
    void DrawItem(const Rml::Vector2f& offset, const Rml::Vector2f& size);
    CManager* m_manager = nullptr;
    tagITEM* m_item = nullptr;
    std::unique_ptr<UI::Items::ItemCameraTarget> m_target;
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, 5> m_currencyTargets;
    std::function<void(std::uint8_t, int)> m_apply;
    std::function<void()> m_cancel;
    bool m_extended = false;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_store_price",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_store_price.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};
extern CParisStorePriceWindow* g_pParisStorePriceWindow;
}
