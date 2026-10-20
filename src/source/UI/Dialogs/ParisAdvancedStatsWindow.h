#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "Network/Paris/AdvancedStatsField.h"
#include "Network/Paris/ParisPacketDispatch.h"

#include <array>
#include <cstdint>
#include <vector>

namespace mu::ui::window
{
class CManager;

class CParisAdvancedStatsWindow final : public CObject
{
public:
    static constexpr DWORD InterfaceId = 0x41535453u;
    static constexpr int FieldCount = static_cast<int>(Network::Paris::AdvancedStatsField::Count);
    ~CParisAdvancedStatsWindow() override;
    bool Create(CManager* manager);
    void Release();
    void OpenWindow();
    void CloseWindow();
    void Show(bool show) override;
    Rml::ElementDocument* GetPlacedDocument() const override { return m_view.Document(); }
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override { return 5.2f; }
    float GetKeyEventOrder() override { return 101.f; }

private:
    struct Row
    {
        Rml::String label;
        Rml::String value;
        bool section = false;
        bool alternate = false;
        bool operator==(const Row&) const = default;
    };
    struct Model
    {
        float textPx = 0.f;
        Rml::String status;
        Rml::String totalPower;
        Rml::String totalDefense;
        std::vector<Row> rows;
        bool operator==(const Model&) const = default;
    };

    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void ReceivePacket(const std::uint8_t* packet, int size);
    void RequestSnapshot();
    void SyncModel();

    CManager* m_manager = nullptr;
    UI::RmlBridge::ThemedView<Model> m_view{"paris_advanced_stats",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_advanced_stats.rml"}},
        {.modal = Rml::ModalFlag::None, .focus = Rml::FocusFlag::None}};
    std::uint64_t m_packetHandler = 0;
    std::array<std::int32_t, FieldCount> m_values{};
    DWORD m_lastRequestTick = 0;
    DWORD m_requestSentTick = 0;
    DWORD m_lastSnapshotTick = 0;
    bool m_open = false;
    bool m_pending = false;
    bool m_hasSnapshot = false;
    bool m_modelDirty = true;
    int m_receivedFields = 0;
};

extern CParisAdvancedStatsWindow* g_pParisAdvancedStatsWindow;
} // namespace mu::ui::window
