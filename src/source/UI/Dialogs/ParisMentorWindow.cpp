#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisMentorWindow.h"
#include "Network/Paris/MentorProtocol.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Inventory/ItemMng.h"
#include "Engine/Object/ZzzInventory.h"
#include <RmlUi/Core/Event.h>
#include <algorithm>
#include <ctime>

namespace mu::ui::window
{
namespace
{
namespace Mentor = Network::Paris::Mentor;
constexpr DWORD InterfaceId = 0x504D4E54u;
constexpr DWORD RefreshMilliseconds = 2000, StaleMilliseconds = 6000, OpeningMilliseconds = 8000;
constexpr int RewardSlots = 6;
Rml::String Utf8(const std::wstring& text) { return StringUtils::WideToNarrow(text.c_str()); }
Rml::String Label(const char* key, const char* fallback)
{
    const auto found = Mentor::Data.Labels.find(key);
    return found == Mentor::Data.Labels.end() || found->second.empty() ? fallback : Utf8(found->second);
}
Rml::String Date(long long stamp)
{
    if (stamp <= 0) return "-";
    const time_t value = static_cast<time_t>(stamp);
    tm local{};
    if (localtime_s(&local, &value) != 0) return "-";
    char text[32]{};
    strftime(text, sizeof(text), "%d/%m/%Y %H:%M", &local);
    return text;
}
int PageCount(int tab)
{
    const auto& data = Mentor::Data;
    const int total = tab == 0 ? data.RelationCount : tab == 1 ? data.MentorCount : tab == 2 ? data.OfferCount : data.HistoryCount;
    const int rows = tab == 0 ? (data.IsMentor ? 7 : 3) : RewardSlots;
    return std::max(1, (total + rows - 1) / rows);
}
Rml::String Portrait(int characterClass)
{
    const int base = characterClass >> 2;
    if (base >= 3 && base <= 5) return "class-" + std::to_string(base);
    const int tile = base == 0 ? (characterClass >= 1 ? 2 : 4) : base == 1 ? (characterClass >= 6 ? 0 : 1) : base == 2 ? 3 : 5;
    return "portrait-" + std::to_string(tile);
}
} // namespace

CParisMentorWindow* g_pParisMentorWindow = nullptr;
CParisMentorWindow::~CParisMentorWindow() { Release(); }
bool CParisMentorWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}
void CParisMentorWindow::Release()
{
    for (auto& target : m_itemTargets) target->Disable();
    m_RmlView.Release();
    m_itemTargets.clear();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisMentorWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    m_modelHandle = c.GetModelHandle();
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name); row.RegisterMember("detail", &Row::detail);
    row.RegisterMember("value", &Row::value); row.RegisterMember("extra", &Row::extra);
    row.RegisterMember("portrait", &Row::portrait); row.RegisterMember("action", &Row::action);
    row.RegisterMember("index", &Row::index); row.RegisterMember("enabled", &Row::enabled);
    c.RegisterArray<std::vector<Row>>();
#define BIND(field) c.Bind(#field, &model.field)
    BIND(role); BIND(assigned); BIND(requests); BIND(canCancel); BIND(mentor); BIND(ready); BIND(confirm); BIND(canAccept); BIND(tab); BIND(page); BIND(pages); BIND(reset);
    BIND(status); BIND(query); BIND(name); BIND(points); BIND(graduated); BIND(daily); BIND(bonus);
    BIND(active); BIND(hours); BIND(progress); BIND(activeWidth); BIND(confirmText);
    BIND(detailName); BIND(detailMentor); BIND(detailState); BIND(detailEvent); BIND(detailDate); BIND(detailPoints);
    BIND(people); BIND(top); BIND(offers); BIND(history); BIND(recent); BIND(details);
#undef BIND
    BindActions(c);
}
void CParisMentorWindow::BindActions(Rml::DataModelConstructor& c)
{
    c.BindEventCallback("mentor_close", [this](auto, auto&, const auto&) { Show(false); });
    c.BindEventCallback("mentor_tab", [this](auto, auto&, const Rml::VariantList& a) { if (!a.empty()) ChangeTab(a[0].Get<int>(0)); });
    c.BindEventCallback("mentor_page", [this](auto, auto&, const Rml::VariantList& a) { if (!a.empty()) ChangePage(a[0].Get<int>(0)); });
    c.BindEventCallback("mentor_search", [this](auto, auto&, const auto&) { m_RmlView.GetModel().page = 1; Send(); });
    c.BindEventCallback("mentor_resets", [this](auto, auto&, const auto&) {
        auto& model = m_RmlView.GetModel(); model.reset = (model.reset + 25) % 125; model.page = 1; Send(); });
    c.BindEventCallback("mentor_filter", [this](auto, auto&, const Rml::VariantList& a) {
        if (a.empty()) return; m_filter = std::clamp(a[0].Get<int>(0), 0, 3); m_RmlView.GetModel().page = 1; Send(); });
    c.BindEventCallback("mentor_history", [this](auto, auto&, const Rml::VariantList& a) { if (!a.empty()) SelectHistory(a[0].Get<int>(-1)); });
    c.BindEventCallback("mentor_person", [this](auto, auto&, const Rml::VariantList& a) { if (!a.empty()) SelectPerson(a[0].Get<int>(-1)); });
    c.BindEventCallback("mentor_accept", [this](auto, auto&, const auto&) {
        const auto& rows = m_RmlView.GetModel().people;
        if (!Current() || rows.empty() || !m_RmlView.GetModel().canAccept) return;
        Send(3, Mentor::Data.Relations[rows.front().index].Id); });
    c.BindEventCallback("mentor_reject", [this](auto, auto&, const auto&) {
        const auto& rows = m_RmlView.GetModel().people;
        if (!Current() || rows.empty() || !m_RmlView.GetModel().canAccept) return;
        Send(4, Mentor::Data.Relations[rows.front().index].Id); });
    c.BindEventCallback("mentor_link_cancel", [this](auto, auto&, const auto&) {
        const auto& data = Mentor::Data;
        if (Current() && !data.LinkId.empty() && !data.LinkAccepted) Send(data.LinkIncoming ? 4 : 5, data.LinkId); });
    c.BindEventCallback("mentor_redeem", [this](auto, auto&, const Rml::VariantList& a) { if (!a.empty()) Redeem(a[0].Get<int>(-1)); });
    c.BindEventCallback("mentor_confirm", [this](auto, auto&, const auto&) { ConfirmReward(); });
    c.BindEventCallback("mentor_cancel", [this](auto, auto&, const auto&) {
        m_confirmation.clear(); m_RmlView.GetModel().confirm = false; m_RmlView.MarkDirty("confirm"); });
}
void CParisMentorWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    if (m_itemTargets.empty())
        for (std::size_t i = 0; i < RewardSlots; ++i)
            m_itemTargets.push_back(std::make_unique<UI::Items::ItemCameraTarget>(
                [this, i](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(i, offset, size); }));
    auto& model = m_RmlView.GetModel();
    model.tab = Mentor::Data.IsMentor ? 0 : 1; model.page = 1; model.ready = false; model.confirm = false;
    model.status = "Conectando con Mentoría..."; model.query.clear(); model.reset = 0;
    m_confirmation.clear(); m_filter = 0; m_seen = Mentor::Received; m_opened = timeGetTime(); m_waiting = false;
    Show(true); Send();
    m_modelHandle.DirtyAllVariables();
}
void CParisMentorWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
    {
        m_confirmation.clear();
        m_RmlView.Document()->Hide();
        for (auto& target : m_itemTargets) target->Disable();
    }
}
bool CParisMentorWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { Show(false); return false; }
    return !IsVisible();
}
bool CParisMentorWindow::Current() const
{
    const auto& model = m_RmlView.GetModel();
    return !m_waiting && Mentor::Received != 0 && timeGetTime() - Mentor::Received <= StaleMilliseconds
        && Mentor::Data.Enabled && Mentor::Data.Tab == model.tab && Mentor::Data.Page == model.page - 1;
}
void CParisMentorWindow::Send(int action, const std::string& identity, const std::wstring& target)
{
    auto& model = m_RmlView.GetModel();
    if (action != 0 && !Current()) return;
    std::wstring filter = target;
    if (action == 0 && model.tab == 1)
        filter = StringUtils::NarrowToWide(model.query) + L"|" + std::to_wstring(Mentor::Data.Minimum + model.reset);
    if (action == 0 && model.tab == 3)
    {
        constexpr const wchar_t* Filters[]{L"all", L"completed", L"ended", L"rewards"};
        filter = Filters[m_filter];
    }
    Mentor::Request(static_cast<BYTE>(action), static_cast<BYTE>(model.tab), static_cast<BYTE>(model.page - 1), identity, filter.c_str());
    m_lastRequest = timeGetTime();
    if (action != 0) m_waiting = true;
}
void CParisMentorWindow::ChangeTab(int tab)
{
    auto& model = m_RmlView.GetModel();
    if (tab < 0 || tab > 3 || (model.mentor ? tab == 1 : tab == 0 || tab == 2)) return;
    if (tab == model.tab) return;
    m_opened = timeGetTime(); m_seen = 0;
    model.status = "Actualizando Mentoría…";
    model.tab = tab; model.page = 1; model.ready = false; m_selected = 0; m_confirmation.clear(); model.confirm = false;
    Send(); m_modelHandle.DirtyAllVariables();
}
void CParisMentorWindow::ChangePage(int direction)
{
    auto& model = m_RmlView.GetModel();
    const int page = model.page + direction;
    if (page < 1 || page > model.pages) return;
    m_opened = timeGetTime(); m_seen = 0;
    model.status = "Actualizando Mentoría…";
    model.page = page; model.ready = false; m_selected = 0; Send(); m_modelHandle.DirtyAllVariables();
}
bool CParisMentorWindow::Update()
{
    if (!IsVisible()) return true;
    SyncModel();
    if (timeGetTime() - m_lastRequest >= RefreshMilliseconds && !m_RmlView.GetModel().confirm) Send();
    return true;
}
void CParisMentorWindow::SyncModel()
{
    auto& model = m_RmlView.GetModel();
    if (!model.ready && timeGetTime() - m_opened > OpeningMilliseconds)
    { model.status = "Esperando respuesta de Mentoría; reintentando…"; m_RmlView.MarkDirty("status"); }
    if (m_seen == Mentor::Received || Mentor::Received == 0) return;
    m_seen = Mentor::Received;
    const auto& data = Mentor::Data;
    if (data.Tab != model.tab || data.Page != model.page - 1) return;
    m_waiting = false;
    model.mentor = data.IsMentor;
    if (model.mentor ? model.tab == 1 : model.tab == 0 || model.tab == 2) { ChangeTab(model.mentor ? 0 : 1); return; }
    model.role = data.IsMentor ? "Mentor" : "Novato";
    model.assigned = data.AssignedMentor.empty() ? "Sin asignar" : Utf8(data.AssignedMentor);
    model.requests = std::to_string(data.Requests); model.canCancel = !data.LinkId.empty() && !data.LinkAccepted;
    model.ready = true; model.name = Utf8(data.Name); model.points = std::to_string(data.Points);
    model.graduated = std::to_string(data.Graduated); model.daily = std::to_string(data.Daily) + " / " + std::to_string(data.DailyLimit);
    model.bonus = std::to_string(data.Bonus); model.pages = PageCount(model.tab);
    model.active = std::to_string(data.ActiveNovices) + " / " + std::to_string(data.MaximumNovices);
    model.activeWidth = std::to_string(std::clamp(100 * data.ActiveNovices / std::max(1, data.MaximumNovices), 0, 100)) + "%";
    model.hours = std::to_string(static_cast<int>(data.HelpSeconds / 3600)); model.progress = std::to_string(static_cast<int>(data.NoviceProgress));
    model.status = Utf8(data.Message);
    if (!m_confirmation.empty() && m_confirmToken != data.Token) { m_confirmation.clear(); model.confirm = false; }
    SyncPeople(); SyncOffers(); SyncHistory();
    model.recent.clear();
    for (const auto& entry : data.Recent)
        model.recent.push_back({Utf8(entry.Kind + L" - " + entry.Name), Date(entry.Time)});
    m_modelHandle.DirtyAllVariables();
}
void CParisMentorWindow::SyncPeople()
{
    auto& model = m_RmlView.GetModel();
    const auto& data = Mentor::Data;
    model.people.clear(); model.top.clear(); model.canAccept = false;
    const auto& source = model.tab == 1 ? data.Mentors : data.Relations;
    for (int i = 0; i < static_cast<int>(source.size()); ++i)
    {
        const auto& person = source[i];
        Row row{Utf8(person.Name), Utf8(person.ClassName) + " - " + std::to_string(person.Level),
            std::to_string(person.Resets) + " / " + std::to_string(person.Goal), Utf8(person.Status), Portrait(person.Class), "", i};
        row.enabled = data.Enabled && ((!person.Id.empty() && !person.Accepted) || (data.CanRequest && person.Novices < person.Maximum));
        row.action = !person.Id.empty() ? person.Accepted ? Utf8(person.Status) : Label(data.LinkIncoming ? "MentorAccept" : "MentorCancel", data.LinkIncoming ? "Aceptar" : "Cancelar") : Label("MentorRequest", "Solicitar");
        model.people.push_back(std::move(row));
    }
    if (model.tab == 0 && !model.people.empty())
    {
        m_selected = std::clamp(m_selected, 0, static_cast<int>(model.people.size()) - 1);
        std::rotate(model.people.begin(), model.people.begin() + m_selected, model.people.begin() + m_selected + 1);
        const auto& person = source[model.people.front().index];
        model.canAccept = data.Enabled && person.Incoming && !person.Accepted && !person.Graduated;
    }
    for (const auto& person : data.Top)
        model.top.push_back({Utf8(person.Name), Utf8(person.ClassName), std::to_string(person.GraduatedCount), Utf8(person.GuildName), Portrait(person.Class)});
}
void CParisMentorWindow::SelectPerson(int index)
{
    const auto& data = Mentor::Data;
    auto& model = m_RmlView.GetModel();
    if (!Current()) return;
    if (model.tab == 0)
    {
        if (index < 0 || index >= static_cast<int>(data.Relations.size())) return;
        m_selected = index; SyncPeople(); m_modelHandle.DirtyAllVariables(); return;
    }
    if (index < 0 || index >= static_cast<int>(data.Mentors.size())) return;
    const auto& person = data.Mentors[index];
    if (!person.Id.empty() && !person.Accepted) Send(data.LinkIncoming ? 3 : 5, person.Id);
    else if (data.CanRequest && person.Novices < person.Maximum) Send(1, {}, person.Name);
}
void CParisMentorWindow::SyncOffers()
{
    auto& rows = m_RmlView.GetModel().offers; rows.clear();
    const auto& data = Mentor::Data;
    for (int i = 0; i < static_cast<int>(data.Offers.size()); ++i)
    {
        const auto& offer = data.Offers[i];
        Rml::String summary;
        for (const auto& reward : offer.Rewards)
        {
            if (!summary.empty()) summary += " + ";
            Rml::String name = reward.Type == 3 ? "WC" : reward.Type == 4 ? "WP" : reward.Type == 5 ? "GP" : "Zen";
            if (reward.Type == 6 && reward.Group >= 0 && reward.Group < 16 && reward.Number >= 0 && reward.Number < MAX_ITEM_INDEX)
                name = StringUtils::WideToNarrow(ItemAttribute[reward.Group * MAX_ITEM_INDEX + reward.Number].Name);
            summary += std::to_string(reward.Amount) + " " + name;
        }
        const bool enabled = data.Enabled && data.IsMentor && !data.Pending && data.Points >= offer.Cost;
        rows.push_back({Utf8(offer.Name), summary, std::to_string(offer.Cost), "", "",
            enabled ? Label("MentorRedeem", "Canjear") : data.Pending ? "Entrega pendiente" : "Faltan " + std::to_string(std::max(0, offer.Cost - data.Points)) + " puntos", i, enabled});
    }
}
void CParisMentorWindow::Redeem(int index)
{
    const auto& data = Mentor::Data;
    if (!Current() || !data.IsMentor || data.Pending || index < 0 || index >= static_cast<int>(data.Offers.size())) return;
    const auto& offer = data.Offers[index];
    if (data.Points < offer.Cost) return;
    m_confirmation = offer.Id; m_confirmCost = offer.Cost; m_confirmToken = data.Token;
    auto& model = m_RmlView.GetModel(); model.confirm = true;
    model.confirmText = Utf8(offer.Name) + " - Costo: " + std::to_string(offer.Cost) + " / Puntos: " + std::to_string(data.Points);
    m_modelHandle.DirtyAllVariables();
}
void CParisMentorWindow::ConfirmReward()
{
    if (m_confirmation.empty() || !Current()) return;
    const auto& data = Mentor::Data;
    const auto found = std::find_if(data.Offers.begin(), data.Offers.end(), [this](const auto& offer) { return offer.Id == m_confirmation && offer.Cost == m_confirmCost; });
    if (data.Token == m_confirmToken && data.IsMentor && !data.Pending && found != data.Offers.end() && data.Points >= found->Cost) Send(6, m_confirmation);
    m_confirmation.clear(); m_RmlView.GetModel().confirm = false; m_RmlView.MarkDirty("confirm");
}
void CParisMentorWindow::SyncHistory()
{
    auto& model = m_RmlView.GetModel(); model.history.clear();
    const auto& data = Mentor::Data;
    for (int i = 0; i < static_cast<int>(data.History.size()); ++i)
    {
        const auto& entry = data.History[i];
        model.history.push_back({Utf8(entry.Name), Utf8(entry.Kind), std::to_string(entry.Points), Date(entry.Time), "", "", i});
    }
    SelectHistory(m_selected);
}
void CParisMentorWindow::SelectHistory(int index)
{
    auto& model = m_RmlView.GetModel(); const auto& rows = Mentor::Data.History;
    model.details.clear(); model.detailName.clear(); model.detailMentor.clear(); model.detailState.clear();
    model.detailEvent.clear(); model.detailDate.clear(); model.detailPoints.clear();
    if (rows.empty()) return;
    m_selected = std::clamp(index, 0, static_cast<int>(rows.size()) - 1);
    const auto& entry = rows[m_selected];
    model.detailName = Utf8(entry.HasDetail ? entry.StudentName : entry.Name);
    model.detailMentor = Utf8(entry.HasDetail ? entry.MentorName : entry.Kind); model.detailState = Utf8(entry.State);
    model.detailEvent = Utf8(entry.Kind); model.detailDate = Date(entry.Time); model.detailPoints = std::to_string(entry.Points);
    if (entry.HasDetail)
    {
        model.details.push_back({"Inicio", Date(entry.Created)}); model.details.push_back({"Fin", entry.Ended ? Date(entry.Ended) : "En curso"});
        model.details.push_back({"Tiempo valido", std::to_string(entry.Seconds / 3600) + " h " + std::to_string(entry.Seconds % 3600 / 60) + " min"});
        model.details.push_back({"Horas en party", std::to_string(entry.Seconds / 3600)}); model.details.push_back({"Puntos obtenidos", std::to_string(entry.Awarded)});
    }
    m_modelHandle.DirtyAllVariables();
}
bool CParisMentorWindow::Render()
{
    if (!IsVisible() || !m_RmlView.Document()) return true;
    Rml::ElementList icons; m_RmlView.Document()->QuerySelectorAll(icons, ".mentor-reward-item");
    const auto& model = m_RmlView.GetModel();
    for (std::size_t i = 0; i < m_itemTargets.size(); ++i)
        m_itemTargets[i]->Sync(i < icons.size() ? icons[i] : nullptr, model.ready && model.tab == 2 && !model.confirm && i < Mentor::Data.Offers.size());
    return true;
}
void CParisMentorWindow::DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    const auto& offers = Mentor::Data.Offers;
    if (index >= offers.size()) return;
    for (const auto& reward : offers[index].Rewards)
    {
        if (reward.Type != 6 || reward.Group < 0 || reward.Group >= 16 || reward.Number < 0 || reward.Number >= MAX_ITEM_INDEX) continue;
        RenderItem3D(offset.x, offset.y, size.x, size.y, reward.Group * MAX_ITEM_INDEX + reward.Number, reward.Level, reward.Excellent, reward.Ancient, false);
        break;
    }
}
} // namespace mu::ui::window
