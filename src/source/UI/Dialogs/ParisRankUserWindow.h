#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <RmlUi/Core/Types.h>

#include <cstdint>

namespace mu::ui::window
{
class CManager;

class CParisRankUserWindow final : public CObject
{
public:
    ~CParisRankUserWindow() override;
    bool Create(CManager* manager);
    void Release();
    void OpenWindow(std::uint16_t target);
    void CloseWindow();
    void Show(bool show) override;
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override { return 62.5f; }
    float GetKeyEventOrder() override { return 101.f; }

private:
    struct Model
    {
        Rml::String status;
        Rml::String name;
        Rml::String rank;
        Rml::String characterClass;
        Rml::String level;
        Rml::String resets;
        Rml::String totalStats;
        Rml::String strength;
        Rml::String agility;
        Rml::String vitality;
        Rml::String energy;
        Rml::String command;
        Rml::String account;
    };

    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void SyncModel();
    void Refresh();
    CManager* m_manager = nullptr;
    std::uint16_t m_target = 0;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_rank_user",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_rank_user.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisRankUserWindow* g_pParisRankUserWindow;
} // namespace mu::ui::window
