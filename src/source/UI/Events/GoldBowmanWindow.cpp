#include "stdafx.h"
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
    m_SerialFocusPending = !g_TrainingMode;
}

void CGoldBowmanWindow::ClosingProcess()
{
    ZeroMemory(g_strGiftName, sizeof(g_strGiftName));
    ClearSerialField();
    m_SerialFocusPending = false;
    SocketClient->ToGameServer()->SendEventChipExitDialog();
    g_TrainingMode = false;
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
    if (auto* panel = m_View.GetElementById("panel")) panel->SetClass("pvp-training", g_TrainingMode);
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
