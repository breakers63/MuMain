#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisJewelBankWindow.h"

#include "Engine/Object/ZzzInventory.h" // RenderItem3D
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Models/ZzzBMD.h"
#include "Network/Paris/JewelBankProtocol.h"
#include "Network/Paris/ParisPacketDispatch.h"
#include "Network/Paris/ParisRequests.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "Core/Utilities/StringUtils.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace mu::ui::window
{
CParisJewelBankWindow* g_pParisJewelBankWindow = nullptr;

CParisJewelBankWindow::~CParisJewelBankWindow()
{
    Release();
}

bool CParisJewelBankWindow::Create(CManager* manager)
{
    if (!manager)
        return false;

    m_manager = manager;
    m_acceptPackets = true;
    m_textHandler = Network::Paris::RegisterTextHandler(
        [this](std::wstring_view text)
        {
            if (!text.starts_with(L"[#PJB"))
                return false;
            std::lock_guard lock(m_receiveMutex);
            if (m_acceptPackets)
                m_pendingTexts.emplace_back(text);
            return true;
        });
    manager->AddUIObj(InterfaceId, this);
    // Build the document when the player opens the panel. At Create() time the RmlUi context
    // may not exist yet, and pre-building it can leave a model tied to a context being torn down.
    CObject::Show(false);
    return true;
}

void CParisJewelBankWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto entry = c.RegisterStruct<JewelEntry>();
    entry.RegisterMember("name", &JewelEntry::name);
    entry.RegisterMember("balance", &JewelEntry::balance);
    entry.RegisterMember("inventory", &JewelEntry::inventory);
    entry.RegisterMember("alternate", &JewelEntry::alternate);
    entry.RegisterMember("index", &JewelEntry::index);
    entry.RegisterMember("item", &JewelEntry::item);
    entry.RegisterMember("level", &JewelEntry::level);
    c.RegisterArray<std::vector<JewelEntry>>();
    c.Bind("entries", &model.entries);
    c.Bind("status", &model.status);
    c.Bind("search", &model.search);
    c.Bind("pending", &model.pending);
    c.Bind("empty", &model.empty);
    c.Bind("auto_pick", &model.auto_pick);

    c.BindEventCallback("paris_bank_refresh",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ClickRefresh(); });
    c.BindEventCallback("paris_bank_deposit",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            if (args.size() == 2) RequestTransfer(args[0].Get<int>(-1), true, args[1].Get<int>(-1));
        });
    c.BindEventCallback("paris_bank_withdraw",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            if (args.size() == 2) RequestTransfer(args[0].Get<int>(-1), false, args[1].Get<int>(-1));
        });
    c.BindEventCallback("paris_bank_auto_pick",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ClickAutoPick(); });
    c.BindEventCallback("paris_bank_close",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { m_closeRequested = true; });
}

void CParisJewelBankWindow::Release()
{
    Network::Paris::UnregisterTextHandler(m_textHandler);
    m_textHandler = 0;
    {
        std::lock_guard lock(m_receiveMutex);
        m_acceptPackets = false;
        m_pendingTexts.clear();
    }
    m_RmlView.Release();
    m_itemTargets.clear();
    m_displayEntries.clear();
    if (m_manager)
    {
        m_manager->RemoveUIObj(this);
        m_manager = nullptr;
    }
}

void CParisJewelBankWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document())
        return;

    if (show)
        m_RmlView.Document()->Show(Rml::ModalFlag::None, Rml::FocusFlag::Document);
    else
        m_RmlView.Document()->Hide();
}

void CParisJewelBankWindow::OpenWindow()
{
    // Don't probe the document before its owning context/model exists. In particular, opening
    // this window from CUSTOM can happen before the lazy Rml document has been built.
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure())
        return;
    if (m_itemTargets.empty())
    {
        m_itemTargets.reserve(Network::Paris::JewelBank::MaximumEntries);
        for (std::size_t index = 0; index < Network::Paris::JewelBank::MaximumEntries; ++index)
            m_itemTargets.push_back(std::make_unique<UI::Items::ItemCameraTarget>(
                [this, index](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(index, offset, size); }));
    }
    if (g_pNewUISystem)
    {
        if (g_pNewUISystem->IsVisible(INTERFACE_CHARACTER)) g_pNewUISystem->Hide(INTERFACE_CHARACTER);
        if (!g_pNewUISystem->IsVisible(INTERFACE_INVENTORY)) g_pNewUISystem->Show(INTERFACE_INVENTORY);
    }
    m_closeRequested = false;
    Show(true);
    RequestList();
}

void CParisJewelBankWindow::CloseWindow()
{
    m_closeRequested = false;
    Show(false);
}

bool CParisJewelBankWindow::UpdateMouseEvent()
{
    return !IsVisible();
}

bool CParisJewelBankWindow::UpdateKeyEvent()
{
    if (IsVisible() && mu::ui::window::IsPress(VK_ESCAPE))
    {
        CloseWindow();
        return false;
    }
    return !IsVisible();
}

bool CParisJewelBankWindow::Update()
{
    if (m_closeRequested)
    {
        CloseWindow();
        return true;
    }

    std::vector<std::wstring> incoming;
    {
        std::lock_guard lock(m_receiveMutex);
        incoming.swap(m_pendingTexts);
    }
    for (const std::wstring& text : incoming)
        ReceiveText(text);

    if (m_pendingRequest != 0 && GetTickCount() - m_requestStartedAt > 10000)
    {
        m_pendingRequest = 0;
        m_headerReceived = false;
        m_status = "No hubo respuesta del servidor. Pulsa Actualizar.";
        RefreshDisplayedRows();
    }
    return true;
}

bool CParisJewelBankWindow::Render()
{
    SyncModel();
    Rml::ElementDocument* document = m_RmlView.Document();
    if (document)
    {
        Rml::ElementList icons;
        document->QuerySelectorAll(icons, ".bank-item-icon");
        for (std::size_t index = 0; index < m_itemTargets.size(); ++index)
        {
            if (IsVisible() && index < m_displayEntries.size() && index < icons.size())
                m_itemTargets[index]->Sync(icons[index], true);
            else
                m_itemTargets[index]->Disable();
        }
    }
    return true;
}

void CParisJewelBankWindow::DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    if (index >= m_displayEntries.size() || m_displayEntries[index].item < 0)
        return;
    const JewelEntry& item = m_displayEntries[index];
    RenderItem3D(offset.x, offset.y, size.x, size.y, item.item, item.level, 0, 0, false);
}

void CParisJewelBankWindow::SyncModel()
{
    if (!m_RmlView.Document())
        return;
    auto& model = m_RmlView.GetModel();
    if (model.status != m_status)
    {
        model.status = m_status;
        m_RmlView.MarkDirty("status");
    }
    if (model.auto_pick != m_autoPick)
    {
        model.auto_pick = m_autoPick;
        m_RmlView.MarkDirty("auto_pick");
    }
    std::string query = model.search;
    std::transform(query.begin(), query.end(), query.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    m_displayEntries.clear();

    // Keep the complete OpenGL jewel catalogue visible even when an older server only
    // returns rows for jewels that are configured in its bank. Synthetic rows are display
    // only (index == -1); transfers remain enabled solely for real server rows.
    struct DefaultJewel { const char* name; int item; };
    static const DefaultJewel defaults[] = {
        {"Jewel of Bless", ITEM_JEWEL_OF_BLESS},
        {"Jewel of Soul", ITEM_JEWEL_OF_SOUL},
        {"Jewel of Life", ITEM_JEWEL_OF_LIFE},
        {"Jewel of Creation", ITEM_JEWEL_OF_CREATION},
        {"Jewel of Guardian", ITEM_JEWEL_OF_GUARDIAN},
        {"Gemstone", ITEM_GEMSTONE},
        {"Jewel of Harmony", ITEM_JEWEL_OF_HARMONY},
        {"Jewel of Chaos", ITEM_JEWEL_OF_CHAOS},
        {"Lower Stone", ITEM_LOWER_REFINE_STONE},
        {"Higher Stone", ITEM_HIGHER_REFINE_STONE},
    };
    std::vector<JewelEntry> catalogue;
    catalogue.reserve((sizeof(defaults) / sizeof(defaults[0])) + m_entries.size());
    for (const DefaultJewel& jewel : defaults)
    {
        JewelEntry entry;
        entry.name = jewel.name;
        entry.inventory = "0";
        entry.balance = "0";
        entry.index = -1;
        entry.item = jewel.item;
        catalogue.push_back(std::move(entry));
    }
    for (const JewelEntry& serverEntry : m_entries)
    {
        auto known = std::find_if(catalogue.begin(), catalogue.end(), [&](const JewelEntry& entry)
        {
            return entry.item == serverEntry.item;
        });
        if (known != catalogue.end())
        {
            const Rml::String canonicalName = known->name;
            *known = serverEntry;
            known->name = canonicalName;
        }
        else
            catalogue.push_back(serverEntry);
    }

    for (const JewelEntry& sourceEntry : catalogue)
    {
        std::string name = sourceEntry.name;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (query.empty() || name.find(query) != std::string::npos)
        {
            JewelEntry entry = sourceEntry;
            entry.alternate = (m_displayEntries.size() % 2) != 0;
            m_displayEntries.push_back(std::move(entry));
        }
    }
    if (model.entries != m_displayEntries)
    {
        model.entries = m_displayEntries;
        m_RmlView.MarkDirty("entries");
    }
    const bool pending = m_pendingRequest != 0;
    if (model.pending != pending)
    {
        model.pending = pending;
        m_RmlView.MarkDirty("pending");
    }
    const bool empty = m_displayEntries.empty();
    if (model.empty != empty)
    {
        model.empty = empty;
        m_RmlView.MarkDirty("empty");
    }
}

void CParisJewelBankWindow::RequestList()
{
    RequestCommand(L"list", "Consultando el banco de joyas…");
}

void CParisJewelBankWindow::RequestCommand(const std::wstring& operation, const char* status)
{
    if (m_pendingRequest != 0)
        return;
    if (m_nextRequest == 0)
        ++m_nextRequest;

    const std::uint32_t request = m_nextRequest++;
    wchar_t command[96]{};
    swprintf_s(command, L"/parisbank %u %ls", request, operation.c_str());
    if (!Hero || !Network::Paris::RequestJewelBank(Hero->ID, command))
    {
        m_status = "No hay conexión con el servidor.";
        RefreshDisplayedRows();
        return;
    }

    m_pendingRequest = request;
    m_hasSnapshot = false;
    m_requestStartedAt = GetTickCount();
    m_headerReceived = false;
    m_expectedEntries = 0;
    m_receivedEntries.clear();
    if (operation == L"list")
        m_entries.clear();
    m_status = status;
    RefreshDisplayedRows();
}

void CParisJewelBankWindow::RequestTransfer(int index, bool deposit, int amount)
{
    // The original bank protocol uses zero for ALL; the server applies balance,
    // bank capacity and free inventory space limits to the requested quantity.
    constexpr int TransferAll = 0;
    constexpr int TransferOne = 1;
    constexpr int TransferTen = 10;
    if (m_pendingRequest != 0 || (amount != TransferAll && amount != TransferOne && amount != TransferTen))
        return;
    if (index < 0 || index >= static_cast<int>(m_entries.size()))
    {
        m_status = "La joya no está configurada en el servidor.";
        RefreshDisplayedRows();
        return;
    }
    if (!m_hasSnapshot)
    {
        m_status = "Actualiza el banco antes de transferir joyas.";
        RefreshDisplayedRows();
        return;
    }

    wchar_t operation[64]{};
    swprintf_s(operation, L"%ls %d %d", deposit ? L"deposit" : L"withdraw", index, amount);
    RequestCommand(operation, deposit ? "Depositando joyas…" : "Retirando joyas…");
}

bool CParisJewelBankWindow::ReceiveText(std::wstring_view text)
{
    if (!text.starts_with(L"[#PJB"))
        return false;

    if (m_pendingRequest == 0)
        return true;
    Network::Paris::JewelBankMessage message;
    if (!Network::Paris::JewelBank::Decode(text, m_pendingRequest, message))
        return true;

    using Kind = Network::Paris::JewelBankMessage::Kind;
    if (message.kind == Kind::Header)
    {
        m_hasSnapshot = false;
        m_headerReceived = true;
        m_autoPick = message.automatic;
        m_expectedEntries = message.count;
        m_receivedEntries.assign(message.count, false);
        m_entries.assign(message.count, {});
        if (message.status == 0)
            m_status = "Banco actualizado";
        else if (message.status == 3)
            m_status = "El banco de joyas está desactivado.";
        else if (message.status == 4)
            m_status = "Auto Pick no está disponible.";
        else if (message.status == 5)
            m_status = "La joya no está configurada.";
        else
            m_status = "El servidor rechazó la operación.";
    }
    else if (message.kind == Kind::Entry)
    {
        if (!m_headerReceived || message.row < 0 || message.row >= m_expectedEntries)
            return true;
        JewelEntry& entry = m_entries[message.row];
        const std::wstring name(message.name);
        entry.name = StringUtils::WideToNarrow(name.c_str());
        entry.balance = std::to_string(message.balance);
        entry.inventory = std::to_string(message.inventory);
        entry.index = message.row;
        entry.item = message.item;
        entry.level = message.level;
        m_receivedEntries[message.row] = true;
    }
    else
        CompleteSnapshot();

    RefreshDisplayedRows();
    return true;
}

void CParisJewelBankWindow::CompleteSnapshot()
{
    m_hasSnapshot = m_headerReceived && m_receivedEntries.size() == static_cast<std::size_t>(m_expectedEntries)
        && std::all_of(m_receivedEntries.begin(), m_receivedEntries.end(), [](bool value) { return value; });
    m_pendingRequest = 0;
    if (!m_hasSnapshot)
        m_status = "La respuesta del banco llegó incompleta.";
    else if (m_status == "Banco actualizado")
        m_status = "Banco de joyas actualizado";
}

void CParisJewelBankWindow::RefreshDisplayedRows()
{
    if (!m_RmlView.Document())
        return;
    SyncModel();
}

void CParisJewelBankWindow::ClickRefresh()
{
    RequestList();
}

void CParisJewelBankWindow::ClickAutoPick()
{
    RequestCommand(m_autoPick ? L"auto 0" : L"auto 1", "Guardando Auto Pick…");
}

} // namespace mu::ui::window
