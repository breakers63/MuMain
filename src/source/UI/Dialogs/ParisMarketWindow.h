#pragma once

#include "Network/Paris/MarketProtocol.h"
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include "UI/Inventory/ItemCameraTarget.h"
#include <memory>
#include <cstdint>
#include <vector>

struct tagITEM;

namespace mu::ui::window
{
class CManager;

class CParisMarketWindow final : public CObject
{
public:
    ~CParisMarketWindow() override;
    bool Create(CManager* manager);
    void Release();
    void OpenWindow();
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
        Rml::String seller;
        Rml::String name;
        Rml::String price;
        Rml::String currency;
        int index = 0;
        bool operator==(const Row&) const = default;
    };
    struct Model
    {
        Rml::String status, query, selectedName, selectedSeller, selectedPrice, selectedDetail, sellerInitial;
        int category = 255, currency = 255, sort = 0, selected = -1;
        bool hasSelection = false;
        std::vector<Row> listings;
        bool empty = true;
        int offerCount = 0;
        int page = 0;
        int pageCount = 1;
    };

    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void SyncModel();
    void BindActions(Rml::DataModelConstructor& constructor);
    void SelectListing(int index);
    void RequestPage(int page);
    void DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size);
    void RequestRefresh();
    void RequestBuy(int index);
    bool ReceivePacket(const std::uint8_t* packet, int size);

    struct ItemPreview { int type = -1, level = 0, excellent = 0, ancient = 0; };
    std::vector<ItemPreview> m_previews;
    std::vector<std::unique_ptr<UI::Items::ItemCameraTarget>> m_itemTargets;
    std::vector<tagITEM*> m_tooltipItems;
    void ClearTooltipItems();
    void UpdateTooltip();
    bool m_tooltipShown = false, m_filtersDirty = false;
    bool m_rowsDirty = true;
    CManager* m_manager = nullptr;
    std::uint64_t m_capabilityHandler = 0;
    std::uint64_t m_resultsHandler = 0;
    std::uint64_t m_buyHandler = 0;
    std::uint64_t m_errorHandler = 0, m_priceHandler = 0;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_market",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_market.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisMarketWindow* g_pParisMarketWindow;
} // namespace mu::ui::window
