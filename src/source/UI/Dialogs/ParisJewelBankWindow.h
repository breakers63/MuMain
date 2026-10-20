#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <RmlUi/Core/Types.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Rml { class ElementDocument; }

namespace mu::ui::window
{
class CManager;

class CParisJewelBankWindow final : public CObject
{
public:
    static constexpr DWORD InterfaceId = 0x504A424Bu;

    ~CParisJewelBankWindow() override;
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
    struct JewelEntry
    {
        Rml::String name;
        Rml::String balance;
        Rml::String inventory;
        bool alternate = false;
        int index = 0;
        int item = -1;
        int level = 0;
        bool operator==(const JewelEntry&) const = default;
    };
    struct Model
    {
        Rml::String status;
        Rml::String search;
        std::vector<JewelEntry> entries;
        bool pending = false;
        bool empty = true;
        bool auto_pick = false;
    };

    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void SyncModel();
    void RequestList();
    void RequestCommand(const std::wstring& operation, const char* status);
    void RequestTransfer(int index, bool deposit, int amount);
    bool ReceiveText(std::wstring_view text);
    void CompleteSnapshot();
    void RefreshDisplayedRows();
    void ClickRefresh();
    void ClickAutoPick();
    void DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size);

    CManager* m_manager = nullptr;
    std::mutex m_receiveMutex;
    std::vector<std::wstring> m_pendingTexts;
    bool m_acceptPackets = false;
    bool m_closeRequested = false;
    std::uint64_t m_textHandler = 0;
    std::uint32_t m_nextRequest = 1;
    std::uint32_t m_pendingRequest = 0;
    DWORD m_requestStartedAt = 0;
    bool m_headerReceived = false;
    bool m_hasSnapshot = false;
    bool m_autoPick = false;
    int m_expectedEntries = 0;
    std::vector<bool> m_receivedEntries;
    std::vector<JewelEntry> m_entries;
    std::vector<JewelEntry> m_displayEntries;
    std::vector<std::unique_ptr<UI::Items::ItemCameraTarget>> m_itemTargets;
    Rml::String m_status = "Sin datos del servidor";
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_jewel_bank",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_jewel_bank.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisJewelBankWindow* g_pParisJewelBankWindow;
} // namespace mu::ui::window
