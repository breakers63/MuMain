#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisBattlePassWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "GameLogic/Social/BattlePassRewardState.h"
#include "Network/Paris/ParisBattlePassProtocol.h"
#include "Network/Paris/ParisRequests.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include "UI/Inventory/ItemMng.h"
#include "Engine/Object/ZzzInventory.h"
#include <ctime>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50425053u;
constexpr DWORD SnapshotFreshness = 30000, RequestTimeout = 6000;
constexpr int VisibleLevels = 6, RewardTargets = VisibleLevels * 2;
}

CParisBattlePassWindow* g_pParisBattlePassWindow = nullptr;

CParisBattlePassWindow::~CParisBattlePassWindow() { Release(); }

bool CParisBattlePassWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisBattlePassWindow::Release()
{
    for (auto& target : m_itemTargets) target->Disable();
    m_RmlView.Release();
    m_itemTargets.clear();
    if (!m_manager) return;
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CParisBattlePassWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<RewardRow>();
    row.RegisterMember("label", &RewardRow::label);
    row.RegisterMember("amount", &RewardRow::amount);
    row.RegisterMember("level", &RewardRow::level);
    row.RegisterMember("column", &RewardRow::column);
    row.RegisterMember("premium", &RewardRow::premium);
    row.RegisterMember("claimed", &RewardRow::claimed);
    row.RegisterMember("locked", &RewardRow::locked);
    row.RegisterMember("zen", &RewardRow::zen);
    row.RegisterMember("coins", &RewardRow::coins);
    row.RegisterMember("ruud", &RewardRow::ruud);
    c.RegisterArray<std::vector<RewardRow>>();
    c.Bind("status", &model.status);
    c.Bind("message", &model.message);
    c.Bind("rewards", &model.rewards);
    c.Bind("level", &model.level);
    c.Bind("max_level", &model.max_level);
    c.Bind("points", &model.points);
    c.Bind("required_points", &model.required_points);
    c.Bind("progress", &model.progress);
    c.Bind("visible_fraction", &model.visible_fraction);
    c.Bind("page", &model.page);
    c.Bind("max_page", &model.max_page);
    c.Bind("active", &model.active);
    c.Bind("pending", &model.pending);
    c.Bind("premium_dialog", &model.premium_dialog);
    c.Bind("requirement", &model.requirement);
    c.Bind("premium_status", &model.premium_status);
    c.Bind("expires", &model.expires);
    BindEvents(c);
}

void CParisBattlePassWindow::BindEvents(Rml::DataModelConstructor& c)
{
    c.BindEventCallback("paris_bp_claim", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (args.size() >= 2) Claim(args[0].Get<int>(0), args[1].Get<bool>(false));
    });
    c.BindEventCallback("paris_bp_premium", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        m_RmlView.GetModel().premium_dialog = true;
        UpdatePremium();
        m_RmlView.MarkDirty("premium_dialog");
    });
    c.BindEventCallback("paris_bp_activate", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        const auto& data = Network::Paris::BattlePass::Data;
        if (m_pending) return;
        if (m_hasSnapshot && data.Active && !data.Premium) SendRequest(2);
        else
        {
            m_RmlView.GetModel().premium_dialog = false;
            m_RmlView.MarkDirty("premium_dialog");
        }
    });
    c.BindEventCallback("paris_bp_cancel", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        if (m_pending && m_operation == 2) return;
        m_RmlView.GetModel().premium_dialog = false;
        m_RmlView.MarkDirty("premium_dialog");
    });
    c.BindEventCallback("paris_bp_scroll", [this](Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList&)
    {
        auto& model = m_RmlView.GetModel();
        if (model.premium_dialog) return;
        const float delta = event.GetParameter<float>("wheel_delta_y", 0.f);
        if (delta == 0.f) return;
        model.page = std::clamp(model.page + (delta > 0.f ? 1 : -1), 0, model.max_page);
        m_RmlView.MarkDirty("page");
        event.StopPropagation();
    });
    c.BindEventCallback("paris_bp_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisBattlePassWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    if (m_itemTargets.empty())
        for (std::size_t index = 0; index < RewardTargets + 1; ++index)
            m_itemTargets.push_back(std::make_unique<UI::Items::ItemCameraTarget>(
                [this, index](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(index, offset, size); }));
    m_hasSnapshot = false;
    m_lastReceived = Network::Paris::BattlePass::Received;
    m_pending = false;
    m_lastPage = -1;
    auto& model = m_RmlView.GetModel();
    model.page = 0;
    model.message.clear();
    m_RmlView.MarkDirty("message");
    model.active = false;
    model.premium_dialog = false;
    m_RmlView.MarkDirty("active");
    m_RmlView.MarkDirty("premium_dialog");
    Show(true);
    RequestSnapshot();
}

void CParisBattlePassWindow::CloseWindow() { Show(false); }

void CParisBattlePassWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
    {
        m_RmlView.Document()->Hide();
        for (auto& target : m_itemTargets) target->Disable();
    }
}

bool CParisBattlePassWindow::UpdateMouseEvent() { return !IsVisible(); }

bool CParisBattlePassWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}

bool CParisBattlePassWindow::Update() { return true; }
bool CParisBattlePassWindow::Render()
{
    if (!IsVisible()) return true;
    SyncModel();
    auto* document = m_RmlView.Document();
    if (!document) return true;
    Rml::ElementList icons;
    document->QuerySelectorAll(icons, ".bp-item-icon");
    const bool dialog = m_RmlView.GetModel().premium_dialog;
    for (std::size_t index = 0; index < RewardTargets && index < m_itemTargets.size(); ++index)
        m_itemTargets[index]->Sync(index < icons.size() ? icons[index] : nullptr,
            !dialog && index < m_previews.size() && m_previews[index].type >= 0);
    if (m_itemTargets.size() > RewardTargets)
        m_itemTargets[RewardTargets]->Sync(document->GetElementById("premium-item"), dialog && m_hasSnapshot && Network::Paris::BattlePass::Data.Active
            && !Network::Paris::BattlePass::Data.PremiumFree);
    return true;
}

void CParisBattlePassWindow::DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    ItemPreview item;
    if (index == RewardTargets)
    {
        const auto& data = Network::Paris::BattlePass::Data;
        item.type = data.PremiumItem;
        item.level = data.PremiumItemLevel;
    }
    else if (index < m_previews.size()) item = m_previews[index];
    if (item.type >= 0 && item.type < MAX_ITEM)
        RenderItem3D(offset.x, offset.y, size.x, size.y, item.type, item.level, item.excellent, item.ancient, false);
}

bool CParisBattlePassWindow::SendRequest(unsigned char operation, int level, bool premium)
{
    if (m_pending) return false;
    if (operation != 0 && (!m_hasSnapshot || timeGetTime() - m_lastReceived >= SnapshotFreshness))
    {
        RequestSnapshot();
        return false;
    }
    m_receivedBeforeRequest = Network::Paris::BattlePass::Received;
    if (!Network::Paris::RequestBattlePass(operation, level, premium)) return false;
    m_pending = true;
    m_operation = operation;
    m_requestedAt = timeGetTime();
    m_RmlView.GetModel().pending = true;
    m_RmlView.MarkDirty("pending");
    UpdatePremium();
    return true;
}

void CParisBattlePassWindow::RequestSnapshot()
{
    auto& model = m_RmlView.GetModel();
    model.status = "Consultando temporada...";
    if (!SendRequest(0)) model.status = "No se pudo consultar el servidor";
    m_RmlView.MarkDirty("status");
}

void CParisBattlePassWindow::Claim(int level, bool premium)
{
    const auto& data = Network::Paris::BattlePass::Data;
    if (!m_hasSnapshot || !data.Active || m_RmlView.GetModel().premium_dialog) return;
    const auto reward = std::find_if(data.Rewards.begin(), data.Rewards.end(), [=](const auto& item)
        { return item.Level == level && item.Premium == premium; });
    if (reward == data.Rewards.end() || reward->Claimed || !GameLogic::Social::BattlePassRewardState::Unlocked(
        data.Active, data.Level, level, premium, data.Premium, reward->Claimed)) return;
    SendRequest(1, level, premium);
}

void CParisBattlePassWindow::BuildRewards()
{
    auto& model = m_RmlView.GetModel();
    const auto& data = Network::Paris::BattlePass::Data;
    model.page = std::clamp(model.page, 0, model.max_page);
    m_lastPage = model.page;
    model.rewards.clear();
    m_previews.clear();
    if (m_hasSnapshot && data.Active)
        for (int column = 0; column < VisibleLevels && model.page + column < static_cast<int>(m_levels.size()); ++column)
            for (bool premium : {false, true})
            {
                RewardRow row;
                row.level = m_levels[model.page + column];
                row.column = column;
                row.premium = premium;
                ItemPreview preview;
                const auto found = std::find_if(data.Rewards.begin(), data.Rewards.end(), [&](const auto& reward)
                    { return reward.Level == row.level && reward.Premium == premium; });
                if (found != data.Rewards.end())
                {
                    row.label = StringUtils::WideToNarrow(found->Name.c_str());
                    row.claimed = found->Claimed;
                    row.locked = !GameLogic::Social::BattlePassRewardState::Unlocked(
                        data.Active, data.Level, row.level, premium, data.Premium, found->Claimed);
                    row.zen = found->Type == 2;
                    row.coins = found->Type == 3 || found->Type == 4;
                    row.ruud = found->Type == 5;
                    if (found->Type == 6)
                    {
                        if (found->Amount > 1) row.amount = "X" + std::to_string(found->Amount);
                        if (!found->Data.empty())
                        {
                            const auto item = ParseItemData(found->Data);
                            preview = {item.Group * 512 + item.Number, item.Level, item.ExcellentFlags, item.AncientDiscriminator};
                        }
                    }
                }
                model.rewards.push_back(std::move(row));
                m_previews.push_back(preview);
            }
    m_RmlView.MarkDirty("page");
    m_RmlView.MarkDirty("rewards");
}

void CParisBattlePassWindow::UpdatePremium()
{
    auto& model = m_RmlView.GetModel();
    const auto& data = Network::Paris::BattlePass::Data;
    model.requirement = "Requisito no publicado por el servidor";
    if (m_pending && m_operation == 0) model.requirement = "Consultando requisitos...";
    else if (m_hasSnapshot && data.PremiumFree) model.requirement = "Sin item de requerimiento (0)";
    else if (m_hasSnapshot && data.PremiumItem >= 0 && data.PremiumItem < MAX_ITEM)
    {
        model.requirement = StringUtils::WideToNarrow(ItemAttribute[data.PremiumItem].Name);
        if (data.PremiumItemLevel > 0) model.requirement += " +" + std::to_string(data.PremiumItemLevel);
        model.requirement += " x" + std::to_string(data.PremiumCount);
    }
    model.premium_status = m_pending ? (m_operation == 2 ? "Activando..." : "Consultando servidor...")
        : !m_hasSnapshot || !data.Active ? "Sin temporada activa" : data.Premium ? "Premium ya activo"
        : !data.PremiumTermsKnown ? "Confirmar activacion"
        : data.PremiumRequirementReady ? "Ready to Activate!" : "Falta el requisito";
    model.expires = "No publicado";
    __time64_t stamp = m_hasSnapshot ? data.PremiumExpiresUtc : 0;
    tm utc{};
    if (stamp > 0 && _gmtime64_s(&utc, &stamp) == 0)
    {
        char date[32]{};
        strftime(date, sizeof(date), "%d/%m/%Y", &utc);
        model.expires = date;
    }
    for (const char* field : {"requirement", "premium_status", "expires"}) m_RmlView.MarkDirty(field);
}

void CParisBattlePassWindow::ApplySnapshot()
{
    auto& model = m_RmlView.GetModel();
    const auto& data = Network::Paris::BattlePass::Data;
    model.active = data.Active;
    model.pending = false;
    model.level = data.Level;
    model.max_level = data.MaxLevel;
    model.points = data.Points;
    model.required_points = data.RequiredPoints;
    model.progress = data.RequiredPoints > 0 ? std::clamp(100.f * data.Points / data.RequiredPoints, 0.f, 100.f) : 0.f;
    model.message = StringUtils::WideToNarrow(data.Message.c_str());
    model.status = data.Active ? "" : "No hay una temporada activa.";
    m_levels.clear();
    for (const auto& reward : data.Rewards) m_levels.push_back(reward.Level);
    std::sort(m_levels.begin(), m_levels.end());
    m_levels.erase(std::unique(m_levels.begin(), m_levels.end()), m_levels.end());
    model.max_page = std::max(0, static_cast<int>(m_levels.size()) - VisibleLevels);
    model.visible_fraction = float(VisibleLevels) / std::max(VisibleLevels, static_cast<int>(m_levels.size()));
    for (const char* field : {"active", "pending", "level", "max_level", "points", "required_points",
        "progress", "message", "status", "max_page", "visible_fraction"}) m_RmlView.MarkDirty(field);
    BuildRewards();
    UpdatePremium();
}

void CParisBattlePassWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    auto& model = m_RmlView.GetModel();
    const DWORD received = Network::Paris::BattlePass::Received;
    if (received != 0 && received != m_lastReceived && (!m_pending || received != m_receivedBeforeRequest))
    {
        m_lastReceived = received;
        m_hasSnapshot = true;
        m_pending = false;
        ApplySnapshot();
    }
    if (m_pending && timeGetTime() - m_requestedAt > RequestTimeout)
    {
        m_pending = model.pending = false;
        model.status = model.message = "Sin respuesta. Cierra y vuelve a abrir.";
        for (const char* field : {"pending", "status", "message"}) m_RmlView.MarkDirty(field);
        UpdatePremium();
    }
    if (model.page != m_lastPage) BuildRewards();
}
} // namespace mu::ui::window
