#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include <array>
#include <vector>

namespace mu::ui::window
{
class CManager;
class CParisSoulWindow final : public CObject
{
public:
    ~CParisSoulWindow() override;
    bool Create(CManager* manager);
    void Release();
    void OpenWindow();
    void Show(bool show) override;
    bool UpdateMouseEvent() override { return !IsVisible(); }
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override { return 62.5f; }
    float GetKeyEventOrder() override { return 101.f; }
private:
    struct Tier { int index = 0, rank = 0; bool unlocked = false; Rml::String top, hint; };
    struct Column
    {
        int index = 0, rank = 0;
        Rml::String name, benefit, progress;
        bool add = false, subtract = false;
        std::vector<Tier> tiers;
    };
    struct Model
    {
        Rml::String remaining, status, confirmation;
        int category = 0;
        bool loading = true, pending = false, confirmReset = false, canApply = false, canReset = false;
        std::vector<Column> columns;
    };
    void BindModel(Rml::DataModelConstructor& c, Model& model);
    void SyncModel();
    void RefreshModel();
    void Send(int action);
    void ChangeRank(int column, int delta);
    CManager* m_manager = nullptr;
    std::array<int, 16> m_draft{};
    DWORD m_observed = 0, m_sent = 0;
    Rml::DataModelHandle m_handle;
    bool m_refresh = true, m_queriedAfterTimeout = false;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_soul",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_soul.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};
extern CParisSoulWindow* g_pParisSoulWindow;
}
