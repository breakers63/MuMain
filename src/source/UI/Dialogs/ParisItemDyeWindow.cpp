#include "stdafx.h"
#include "ParisItemDyeWindow.h"
#include "ParisWindowGroup.h"
#include "GenericConfirmDialog.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/MyInventory.h"
#include "UI/Inventory/InventoryCtrl.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "Core/Utilities/StringUtils.h"
#include "Engine/Object/ZzzInventory.h"
#include "Render/Models/ItemDyeAppearance.h"
#include "Render/Items/ItemPreviewFit.h"
#include <RmlUi/Core/Event.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mu::ui::window {
namespace Dye = Network::Paris::ItemDye;
namespace {
constexpr unsigned PollInterval = 1500, ApplyInterval = 700;
constexpr int FirstArmorGroup = 7, LastArmorGroup = 11, FirstPotion = ITEM_POTION + 450;
constexpr const char* TargetIds[] = {"dye_preview", "dye_original", "dye_new",
    "dye_piece_0", "dye_piece_1", "dye_piece_2", "dye_piece_3", "dye_piece_4",
    "dye_potion_0", "dye_potion_1", "dye_potion_2"};
int HsvColor(float hue, float saturation, float value)
{
    const float sector = (hue - std::floor(hue)) * 6.f;
    const int index = static_cast<int>(sector);
    const float fraction = sector - index;
    const float p = value * (1 - saturation), q = value * (1 - fraction * saturation);
    const float t = value * (1 - (1 - fraction) * saturation);
    const float rgb[6][3] = {{value,t,p},{q,value,p},{p,value,t},{p,q,value},{t,p,value},{value,p,q}};
    return (int(rgb[index][0] * 255 + .5f) << 16) | (int(rgb[index][1] * 255 + .5f) << 8) | int(rgb[index][2] * 255 + .5f);
}
Rml::String Hex(int color)
{
    char buffer[10]; sprintf_s(buffer, "#%06X", color); return buffer;
}
}
CParisItemDyeWindow* g_pParisItemDyeWindow = nullptr;
CParisItemDyeWindow::~CParisItemDyeWindow() { Release(); }
bool CParisItemDyeWindow::Create(CManager* owner)
{
    if (!owner) return false;
    manager = owner; owner->AddUIObj(0x50445945u, this); CObject::Show(false); return true;
}
void CParisItemDyeWindow::Release()
{
    for (auto& target : targets) target.reset();
    view.Release();
    if (manager) manager->RemoveUIObj(this);
    manager = nullptr;
}
void CParisItemDyeWindow::Bind(Rml::DataModelConstructor& constructor, Model& model)
{
    handle = constructor.GetModelHandle();
    model.pieces.resize(PieceCount);
    for (int index = 0; index < PieceCount; ++index) model.pieces[index].index = index;
    auto piece = constructor.RegisterStruct<Piece>();
    piece.RegisterMember("label", &Piece::label); piece.RegisterMember("index", &Piece::index);
    piece.RegisterMember("selected", &Piece::selected); piece.RegisterMember("focused", &Piece::focused);
    constructor.RegisterArray<std::vector<Piece>>();
    auto swatch = constructor.RegisterStruct<Swatch>();
    swatch.RegisterMember("color", &Swatch::color); swatch.RegisterMember("value", &Swatch::value);
    constructor.RegisterArray<std::vector<Swatch>>();
    constructor.Bind("pieces", &model.pieces); constructor.Bind("palette", &model.palette);
    constructor.Bind("status", &model.status); constructor.Bind("hex", &model.hex);
    constructor.Bind("color", &model.color); constructor.Bind("hue_color", &model.hueColor);
    constructor.Bind("cursor_x", &model.cursorX); constructor.Bind("cursor_y", &model.cursorY);
    constructor.Bind("hue_x", &model.hueX); constructor.Bind("red", &model.red);
    constructor.Bind("green", &model.green); constructor.Bind("blue", &model.blue);
    constructor.Bind("item_name", &model.itemName); constructor.Bind("count", &model.count);
    constructor.Bind("cost", &model.cost);
    constructor.Bind("red_count", &model.redCount); constructor.Bind("green_count", &model.greenCount);
    constructor.Bind("blue_count", &model.blueCount); constructor.Bind("multiple", &model.multiple);
    constructor.Bind("busy", &model.busy); constructor.Bind("can_apply", &model.canApply);
    constructor.Bind("can_restore", &model.canRestore);
    model.palette.clear();
    for (int row = 0; row < 9; ++row) for (int column = 0; column < 14; ++column) {
        const float saturation = column == 13 ? 0.f : row < 5 ? 1.f : 1.f - (row - 4) * .2f;
        const int rgb = HsvColor(column / 14.f, saturation, row < 5 ? .2f + row * .2f : 1.f);
        model.palette.push_back({Hex(rgb), rgb});
    }
    constructor.BindEventCallback("dye_close", [this](auto, auto&, const auto&) { Show(false); });
    constructor.BindEventCallback("dye_mode", [this](auto, auto&, const Rml::VariantList& args) {
        if (!args.empty()) SetMode(args[0].Get<bool>());
    });
    constructor.BindEventCallback("dye_piece", [this](auto, auto&, const Rml::VariantList& args) {
        if (!args.empty()) FocusPiece(args[0].Get<int>());
    });
    constructor.BindEventCallback("dye_step", [this](auto, auto&, const Rml::VariantList& args) {
        if (!args.empty()) StepPreview(args[0].Get<int>());
    });
    constructor.BindEventCallback("dye_swatch", [this](auto, auto&, const Rml::VariantList& args) {
        if (!args.empty() && queue.empty()) SetColor(args[0].Get<int>());
    });
    constructor.BindEventCallback("dye_apply", [this](auto, auto&, const auto&) { ConfirmApply(false); });
    constructor.BindEventCallback("dye_restore", [this](auto, auto&, const auto&) { ConfirmApply(true); });
}
void CParisItemDyeWindow::OpenWindow()
{
    if (!view.Ensure()) return;
    for (size_t index = 0; index < targets.size(); ++index) if (!targets[index]) {
        targets[index] = std::make_unique<UI::Items::ItemCameraTarget>([this,index](const auto& offset,const auto& size) { DrawTarget(index,offset,size); });
        targets[index]->SetContinuous(false);
    }
    Show(true);
    g_pNewUISystem->Show(INTERFACE_INVENTORY);
    Dye::Request(); lastPoll = timeGetTime(); revision = 0; Sync();
}
void CParisItemDyeWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (view.Document()) {
        if (show) view.Document()->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        else view.Document()->Hide();
    }
    if (!show) {
        for (auto& target : targets) if (target) target->Disable();
        queue.clear(); awaiting = false; batchStatus.clear(); drag = 0;
    }
}
bool CParisItemDyeWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { Show(false); return false; }
    return !IsVisible();
}
bool CParisItemDyeWindow::UpdateMouseEvent()
{
    return !IsVisible() || !UI::RmlBridge::IsPointerOver(view.Document());
}
const CParisItemDyeWindow::Entry* CParisItemDyeWindow::Find(const std::string& id) const
{
    for (const auto& item : Dye::Inventory) if (item.Id == id && item.Eligible) return &item;
    return nullptr;
}
const CParisItemDyeWindow::Entry* CParisItemDyeWindow::PieceAt(int column) const
{
    for (const auto& id : selection) if (const auto* item = Find(id)) {
        if (item->Type / MAX_ITEM_INDEX == FirstArmorGroup + column) return item;
        if (column == 0 && item->Type / MAX_ITEM_INDEX < FirstArmorGroup) return item;
    }
    return nullptr;
}
bool CParisItemDyeWindow::SelectInventoryItem(DWORD key)
{
    if (!IsVisible()) return false;
    if (Dye::Pending || !queue.empty()) return true;
    const auto* local = g_pMyInventory->FindItemByKey(key);
    if (!local) return true;
    const int inventorySlot = g_pMyInventory->GetInventoryCtrl()->GetIndexByItem(const_cast<ITEM*>(local));
    for (const auto& item : Dye::Inventory) {
        if (!item.Eligible || item.Slot < MAX_EQUIPMENT) continue;
        if (item.Slot != inventorySlot || item.Type != local->Type) continue;
        const int group = item.Type / MAX_ITEM_INDEX;
        if (!multiple || group < FirstArmorGroup) selection.clear();
        else selection.erase(std::remove_if(selection.begin(),selection.end(),[&](const auto& id) {
            const auto* previous = Find(id);
            return previous && previous->Id != item.Id && (previous->Type / MAX_ITEM_INDEX == group || previous->Type / MAX_ITEM_INDEX < FirstArmorGroup);
        }),selection.end());
        if (std::find(selection.begin(),selection.end(),item.Id) == selection.end()) selection.push_back(item.Id);
        selected = item.Id; batchStatus.clear(); revision = 0; Sync(); InvalidateTargets(); return true;
    }
    g_ErrorReport.Write(L"RGB selection unavailable: slot=%d type=%d snapshot=%u rows=%u\r\n", inventorySlot, local->Type, Dye::Received, static_cast<unsigned>(Dye::Inventory.size()));
    Dye::Request();
    batchStatus = Dye::Received ? "Esperando la posicion actual de la pieza. Intenta de nuevo." : "Esperando sistema RGB del servidor";
    revision = 0; Sync(); return true;
}
void CParisItemDyeWindow::SetMode(bool mode)
{
    if (!queue.empty()) return;
    multiple = mode;
    if (!multiple && !selected.empty()) selection = {selected};
    revision = 0; Sync(); InvalidateTargets();
}
void CParisItemDyeWindow::FocusPiece(int column)
{
    if (column < 0 || column >= PieceCount || !queue.empty()) return;
    if (const auto* item = PieceAt(column)) selected = item->Id;
    else g_pNewUISystem->Show(INTERFACE_INVENTORY);
    revision = 0; Sync(); InvalidateTargets();
}
void CParisItemDyeWindow::StepPreview(int direction)
{
    if (selection.empty()) return;
    auto order = selection;
    std::stable_sort(order.begin(),order.end(),[this](const auto& a,const auto& b) {
        const auto* left = Find(a); const auto* right = Find(b);
        return left && right && left->Type / MAX_ITEM_INDEX < right->Type / MAX_ITEM_INDEX;
    });
    const auto current = std::find(order.begin(),order.end(),selected);
    const auto index = current == order.end() ? 0 : std::distance(order.begin(),current);
    selected = order[(index + (direction < 0 ? order.size() - 1 : 1)) % order.size()];
    revision = 0; Sync(); InvalidateTargets();
}
void CParisItemDyeWindow::InvalidateTargets()
{
    for (auto& target : targets) if (target) target->Invalidate();
}
void CParisItemDyeWindow::DrawTarget(size_t index,const Rml::Vector2f& offset,const Rml::Vector2f& size)
{
    const Entry* item = index < 3 ? Find(selected) : index < 8 ? PieceAt(static_cast<int>(index - 3)) : nullptr;
    // Use the native red, antidote-green and mana-blue bottle artwork for the reagent indicators.
    constexpr int PotionPreviewTypes[] = {ITEM_POTION + 1, ITEM_POTION + 8, ITEM_POTION + 4};
    const int type = index >= 8 ? PotionPreviewTypes[index - 8] : item ? item->Type : -1;
    if (type < 0) return;
    const int tint = index == 1 ? -1 : index < 8 ? color : -1;
    Render::ItemDye::PreviewScope previewTint(tint);
    Render::Items::Preview::ScopedFit fit(offset.x,offset.y,size.x,size.y);
    RenderItem3D(offset.x,offset.y,size.x,size.y,type,item ? item->Level : 0,0,0,false);
}
void CParisItemDyeWindow::SyncTargets()
{
    auto* document = view.Document();
    for (size_t index = 0; index < targets.size(); ++index) if (targets[index]) {
        const bool enabled = index >= 8 || (index < 3 ? Find(selected) != nullptr : PieceAt(static_cast<int>(index - 3)) != nullptr);
        targets[index]->Sync(document->GetElementById(TargetIds[index]),enabled);
    }
}
int CParisItemDyeWindow::PotionSlot(int channel) const
{
    for (const auto& item : Dye::Inventory) if (item.Slot >= MAX_EQUIPMENT && item.Type == FirstPotion + channel && item.Level == 0) return item.Slot;
    return -1;
}
int CParisItemDyeWindow::PotionCount(int channel) const
{
    return static_cast<int>(std::count_if(Dye::Inventory.begin(),Dye::Inventory.end(),[channel](const auto& item) {
        return item.Slot >= MAX_EQUIPMENT && item.Type == FirstPotion + channel && item.Level == 0;
    }));
}
bool CParisItemDyeWindow::MaterialsReady() const
{
    if (selection.empty() || !Dye::Received) return false;
    if (!Dye::RequireReagents) return true;
    for (int channel = 0; channel < 3; ++channel)
        if (PotionCount(channel) < static_cast<int>(selection.size()) * Dye::PotionsPerColor) return false;
    return true;
}
void CParisItemDyeWindow::RefreshColor()
{
    color = HsvColor(hue,saturation,value);
    auto& model = view.GetModel(); model.color = model.hex = Hex(color);
    model.hueColor = Hex(HsvColor(hue,1.f,1.f));
    model.red = std::to_string((color >> 16) & 255); model.green = std::to_string((color >> 8) & 255); model.blue = std::to_string(color & 255);
    model.cursorX = std::to_string(saturation * 100) + "%"; model.cursorY = std::to_string((1 - value) * 100) + "%";
    model.hueX = std::to_string(hue * 100) + "%";
    handle.DirtyAllVariables(); InvalidateTargets();
}
void CParisItemDyeWindow::SetColor(int rgb)
{
    rgb = std::clamp(rgb,0,0xFFFFFF);
    const float r = ((rgb >> 16) & 255) / 255.f, g = ((rgb >> 8) & 255) / 255.f, b = (rgb & 255) / 255.f;
    const float high = std::max({r,g,b}), low = std::min({r,g,b}), delta = high - low;
    value = high; saturation = high > 0 ? delta / high : 0;
    if (delta > 0) {
        hue = (high == r ? (g-b)/delta : high == g ? (b-r)/delta+2 : (r-g)/delta+4) / 6.f;
        if (hue < 0) hue += 1;
    }
    RefreshColor();
}
void CParisItemDyeWindow::UpdatePicker()
{
    const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    auto* document = view.Document();
    if (!down) drag = 0;
    if (down && !pointerDown && queue.empty()) {
        if (UI::RmlBridge::IsPointerWithin(document->GetElementById("dye_sv"))) drag = 1;
        else if (UI::RmlBridge::IsPointerWithin(document->GetElementById("dye_hue"))) drag = 2;
    }
    pointerDown = down;
    if (!down || !drag) return;
    auto* element = document->GetElementById(drag == 1 ? "dye_sv" : "dye_hue");
    Rml::Vector2f local;
    if (!element || !UI::RmlBridge::PointerIn(element,local)) return;
    const auto size = element->GetBox().GetSize(Rml::BoxArea::Content);
    if (size.x <= 0 || size.y <= 0) return;
    if (drag == 1) { saturation = std::clamp(local.x/size.x,0.f,1.f); value = 1 - std::clamp(local.y/size.y,0.f,1.f); }
    else hue = std::clamp(local.x/size.x,0.f,.99999f);
    RefreshColor();
}
void CParisItemDyeWindow::Sync()
{
    if (revision == Dye::Revision && revision != 0) return;
    revision = Dye::Revision;
    selection.erase(std::remove_if(selection.begin(),selection.end(),[this](const auto& id) { return !Find(id); }),selection.end());
    if (!Find(selected)) selected = selection.empty() ? std::string() : selection.back();
    auto& model = view.GetModel(); model.pieces.clear();
    constexpr const char* names[] = {"CASCO","PECHERA","PANTALON","GUANTES","BOTAS"};
    for (int index = 0; index < PieceCount; ++index) {
        const auto* item = PieceAt(index);
        model.pieces.push_back({names[index],index,item != nullptr,item && item->Id == selected});
    }
    model.multiple = multiple; model.busy = Dye::Pending || !queue.empty();
    model.canApply = !model.busy && MaterialsReady(); model.canRestore = !model.busy && !selection.empty();
    model.count = std::to_string(selection.size()) + " pieza(s) seleccionada(s)";
    model.itemName = Find(selected) ? StringUtils::WideToNarrow(Find(selected)->Name.c_str()) : "Clic derecho sobre una pieza del inventario";
    const int required = Dye::RequireReagents ? static_cast<int>(selection.size()) * Dye::PotionsPerColor : 0;
    model.redCount = std::to_string(PotionCount(0)) + "/" + std::to_string(required);
    model.greenCount = std::to_string(PotionCount(1)) + "/" + std::to_string(required);
    model.blueCount = std::to_string(PotionCount(2)) + "/" + std::to_string(required);
    model.cost = Dye::RequireReagents ? std::to_string(Dye::PotionsPerColor) + " de cada color por pieza" : "Pruebas: sin consumir pociones";
    model.status = !batchStatus.empty() ? batchStatus : !Dye::Received ? "Esperando sistema RGB del servidor" : "Solo apariencia. Atributos intactos.";
    handle.DirtyAllVariables(); InvalidateTargets();
}
void CParisItemDyeWindow::ConfirmApply(bool restore)
{
    if (Dye::Pending || !queue.empty() || selection.empty() || (!restore && !MaterialsReady())) return;
    GenericDialogConfig config; config.title = restore ? L"Restaurar original" : L"Aplicar color";
    config.lines.push_back({restore ? L"Restaurar las piezas seleccionadas sin consumir pociones." :
        Dye::RequireReagents ? L"Se consumiran las pociones indicadas por cada pieza." : L"Prueba sin consumir pociones.",false});
    config.primaryLabel = L"Confirmar"; config.showCancel = true; config.cancelLabel = L"Cancelar";
    config.onPrimary = [this,restore] { if (IsVisible()) BeginBatch(restore); };
    g_pGenericConfirmDialog->Show(std::move(config));
}
void CParisItemDyeWindow::BeginBatch(bool restore)
{
    if (selection.empty() || (!restore && !MaterialsReady())) return;
    queue = selection; queueIndex = 0; restoring = restore; queuedColor = color; awaiting = false; nextApply = timeGetTime();
    batchStatus = "Guardando piezas..."; revision = 0; Sync();
}
void CParisItemDyeWindow::UpdateBatch()
{
    if (queue.empty()) return;
    if (awaiting) {
        if (Dye::Pending || Dye::Revision <= queueRevision) return;
        if (Dye::Result != 0) {
            batchStatus = "No se pudo aplicar. Revisa el objeto y los materiales.";
            queue.clear(); awaiting = false; revision = 0; return;
        }
        awaiting = false; ++queueIndex; nextApply = timeGetTime() + ApplyInterval;
        if (queueIndex >= queue.size()) { queue.clear(); batchStatus = restoring ? "Color original restaurado." : "Color guardado."; revision = 0; return; }
    }
    if (Dye::Pending || timeGetTime() < nextApply) return;
    const auto* item = Find(queue[queueIndex]);
    if (!item) { queue.clear(); batchStatus = "Una pieza cambio de posicion. Seleccionala otra vez."; revision = 0; return; }
    const int slots[] = {PotionSlot(0),PotionSlot(1),PotionSlot(2)};
    queueRevision = Dye::Revision;
    if (restoring) Dye::RestoreOriginal(*item); else Dye::Apply(*item,slots,queuedColor);
    awaiting = Dye::Pending;
    if (!awaiting) { queue.clear(); batchStatus = "No se pudo enviar el cambio."; }
    revision = 0;
}
void CParisItemDyeWindow::HandleItemDrop()
{
    auto* picked = CInventoryCtrl::GetPickedItem();
    const bool released = IsRelease(VK_LBUTTON) || (pointerDown && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0);
    if (!picked || !released || picked->GetOwnerInventory() != g_pMyInventory->GetInventoryCtrl()) return;
    const auto* item = picked->GetItem();
    if (!item) return;
    const int group = item->Type / MAX_ITEM_INDEX;
    const int column = group >= FirstArmorGroup && group <= LastArmorGroup ? group - FirstArmorGroup : 0;
    auto* image = view.Document()->GetElementById(TargetIds[column + 3]);
    auto* slotElement = image ? image->GetParentNode() : nullptr;
    // An empty render-target image is hidden. Its parent slot remains the drop target.
    if (group > LastArmorGroup || !UI::RmlBridge::IsPointerWithin(slotElement)) return;
    const int slot = picked->GetSourceLinealPos();
    CInventoryCtrl::BackupPickedItem();
    if (const auto* restored = g_pMyInventory->FindItem(slot)) SelectInventoryItem(restored->Key);
}
void CParisItemDyeWindow::HandleInventoryPointer()
{
    const bool down = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    if (down && !rightPointerDown && g_pMyInventory && g_pMyInventory->IsVisible())
    {
        if (const auto* item = g_pMyInventory->GetInventoryCtrl()->FindItemAtPointer())
        {
            SelectInventoryItem(item->Key);
            MouseRButton = MouseRButtonPop = MouseRButtonPush = false;
        }
    }
    rightPointerDown = down;
}
bool CParisItemDyeWindow::Update()
{
    if (!IsVisible()) return true;
    if (SceneFlag != MAIN_SCENE) { Show(false); return true; }
    if (timeGetTime() - lastPoll > PollInterval) { Dye::Request(); lastPoll = timeGetTime(); }
    HandleInventoryPointer(); HandleItemDrop(); UpdateBatch(); UpdatePicker(); Sync(); SyncTargets(); return true;
}
}



