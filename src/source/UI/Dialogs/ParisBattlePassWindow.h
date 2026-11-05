#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include "UI/Inventory/ItemCameraTarget.h"
#include <memory>
#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisBattlePassWindow final : public CObject
{
public:
    ~CParisBattlePassWindow() override;
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
    struct RewardRow
    {
        Rml::String label, amount;
        int level = 0, column = 0;
        bool premium = false, claimed = false, locked = false;
        bool zen = false, coins = false, ruud = false;
    };
    struct ItemPreview { int type = -1, level = 0, excellent = 0, ancient = 0; };
    struct Model
    {
        Rml::String status, message, requirement, premium_status, expires;
        std::vector<RewardRow> rewards;
        int level = 0, max_level = 0, points = 0, required_points = 0;
        int page = 0, max_page = 0;
        float progress = 0.f, visible_fraction = 1.f;
        bool active = false, premium_dialog = false, pending = false;
    };
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void BindEvents(Rml::DataModelConstructor& constructor);
    void ApplySnapshot();
    void RequestSnapshot();
    bool SendRequest(unsigned char operation, int level = 0, bool premium = false);
    void Claim(int level, bool premium);
    void SyncModel();
    void BuildRewards();
    void UpdatePremium();
    void DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size);

    CManager* m_manager = nullptr;
    DWORD m_receivedBeforeRequest = 0, m_lastReceived = 0, m_requestedAt = 0;
    bool m_pending = false, m_hasSnapshot = false;
    unsigned char m_operation = 0;
    int m_lastPage = -1;
    std::vector<int> m_levels;
    std::vector<ItemPreview> m_previews;
    std::vector<std::unique_ptr<UI::Items::ItemCameraTarget>> m_itemTargets;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_battle_pass",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_battle_pass.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisBattlePassWindow* g_pParisBattlePassWindow;
} // namespace mu::ui::window
