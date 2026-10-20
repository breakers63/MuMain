#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "Network/Paris/AuctionProtocol.h"

#include <string>
#include <array>
#include <memory>
namespace UI::Items { class ItemCameraTarget; }
struct tagITEM;
#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisAuctionWindow final : public CObject
{
public:
    CParisAuctionWindow();
    ~CParisAuctionWindow() override;
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
        Rml::String name;
        Rml::String options, time;
        Rml::String seller;
        Rml::String price;
        Rml::String state;
        Rml::String action;
        int index = 0;
        bool enabled = false;
        bool operator==(const Row&) const = default;
    };
    struct Model
    {
        Rml::String status;
        Rml::String query;
        int group = 255, subtab = 0;
        Rml::String priceInput = "1";
        Rml::String buyNowInput = "0";
        Rml::String bidInput = "1";
        Rml::String durationLabel;
        Rml::String currencyLabel;
        std::vector<Row> rows;
        int tab = 0;
        int page = 1;
        int pageCount = 1;
        bool empty = true;
    };

    void ClearItems();
    void SyncItems();
    void DrawItem(int index, const Rml::Vector2f& offset, const Rml::Vector2f& size);
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void SyncModel();
    void RequestPage(int tab, int page);
    void HandleRowAction(int index);
    void PublishItem(const Network::Paris::Auctions::Row& row);
    void PlaceBid(const Network::Paris::Auctions::Row& row);
    bool ReadAmount(const Rml::String& value, int& amount) const;

    std::array<tagITEM*, 16> m_items{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, 16> m_targets;
    DWORD m_itemsReceived = 0, m_nextSync = 0;
    bool m_tooltipShown = false;
    CManager* m_manager = nullptr;
    int m_durationIndex = 0;
    int m_currency = 0;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_auction",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_auction.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisAuctionWindow* g_pParisAuctionWindow;
} // namespace mu::ui::window
