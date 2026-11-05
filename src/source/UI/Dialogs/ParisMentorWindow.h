#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include <memory>
#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisMentorWindow final : public CObject
{
public:
    ~CParisMentorWindow() override;
    bool Create(CManager* manager);
    void Release();
    void OpenWindow();
    void Show(bool show) override;
    bool UpdateMouseEvent() override { return !IsVisible(); }
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override { return 63.f; }
    float GetKeyEventOrder() override { return 102.f; }

private:
    struct Row
    {
        Rml::String name, detail, value, extra, portrait, action;
        int index = 0;
        bool enabled = false;
    };
    struct Model
    {
        bool mentor = false, ready = false, confirm = false, canAccept = false;
        int tab = 0, page = 1, pages = 1, reset = 0;
        Rml::String role, assigned, requests;
        bool canCancel = false;
        Rml::String status, query, name, points, graduated, daily, bonus;
        Rml::String active, hours, progress, activeWidth, confirmText;
        Rml::String detailName, detailMentor, detailState, detailEvent, detailDate, detailPoints;
        std::vector<Row> people, top, offers, history, recent, details;
    };
    void BindModel(Rml::DataModelConstructor& c, Model& model);
    void BindActions(Rml::DataModelConstructor& c);
    void SyncModel();
    void SyncPeople();
    void SyncOffers();
    void SyncHistory();
    void SelectHistory(int index);
    void SelectPerson(int index);
    void Send(int action = 0, const std::string& identity = {}, const std::wstring& target = {});
    void ChangeTab(int tab);
    void ChangePage(int direction);
    void Redeem(int index);
    void ConfirmReward();
    bool Current() const;
    void DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size);

    Rml::DataModelHandle m_modelHandle;
    CManager* m_manager = nullptr;
    DWORD m_lastRequest = 0, m_seen = 0, m_opened = 0;
    bool m_waiting = false;
    int m_filter = 0, m_selected = 0;
    std::string m_confirmation;
    std::uint64_t m_confirmToken = 0;
    int m_confirmCost = 0;
    std::vector<std::unique_ptr<UI::Items::ItemCameraTarget>> m_itemTargets;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_mentor",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_mentor.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisMentorWindow* g_pParisMentorWindow;
} // namespace mu::ui::window
