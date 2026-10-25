#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include "Network/Paris/ItemDyeProtocol.h"
#include <array>
#include <memory>
#include <vector>

namespace mu::ui::window {
class CManager;
class CParisItemDyeWindow final : public CObject {
public:
    ~CParisItemDyeWindow() override;
    bool Create(CManager* manager);
    void Release();
    void OpenWindow();
    void Show(bool show) override;
    bool Update() override;
    bool Render() override { return true; }
    bool UpdateKeyEvent() override;
    bool UpdateMouseEvent() override;
    bool SelectInventoryItem(DWORD key);
    float GetLayerDepth() override { return 62.5f; }
    float GetKeyEventOrder() override { return 101.f; }
private:
    static constexpr size_t PieceCount = 5;
    static constexpr size_t PreviewCount = 11;
    struct Piece { Rml::String label; int index = 0; bool selected = false, focused = false; };
    struct Swatch { Rml::String color; int value = 0; };
    struct Model {
        std::vector<Piece> pieces;
        std::vector<Swatch> palette;
        Rml::String status, hex = "#33FFFF", red = "51", green = "255", blue = "255";
        Rml::String itemName = "Selecciona piezas", color = "#33FFFF", hueColor = "#00FFFF";
        Rml::String cursorX = "80%", cursorY = "0%", hueX = "50%";
        Rml::String count = "0 piezas seleccionadas", cost = "";
        Rml::String redCount, greenCount, blueCount;
        bool multiple = true, busy = false, canApply = false, canRestore = false;
    };
    using Entry = Network::Paris::ItemDye::Entry;
    void Bind(Rml::DataModelConstructor&, Model&);
    void Sync();
    void SyncTargets();
    void DrawTarget(size_t index, const Rml::Vector2f&, const Rml::Vector2f&);
    void RefreshColor();
    void SetColor(int rgb);
    void UpdatePicker();
    void HandleItemDrop();
    void HandleInventoryPointer();
    void SetMode(bool multiple);
    void FocusPiece(int column);
    void StepPreview(int direction);
    void ConfirmApply(bool restore);
    void BeginBatch(bool restore);
    void UpdateBatch();
    void InvalidateTargets();
    bool MaterialsReady() const;
    int PotionSlot(int channel) const;
    int PotionCount(int channel) const;
    const Entry* Find(const std::string& id) const;
    const Entry* PieceAt(int column) const;
    CManager* manager = nullptr;
    unsigned revision = 0, lastPoll = 0, queueRevision = 0, nextApply = 0;
    std::string selected;
    std::vector<std::string> selection, queue;
    size_t queueIndex = 0;
    bool multiple = true, restoring = false, awaiting = false, pointerDown = false, rightPointerDown = false;
    int drag = 0, color = 0x33FFFF, queuedColor = 0;
    float hue = .5f, saturation = .8f, value = 1.f;
    Rml::String batchStatus;
    Rml::DataModelHandle handle;
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, PreviewCount> targets;
    UI::RmlBridge::ThemedView<Model> view {"paris_item_dye", [this](auto& c, auto& m) { Bind(c, m); },
        {{"Data/Interface/RmlUi/paris_item_dye.rml"}}};
};
extern CParisItemDyeWindow* g_pParisItemDyeWindow;
}
