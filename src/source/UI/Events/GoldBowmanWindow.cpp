#include "stdafx.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/NPCs/ServiceDialogState.h"
#include "UI/Events/GoldBowmanWindow.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Core/WindowGeometry.h"
#include "UI/Dialogs/CommonMessageBox.h"
#include "UI/Inventory/MyInventory.h"
#include "I18N/All.h"

#include "Core/Utilities/StringUtils.h"
#include "UI/RmlBridge/RmlTheme.h"
#include "Network/Paris/ParisPacketDispatch.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <string>

#define MAXGOLDBOWMANSESERIAL 12

using namespace SEASON3B;
using namespace mu::ui::window;

namespace
{
// The original's exit button (newui_exit_00, 36 x 29).
bool g_TrainingMode = false;
std::uint64_t g_TrainingRegistration = 0;
constexpr const wchar_t* kTrainingCommands[] = {L"/pvpbot bk1", L"/pvpbot bk2", L"/pvpbot sm1", L"/pvpbot sm2", L"/pvpbot elf1", L"/pvpbot elf2", L"/pvpbot dl1", L"/pvpbot dl2", L"/pvpbot mg1", L"/pvpbot mg2"};
constexpr const wchar_t* kTrainingLabels[] = {L"BK - Normal", L"BK - Avanzado", L"Mago - Normal", L"Mago - Avanzado", L"Elfa - Normal", L"Elfa - Avanzado", L"DL - Normal", L"DL - Avanzado", L"MG - Normal", L"MG - Avanzado"};
constexpr float kExitWidth = 36.f;
constexpr float kExitHeight = 29.f;

// The original printed its notes through mu_swprintf() as formats ("100%%" shows "100%").
std::wstring Formatted(const wchar_t* format)
{
    wchar_t text[100] = {};
    mu_swprintf(text, format);
    return text;
}
} // namespace

CGoldBowmanWindow::CGoldBowmanWindow()
{
    m_pNewUIMng = NULL;
    ZeroMemory(g_strGiftName, sizeof(g_strGiftName));
}

CGoldBowmanWindow::~CGoldBowmanWindow()
{
    Release();
}

bool CGoldBowmanWindow::Create(CManager* pNewUIMng)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(mu::ui::window::INTERFACE_GOLD_BOWMAN, this);

    // A new document's field is unfocused (a theme switch rebuilds it).
    m_View.SetAfterBuild([this] { m_SerialFocusPending = IsVisible(); });
    m_View.SetWindowId(mu::ui::window::INTERFACE_GOLD_BOWMAN);
    m_View.SetItemDrawer([](int index, const Rml::Vector2f& offset, const Rml::Vector2f& size)
    {
        const auto& dialog = UI::NPCs::ServiceDialog::state;
        if (!dialog.active) return;
        const auto& group = index < static_cast<int>(dialog.requirements.size()) ? dialog.requirements : dialog.rewards;
        const auto slot = index < static_cast<int>(dialog.requirements.size()) ? index : index - static_cast<int>(dialog.requirements.size());
        if (slot < 0 || slot >= static_cast<int>(group.size())) return;
        const auto& item = group[slot];
        ::RenderItem3D(offset.x, offset.y, size.x, size.y, item.type, std::max(0, item.level) << 3, 0, 0, false);
    });
    m_View.Build();
    g_TrainingRegistration = Network::Paris::RegisterHandler(0xED, 0x40,
        [this](const std::uint8_t* packet, int size)
        {
            if (size != 5 || packet[0] != 0xC1 || packet[4] != 1) return;
            g_TrainingMode = true;
            g_pNewUISystem->Show(mu::ui::window::INTERFACE_GOLD_BOWMAN);
        });

    Show(false);

    return true;
}

void CGoldBowmanWindow::Release()
{
    Network::Paris::UnregisterHandler(g_TrainingRegistration);
    g_TrainingRegistration = 0;
    g_TrainingMode = false;
    m_View.Release();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

Rml::Element* CGoldBowmanWindow::GetSerialField() const
{
    return m_View.GetElementById("serial_field");
}

void CGoldBowmanWindow::ClearSerialField()
{
    m_View.SetInputValue(Rml::String());
    if (Rml::Element* field = GetSerialField())
        field->Blur();
}

void CGoldBowmanWindow::OpeningProcess()
{
    ZeroMemory(g_strGiftName, sizeof(g_strGiftName));
    ClearSerialField();
    m_SerialFocusPending = !g_TrainingMode && !UI::NPCs::ServiceDialog::state.active;
}

void CGoldBowmanWindow::ClosingProcess()
{
    ZeroMemory(g_strGiftName, sizeof(g_strGiftName));
    ClearSerialField();
    m_SerialFocusPending = false;
    SocketClient->ToGameServer()->SendEventChipExitDialog();
    g_TrainingMode = false;
    UI::NPCs::ServiceDialog::state = {};
}

bool CGoldBowmanWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_GOLD_BOWMAN) == false) {
        return true;
    }

    // Register, the exit button and the corner close are RmlUi's (see Update()).
    if (m_View.IsPointerOver())
    {
        if (mu::ui::window::IsPress(VK_RBUTTON))
        {
            MouseRButton = false;
            MouseRButtonPop = false;
            MouseRButtonPush = false;
            return false;
        }

        if (mu::ui::window::IsNone(VK_LBUTTON) == false)
        {
            return false;
        }
    }
    else
    {
        if (mu::ui::window::IsNone(VK_LBUTTON) == false)
        {
            return false;
        }
    }

    return true;
}

bool CGoldBowmanWindow::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_GOLD_BOWMAN) == false) {
        return true;
    }

    if (mu::ui::window::IsPress(VK_ESCAPE) == true)
    {
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_GOLD_BOWMAN);
        return false;
    }

    return true;
}

bool CGoldBowmanWindow::Update()
{
    SyncView();

    // A click RmlUi reported (the original's CButton handling in UpdateMouseEvent()).
    const int pressed = m_View.TakePressedButton();
    if (!IsVisible())
        return true;
    if (UI::NPCs::ServiceDialog::state.active && pressed >= 0)
    {
        const auto npc = UI::NPCs::ServiceDialog::state.npc;
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_GOLD_BOWMAN);
        if (pressed == 0)
        {
            const auto command = L"/npcexchange " + std::to_wstring(npc);
            SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(Hero->ID), MU_C16(command.c_str()));
        }
        return true;
    }
    if (g_TrainingMode && pressed >= 0)
    {
        if (pressed < 10)
            SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(Hero->ID), MU_C16(kTrainingCommands[pressed]));
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_GOLD_BOWMAN);
        return true;
    }
    if (pressed == BUTTON_SERIAL)
    {
        SendSerial();
    }
    else if (pressed == BUTTON_EXIT)
    {
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_GOLD_BOWMAN);
    }
    return true;
}

void CGoldBowmanWindow::SendSerial()
{
    mu::ui::window::CInventoryCtrl* pNewInventoryCtrl = g_pMyInventory->GetInventoryCtrl();
    if (pNewInventoryCtrl->FindEmptySlot(2, 4) == -1)
    {
        mu::ui::window::CreateOkMessageBox(I18N::Game::LeaveAtLeastOneEmptySlotInYourInventory);
        return;
    }

    // The number as three groups of four characters, empty groups where it is shorter.
    // The model, not the element: data-value writes the typed serial back into it.
    std::wstring serial = StringUtils::NarrowToWide(m_View.InputValue());
    serial.resize(MAXGOLDBOWMANSESERIAL, L'\0');

    wchar_t strSerial1[5] = {};
    wmemcpy(strSerial1, serial.c_str(), 4);

    wchar_t strSerial2[5] = {};
    wmemcpy(strSerial2, serial.c_str() + 4, 4);

    wchar_t strSerial3[5] = {};
    wmemcpy(strSerial3, serial.c_str() + 8, 4);

    SocketClient->ToGameServer()->SendLuckyNumberRequest(MU_C16(strSerial1), MU_C16(strSerial2), MU_C16(strSerial3));
}

void CGoldBowmanWindow::SyncView()
{
    const auto& serviceState=UI::NPCs::ServiceDialog::state;
    m_View.SetBuffPresentation(serviceState.active && serviceState.requirements.empty() && serviceState.rewards.empty(),serviceState.presentation);

    if (auto* panel = m_View.GetElementById("panel")) panel->SetClass("npc-buff-service", UI::NPCs::ServiceDialog::state.active && UI::NPCs::ServiceDialog::state.requirements.empty() && UI::NPCs::ServiceDialog::state.rewards.empty());
    if (auto* panel = m_View.GetElementById("panel")) panel->SetClass("pvp-training", g_TrainingMode);
    if (auto* panel = m_View.GetElementById("panel")) panel->SetClass("npc-service", UI::NPCs::ServiceDialog::state.active);
    if (auto* panel = m_View.GetElementById("panel")) panel->SetClass("npc-item-service", UI::NPCs::ServiceDialog::state.active && (!UI::NPCs::ServiceDialog::state.requirements.empty() || !UI::NPCs::ServiceDialog::state.rewards.empty()));
    if (UI::NPCs::ServiceDialog::state.active)
    {
        const auto& dialog = UI::NPCs::ServiceDialog::state;
        if (dialog.requirements.empty() && dialog.rewards.empty())
        {
            std::vector<EventItemEntryView::Text> texts{{dialog.title, INVENTORY_WIDTH, true}};
            for (const auto& line : dialog.lines) {
                const auto split=line.find(L" / ");
                if(split!=std::wstring::npos) { texts.push_back({line.substr(0,split),INVENTORY_WIDTH});texts.push_back({line.substr(split+3),INVENTORY_WIDTH}); }
                else {
                    auto shown=line;
                    if(shown.starts_with(L"Level ")) shown=L"Nivel "+shown.substr(6);
                    texts.push_back({shown, INVENTORY_WIDTH});
                }
            }
            if(texts.size()>3) {texts[2].fontScale=.8f;texts[3].fontScale=.8f;}
            m_View.SetTexts(std::move(texts));
            m_View.SetServiceItems(false, {}, {});
            m_View.SetButtons({{.label = L"Obtener buff", .locked = !UI::NPCs::ServiceDialog::CanExchange(dialog), .width = 164.f, .height = 29.f}, {.label = L"Cerrar", .width = 160.f, .height = 27.f}});
            m_View.Sync(IsVisible());
            return;
        }
        std::vector<EventItemEntryView::Text> texts{{dialog.title, INVENTORY_WIDTH, true}};
        // Cost and level remain server-provided; item rows use the structured snapshot.
        if (dialog.lines.size() > 1) texts.push_back({dialog.lines[1], INVENTORY_WIDTH});
        if (dialog.lines.size() > 2)
        {
            const auto number = dialog.lines[2].find_first_of(L"0123456789");
            texts.push_back({number == std::wstring::npos ? dialog.lines[2] : dialog.lines[2].substr(number), INVENTORY_WIDTH});
        }
        m_View.SetTexts(std::move(texts));
        auto entries = [](const auto& items, bool required)
        {
            std::vector<EventItemEntryServiceItem> result;
            for (const auto& item : items)
            {
                const auto level = item.level < 0 ? L" (cualquier nivel)" : L" +" + std::to_wstring(item.level);
                const auto count = required ? std::to_wstring(item.owned) + L" / " + std::to_wstring(item.count) : L"x" + std::to_wstring(item.count);
                const auto hint = item.name + level;
                result.push_back({StringUtils::WideToNarrow(count.c_str()), StringUtils::WideToNarrow(hint.c_str()), required && item.owned < item.count});
            }
            return result;
        };
        m_View.SetServiceItems(true, entries(dialog.requirements, true), entries(dialog.rewards, false));
        m_View.SetButtons({{.label = L"Cambiar", .locked = !UI::NPCs::ServiceDialog::CanExchange(dialog), .width = 164.f, .height = 29.f}, {.label = L"Cerrar", .width = 160.f, .height = 27.f}});
        m_View.Sync(IsVisible());
        return;
    }
    m_View.SetServiceItems(false, {}, {});
    if (g_TrainingMode)
    {
        m_View.SetTexts({{L"ENTRENAMIENTO PVP", INVENTORY_WIDTH, true}, {L"Elige tu rival", INVENTORY_WIDTH}});
        std::vector<EventItemEntryView::Button> buttons;
        for (const auto* label : kTrainingLabels)
            buttons.push_back({.label = label, .width = 160.f, .height = 24.f});
        buttons.push_back({.label = L"Cerrar", .width = 160.f, .height = 24.f});
        m_View.SetButtons(buttons);
        m_View.Sync(IsVisible());
        return;
    }

    if (IsVisible())
    {
        // The original's RenderTexts(): every line in the normal font, centred on the 190-unit
        // window; the example number green, the registered gift's name (from the server's answer)
        // at y 330.
        const float width = INVENTORY_WIDTH;
        std::vector<EventItemEntryView::Text> texts{
            {getMonsterName(236), width},
            {Formatted(I18N::Game::EnterThe12DigitLuckyNumber), width},
            {Formatted(I18N::Game::WrittenOnThe100WinningCard), width},
            {Formatted(I18N::Game::LuckyNumberRegistrationPeriod), width},
            {Formatted(I18N::Game::Oct282003Nov30), width},
            {Formatted(I18N::Game::EnterTheLuckyNumber), width},
            {Formatted(I18N::Game::ExAUS919DKL2J9), width},
            {Formatted(I18N::Game::PleaseMakeSureToDifferentiate), width},
            {Formatted(I18N::Game::AlphabetOAndNumber0AndAlphabetIAndNumber1), width}};
        // The registered gift's name, from the server's answer -- always the last line, so the
        // rows above it keep their own rules whether it is there or not.
        if (wcscmp(g_strGiftName, L"") != 0)
            texts.push_back({g_strGiftName, width});
        m_View.SetTexts(std::move(texts));

        m_View.SetButtons({{.label = I18N::Game::LuckyNumberRegistered,
                            .width = MSGBOX_BTN_EMPTY_WIDTH,
                            .height = MSGBOX_BTN_EMPTY_HEIGHT},
                           {.width = kExitWidth, .height = kExitHeight, .hint = I18N::Game::Close388}});
    }
    m_View.Sync(IsVisible());

    if (!IsVisible())
        return;

    Rml::Element* field = GetSerialField();
    if (field == nullptr)
        return;
    if (m_SerialFocusPending)
    {
        field->Focus();
        if (field->IsPseudoClassSet("focus"))
            m_SerialFocusPending = false;
    }
}

// The original pointed its related window at the focused edit box, so Escape still closes the
// window while the player types.
bool CGoldBowmanWindow::TakesTypingFrom(const Rml::ElementDocument* document) const
{
    const Rml::Element* field = GetSerialField();
    return field != nullptr && field->GetOwnerDocument() == document;
}

bool CGoldBowmanWindow::Render()
{
    // Nothing native left: the frame, the texts, the field and the buttons are RmlUi.
    return true;
}

float CGoldBowmanWindow::GetLayerDepth()
{
    return 3.4f;
}
