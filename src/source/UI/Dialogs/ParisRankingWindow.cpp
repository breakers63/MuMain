#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisRankingWindow.h"
#include "Core/Utilities/StringUtils.h"
#include "Network/Paris/CustomRankingProtocol.h"
#include "Network/Paris/RankingInspectionProtocol.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/ItemMng.h"
#include "UI/Social/PhotoViewer.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/Tooltip/LegacyTextListTooltip.h"
#include "Render/Items/ItemPreviewFit.h"
#include "Render/Models/MonsterPreviewTarget.h"
#include "Engine/Object/ZzzInventory.h"
#include "Character/CharacterManager.h"
#include <RmlUi/Core/Event.h>
#include <algorithm>
#include <cwctype>
#include <cmath>
#include <cstdio>

namespace mu::ui::window
{
namespace
{
namespace Ranking = Network::Paris::CustomRanking;
namespace Inspection = Network::Paris::RankingInspection;
constexpr DWORD InterfaceId = 0x5052414Eu;
constexpr int PageSize = 8, MemberPageSize = 6;
constexpr std::array<int, 12> CardSlots{2, 3, 4, 5, 6, 0, 1, 7, 9, 10, 11, 8};
constexpr const char* SlotNames[]{"ARMA", "ESCUDO", "CASCO", "ARMADURA", "PANTALÓN", "GUANTES", "BOTAS", "ALAS", "MASCOTA", "AMULETO", "ANILLO", "ANILLO"};
Rml::String Text(const std::wstring& value) { return StringUtils::WideToNarrow(value.c_str()); }
std::wstring Lower(std::wstring value) { for (auto& c : value) c = static_cast<wchar_t>(std::towlower(c)); return value; }
int BaseClass(int serverClass)
{
    return static_cast<int>(gCharacterManager.GetBaseClass(gCharacterManager.ChangeServerClassTypeToClientClassType(static_cast<SERVER_CLASS_TYPE>(serverClass))));
}
Rml::String ClassName(int serverClass)
{
    const auto type = gCharacterManager.ChangeServerClassTypeToClientClassType(static_cast<SERVER_CLASS_TYPE>(serverClass));
    return Text(gCharacterManager.GetCharacterClassText(type));
}
}
CParisRankingWindow* g_pParisRankingWindow = nullptr;
CParisRankingWindow::CParisRankingWindow() = default;
CParisRankingWindow::~CParisRankingWindow() { Release(); }
bool CParisRankingWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager; manager->AddUIObj(InterfaceId, this); CObject::Show(false); return true;
}
void CParisRankingWindow::ClearItems()
{
    if (m_tooltipShown) UI::Tooltip::HideLegacyTextList();
    m_tooltipShown = false;
    for (auto*& item : m_items) { if (item && g_pNewItemMng) g_pNewItemMng->DeleteItem(item); item = nullptr; }
}
void CParisRankingWindow::Release()
{
    ClearItems();
    for (auto& target : m_targets) if (target) target->Disable();
    for (auto& monster : m_monsters) monster.reset();
    if (m_character)
    {
        auto* bones = m_character->GetPhotoChar()->Object.BoneTransform;
        m_character.reset();
        delete[] bones;
    }
    m_RmlView.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisRankingWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    m_handle = c.GetModelHandle();
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("rank", &Row::rank); row.RegisterMember("name", &Row::name); row.RegisterMember("detail", &Row::detail);
    row.RegisterMember("value", &Row::value); row.RegisterMember("extra", &Row::extra); row.RegisterMember("index", &Row::index);
    row.RegisterMember("slot", &Row::slot); row.RegisterMember("character_class", &Row::characterClass);
    c.RegisterArray<std::vector<Row>>();
    auto equipment = c.RegisterStruct<Equipment>(); equipment.RegisterMember("slot", &Equipment::slot);
    equipment.RegisterMember("label", &Equipment::label); equipment.RegisterMember("name", &Equipment::name); equipment.RegisterMember("present", &Equipment::present);
    c.RegisterArray<std::vector<Equipment>>();
    auto mark = c.RegisterStruct<MarkCell>(); mark.RegisterMember("color", &MarkCell::color); c.RegisterArray<std::vector<MarkCell>>();
#define BIND(field) c.Bind(#field, &model.field)
    BIND(title); BIND(status); BIND(query); BIND(name); BIND(guild); BIND(characterClass); BIND(level); BIND(resets); BIND(totalStats); BIND(visibility);
    BIND(memberName); BIND(memberClass); BIND(memberRank); BIND(memberLevel); BIND(memberResets); BIND(memberStats); BIND(memberOnline);
    BIND(guildScore); BIND(guildMembers); BIND(strength); BIND(agility); BIND(vitality); BIND(energy);
    BIND(missionName); BIND(objective); BIND(reward); BIND(progress); BIND(progressWidth);
    BIND(rows); BIND(members); BIND(missions); BIND(missionRanks); BIND(equipment); BIND(mark);
    BIND(tab); BIND(page); BIND(page_count); BIND(selected); BIND(memberPage); BIND(memberPages); BIND(missionSelected);
    BIND(empty); BIND(hasProfile); BIND(publicEquipment); BIND(ownPublic); BIND(preferenceReady);
#undef BIND
    BindActions(c);
}
void CParisRankingWindow::BindActions(Rml::DataModelConstructor& c)
{
    c.BindEventCallback("paris_ranking_tab", [this](auto, auto&, const Rml::VariantList& args) { if (!args.empty()) RequestTab(args[0].Get<int>(-1)); });
    c.BindEventCallback("paris_ranking_previous", [this](auto, auto&, const auto&) { ChangePage(-1); });
    c.BindEventCallback("paris_ranking_next", [this](auto, auto&, const auto&) { ChangePage(1); });
    c.BindEventCallback("paris_ranking_close", [this](auto, auto&, const auto&) { CloseWindow(); });
    c.BindEventCallback("ranking_select", [this](auto, auto&, const Rml::VariantList& args) { if (!args.empty()) Select(args[0].Get<int>(-1)); });
    c.BindEventCallback("ranking_member", [this](auto, auto&, const Rml::VariantList& args) { if (!args.empty()) SelectMember(args[0].Get<int>(-1)); });
    c.BindEventCallback("ranking_member_page", [this](auto, auto&, const Rml::VariantList& args)
    {
        if (args.empty()) return;
        const int pages = std::max(1, (static_cast<int>(Inspection::Data.members.size()) + MemberPageSize - 1) / MemberPageSize);
        m_memberPage = std::clamp(m_memberPage + args[0].Get<int>(), 0, pages - 1); SyncDetails();
    });
    c.BindEventCallback("ranking_mission", [this](auto, auto&, const Rml::VariantList& args) { if (!args.empty()) SelectMission(args[0].Get<int>(-1)); });
    c.BindEventCallback("ranking_privacy", [this](auto, auto&, const auto&)
    {
        const auto& data = Inspection::Data;
        if (data.preferenceReady) Inspection::RequestEquipment({}, 2, !data.ownPublic);
    });
}
void CParisRankingWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
    {
        m_RmlView.Document()->Hide();
        if (m_tooltipShown) UI::Tooltip::HideLegacyTextList(); m_tooltipShown = false;
        for (auto& target : m_targets) if (target) target->Disable();
        if (m_character) m_character->Target().SetEnabled(false);
        for (auto& monster : m_monsters) if (monster) monster->Disable();
    }
}
void CParisRankingWindow::OpenWindow()
{
    if (!m_RmlView.Ensure()) return;
    for (int i = 0; i < 12; ++i) if (!m_targets[i]) m_targets[i] = std::make_unique<UI::Items::ItemCameraTarget>(
        [this, i](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(i, offset, size); });
    for (auto& target : m_targets) target->SetContinuous(false);
    Show(true); RequestTab(0); Inspection::RequestEquipment({}, 1);
}
void CParisRankingWindow::CloseWindow() { Show(false); }
bool CParisRankingWindow::UpdateMouseEvent() { return !IsVisible(); }
bool CParisRankingWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}
bool CParisRankingWindow::Update() { if (IsVisible()) SyncModel(); return true; }
bool CParisRankingWindow::Render() { if (IsVisible()) { SyncModel(); UpdatePreviews(); } return true; }
void CParisRankingWindow::RequestTab(int tab)
{
    if (tab < 0 || tab > 2) return;
    m_tab = tab; m_page = m_memberPage = m_selectedMember = 0; m_rowsDirty = true;
    auto& model = m_RmlView.GetModel(); model.tab = tab; model.selected = -1; model.query.clear();
    model.hasProfile = false; model.rows.clear(); model.missions.clear(); model.status = "Consultando el ranking…";
    m_handle.DirtyAllVariables();
    if (tab == 2) Inspection::RequestCatalog();
    Ranking::Request(static_cast<Ranking::Tab>(tab));
}
void CParisRankingWindow::ChangePage(int delta)
{
    auto& model = m_RmlView.GetModel();
    m_page = std::clamp(m_page + delta, 0, std::max(0, model.page_count - 1)); m_rowsDirty = true;
}
void CParisRankingWindow::Select(int index)
{
    if (index < 0 || index >= static_cast<int>(Ranking::Data.entries.size())) return;
    m_RmlView.GetModel().selected = index; m_RmlView.MarkDirty("selected");
    m_memberPage = m_selectedMember = 0;
    Inspection::RequestProfile(m_tab, Ranking::Data.entries[index].name);
}
void CParisRankingWindow::SelectMember(int index)
{
    if (index < 0 || index >= static_cast<int>(Inspection::Data.members.size())) return;
    m_selectedMember = index; SyncDetails();
}
void CParisRankingWindow::SelectMission(int index)
{
    if (index < 0 || index >= static_cast<int>(Inspection::Data.missions.size())) return;
    m_RmlView.GetModel().missionSelected = index;
    Inspection::RequestMission(Inspection::Data.missions[index].id); SyncDetails();
}
void CParisRankingWindow::SyncModel()
{
    auto& model = m_RmlView.GetModel();
    if (m_seenList != Ranking::Received || m_query != model.query)
    { m_seenList = Ranking::Received; m_query = model.query; m_page = 0; m_rowsDirty = true; }
    if (m_rowsDirty) { m_rowsDirty = false; BuildRows(); }
    const auto& data = Inspection::Data;
    if (m_seenDetails != data.revision)
    {
        m_seenDetails = data.revision; SyncDetails();
        if (m_tab == 2) BuildRows();
    }
    if (m_seenEquipment != data.equipmentRevision) { m_seenEquipment = data.equipmentRevision; SyncEquipment(); }
    if (m_seenProfile != data.profileRevision) { m_seenProfile = data.profileRevision; BuildCharacter(); }
}
void CParisRankingWindow::BuildRows()
{
    auto& model = m_RmlView.GetModel();
    const auto query = Lower(StringUtils::NarrowToWide(model.query));
    m_filtered.clear(); model.rows.clear(); model.missions.clear();
    const auto& data = Ranking::Data;
    if (m_tab == 2)
    {
        const auto& missions = Inspection::Data.missions;
        for (int i = 0; i < static_cast<int>(missions.size()); ++i)
        {
            const auto& mission = missions[i];
            if (Lower(mission.name + L" " + mission.objective).find(query) == std::wstring::npos) continue;
            model.missions.push_back({"#" + std::to_string(mission.id), Text(mission.name), std::to_string(mission.required) + " x " + Text(mission.objective),
                std::to_string(mission.reward), Text(mission.rewardName), i, static_cast<int>(model.missions.size())});
        }
        if (!missions.empty() && (model.missionSelected < 0 || model.missionSelected >= static_cast<int>(missions.size()))) SelectMission(0);
    }
    else if (Ranking::Received && data.tab == static_cast<Ranking::Tab>(m_tab))
    {
        for (int i = 0; i < static_cast<int>(data.entries.size()); ++i)
            if (Lower(data.entries[i].name).find(query) != std::wstring::npos) m_filtered.push_back(i);
        model.page_count = std::max(1, (static_cast<int>(m_filtered.size()) + PageSize - 1) / PageSize);
        m_page = std::clamp(m_page, 0, model.page_count - 1); model.page = m_page + 1;
        for (int i = m_page * PageSize; i < std::min((m_page + 1) * PageSize, static_cast<int>(m_filtered.size())); ++i)
        {
            const int index = m_filtered[i]; const auto& entry = data.entries[index];
            model.rows.push_back({std::to_string(index + 1), Text(entry.name), Text(entry.secondary), std::to_string(entry.primaryValue), std::to_string(entry.secondaryValue), index, i % PageSize});
        }
        if (!model.rows.empty() && model.selected < 0) Select(model.rows.front().index);
    }
    model.empty = m_tab == 2 ? model.missions.empty() : model.rows.empty();
    model.status = model.empty ? "Sin resultados" : "Ranking actualizado del servidor";
    m_handle.DirtyAllVariables();
}
void CParisRankingWindow::SyncDetails()
{
    const auto& data = Inspection::Data; const auto& profile = data.profile;
    auto& model = m_RmlView.GetModel();
    model.hasProfile = profile.available; model.name = Text(profile.name); model.guild = profile.guild.empty() ? "Sin clan" : Text(profile.guild);
    model.characterClass = ClassName(profile.appearance[0]); model.level = std::to_string(profile.level);
    model.resets = std::to_string(profile.resets); model.totalStats = std::to_string(profile.stats);
    model.publicEquipment = data.publicEquipment; model.visibility = data.publicEquipment ? "EQUIPO VISIBLE" : "EQUIPO PRIVADO";
    model.ownPublic = data.ownPublic; model.preferenceReady = data.preferenceReady;
    model.mark.clear();
    constexpr const char* Colors[]{"#00000000", "#000000", "#808080", "#ffffff", "#ff0000", "#ff8000", "#ffff00", "#80ff00", "#00ff00", "#00ff80", "#00ffff", "#0080ff", "#0000ff", "#8000ff", "#ff00ff", "#ff0080"};
    for (auto byte : profile.mark) { model.mark.push_back({Colors[byte >> 4]}); model.mark.push_back({Colors[byte & 15]}); }
    model.members.clear(); model.memberPages = std::max(1, (static_cast<int>(data.members.size()) + MemberPageSize - 1) / MemberPageSize);
    m_memberPage = std::clamp(m_memberPage, 0, model.memberPages - 1); model.memberPage = m_memberPage + 1;
    for (int i = m_memberPage * MemberPageSize; i < std::min((m_memberPage + 1) * MemberPageSize, static_cast<int>(data.members.size())); ++i)
    {
        const auto& member = data.members[i];
        model.members.push_back({std::to_string(member.rank), Text(member.name), ClassName(member.characterClass), std::to_string(member.resets), "", i, i % MemberPageSize, BaseClass(member.characterClass)});
    }
    model.guildMembers = std::to_string(data.members.size()); model.guildScore.clear();
    const int selected = model.selected;
    if (m_tab == 1 && selected >= 0 && selected < static_cast<int>(Ranking::Data.entries.size())) model.guildScore = std::to_string(Ranking::Data.entries[selected].primaryValue);
    model.memberName.clear(); model.memberClass.clear(); model.memberOnline.clear(); model.memberRank.clear();
    model.strength.clear(); model.agility.clear(); model.vitality.clear(); model.energy.clear();
    if (!data.members.empty())
    {
        m_selectedMember = std::clamp(m_selectedMember, 0, static_cast<int>(data.members.size()) - 1);
        const auto& member = data.members[m_selectedMember];
        model.memberName = Text(member.name); model.memberClass = ClassName(member.characterClass);
        model.memberRank = "#" + std::to_string(member.rank); model.memberLevel = std::to_string(member.level);
        model.memberResets = std::to_string(member.resets); model.memberStats = std::to_string(member.stats);
        model.memberOnline = member.online ? "CONECTADO" : "DESCONECTADO";
        if (m_tab == 1) model.visibility = member.details ? "Públicos" : "Privados";
        if (member.details) { model.strength = std::to_string(member.attributes[0]); model.agility = std::to_string(member.attributes[1]); model.vitality = std::to_string(member.attributes[2]); model.energy = std::to_string(member.attributes[3]); }
    }
    model.missionRanks.clear();
    if (model.missionSelected >= 0 && model.missionSelected < static_cast<int>(data.missions.size()))
    {
        const auto& mission = data.missions[model.missionSelected];
        model.missionName = Text(mission.name); model.objective = "Derrota " + std::to_string(mission.required) + " " + Text(mission.objective);
        const auto amount = std::to_string(mission.reward) + " ";
        const auto rewardName = mission.rewardName.empty() ? Rml::String("Paris Coins") : Text(mission.rewardName);
        model.reward = rewardName.starts_with(amount) ? rewardName : amount + rewardName;
        const int percent = static_cast<int>(std::clamp(100LL * mission.current / std::max(1, mission.required), 0LL, 100LL));
        model.progress = std::to_string(mission.current) + " / " + std::to_string(mission.required) + " (" + std::to_string(percent) + "%)";
        model.progressWidth = std::to_string(percent) + "%";
        const auto& ranks = data.missionRanks;
        for (int i = 0; i < static_cast<int>(ranks.size()); ++i)
            model.missionRanks.push_back({std::to_string(i + 1), Text(ranks[i].name), std::to_string(ranks[i].progress) + " / " + std::to_string(mission.required), "", "", i, i});
    }
    m_handle.DirtyAllVariables();
}
void CParisRankingWindow::SyncEquipment()
{
    ClearItems(); auto& model = m_RmlView.GetModel(); model.equipment.clear();
    for (int i = 0; i < 12; ++i)
    {
        if (m_targets[i]) m_targets[i]->Invalidate();
        const int slot = CardSlots[i]; const auto& bytes = Inspection::Data.equipment[slot];
        if (g_pNewItemMng && Inspection::Data.publicEquipment && !bytes.empty()) m_items[i] = g_pNewItemMng->CreateItem(bytes);
        const auto* item = m_items[i];
        const Rml::String name = item && item->Type >= 0 && item->Type < MAX_ITEM ? Text(ItemAttribute[item->Type].Name) : "Vacío";
        model.equipment.push_back({i, SlotNames[slot], name, item != nullptr});
    }
    m_RmlView.MarkDirty("equipment");
}
void CParisRankingWindow::BuildCharacter()
{
    const auto& profile = Inspection::Data.profile;
    if (!profile.available || Inspection::Data.tab == 1 || !Hero) return;
    if (!m_character) { m_character = std::make_unique<CUIPhotoViewer>(); m_character->Init(0); m_character->SetCanControl(false); }
    auto* character = m_character->GetPhotoChar();
    const auto characterClass = gCharacterManager.ChangeServerClassTypeToClientClassType(static_cast<SERVER_CLASS_TYPE>(profile.appearance[0]));
    character->SkinIndex = gCharacterManager.GetSkinModelIndex(characterClass); m_character->SetClass(characterClass);
    std::array<BYTE, 25> equipment; std::copy_n(profile.appearance.begin() + 2, equipment.size(), equipment.begin());
    // Pets with world-owned lifetimes are not part of a detached profile preview.
    equipment[23] = equipment[24] = 0xFF;
    for (int hand = 0; hand < 2; ++hand) if (((equipment[hand * 3] & 0xF0) >> 4) == ITEM_GROUP_HELPER && MAKEWORD(equipment[hand * 3 + 1], equipment[hand * 3] & 0x0F) == ITEM_NUMBER_DARK_SPIRIT) std::fill_n(equipment.data() + hand * 3, 3, 0xFF);
    m_character->SetEquipmentPacket(equipment.data()); m_character->SetID(profile.name.c_str());
    m_character->SetAngle(90.f); m_character->SetZoom(.55f); m_character->SetAnimation(AT_STAND1);
}
void CParisRankingWindow::UpdatePreviews()
{
    auto* document = m_RmlView.Document(); if (!document) return;
    static thread_local Rml::ElementList cards; cards.clear(); document->QuerySelectorAll(cards, ".ranking-equipment-card");
    std::erase_if(cards, [](auto* e) { return !e->IsVisible(); }); int hovered = -1;
    for (int i = 0; i < 12; ++i)
    {
        auto* card = i < static_cast<int>(cards.size()) ? cards[i] : nullptr;
        if (m_targets[i]) m_targets[i]->Sync(card ? card->QuerySelector("img") : nullptr, m_tab == 0 && m_items[i]);
        if (card && UI::RmlBridge::IsPointerWithin(card)) hovered = i;
    }
    if (hovered >= 0 && m_items[hovered]) { RenderItemInfoAtPx(g_fWindowMouseX, g_fWindowMouseY, m_items[hovered], false); m_tooltipShown = true; }
    else if (m_tooltipShown) { UI::Tooltip::HideLegacyTextList(); m_tooltipShown = false; }
    static thread_local Rml::ElementList missions; missions.clear();
    document->QuerySelectorAll(missions, ".mission-row");
    std::erase_if(missions, [](auto* row) { return !row->IsVisible(); });
    Rml::Vector2f listOffset, listSize;
    auto* list = document->QuerySelector(".mission-list");
    const bool listPlaced = list && m_tab == 2 && UI::RmlBridge::DrawnContentBox(*list, listOffset, listSize);
    std::size_t visibleSlot = 0;
    for (auto* row : missions)
    {
        const int index = row->GetAttribute<int>("data-index", -1);
        if (!listPlaced || index < 0 || index >= static_cast<int>(Inspection::Data.missions.size()) || visibleSlot >= 6) continue;
        Rml::Vector2f offset, size;
        if (!UI::RmlBridge::DrawnContentBox(*row, offset, size) || offset.y + size.y <= listOffset.y || offset.y >= listOffset.y + listSize.y) continue;
        if (!m_monsters[visibleSlot]) m_monsters[visibleSlot] = std::make_unique<Render::MonsterPreviews::MonsterPreviewTarget>();
        m_monsters[visibleSlot++]->Sync(row->QuerySelector(".mission-monster-small"), Inspection::Data.missions[index].monster);
    }
    for (std::size_t slot = visibleSlot; slot < 6; ++slot) if (m_monsters[slot]) m_monsters[slot]->Disable();
    const int mission = m_RmlView.GetModel().missionSelected;
    if (m_tab == 2 && mission >= 0 && mission < static_cast<int>(Inspection::Data.missions.size()))
    {
        if (!m_monsters[6]) m_monsters[6] = std::make_unique<Render::MonsterPreviews::MonsterPreviewTarget>();
        m_monsters[6]->Sync(document->GetElementById("mission-monster-large"), Inspection::Data.missions[mission].monster);
    }
    else if (m_monsters[6]) m_monsters[6]->Disable();
    if (!m_character) return;
    auto* image = document->GetElementById("ranking-character"); Rml::Vector2f offset, size;
    const bool visible = m_tab == 0 && m_RmlView.GetModel().hasProfile && image && UI::RmlBridge::DrawnContentBox(*image, offset, size);
    auto& target = m_character->Target(); target.SetEnabled(visible);
    if (!visible) return;
    target.Resize(static_cast<std::uint32_t>(std::lround(size.x)), static_cast<std::uint32_t>(std::lround(size.y)));
    if (!target.Source().empty())
    {
        if (image->GetAttribute<Rml::String>("src", "") != target.Source())
            image->SetAttribute("src", target.Source());
        image->RemoveProperty("visibility");
    }
    else image->SetProperty("visibility", "hidden");
}
void CParisRankingWindow::DrawItem(int slot, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    if (slot < 0 || slot >= 12 || !m_items[slot]) return;
    const auto& item = *m_items[slot]; const Render::Items::Preview::ScopedFit fit(offset.x, offset.y, size.x, size.y);
    RenderItem3D(offset.x, offset.y, size.x, size.y, item.Type, item.Level, item.ExcellentFlags, item.AncientDiscriminator, false);
}
}
