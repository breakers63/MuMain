#pragma once

#include "Network/Paris/CustomRankingProtocol.h"
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <vector>
#include <array>
#include <memory>
#include "UI/Inventory/ItemCameraTarget.h"
struct tagITEM;
class CUIPhotoViewer;
namespace Render::MonsterPreviews { class MonsterPreviewTarget; }

namespace mu::ui::window
{
class CManager;

class CParisRankingWindow final : public CObject
{
public:
    CParisRankingWindow();
    ~CParisRankingWindow() override;
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
    struct Row
    {
        Rml::String rank, name, detail, value, extra;
        int index = 0, slot = 0, characterClass = 0;
        bool operator==(const Row&) const = default;
    };
    struct Equipment { int slot = 0; Rml::String label, name; bool present = false; };
    struct MarkCell { Rml::String color; };
    struct Model
    {
        Rml::String title = "RANKING", status, query, name, guild, characterClass, level, resets, totalStats, visibility;
        Rml::String memberName, memberClass, memberRank, memberLevel, memberResets, memberStats, memberOnline, guildScore, guildMembers;
        Rml::String strength, agility, vitality, energy, missionName, objective, reward, progress, progressWidth;
        std::vector<Row> rows, members, missions, missionRanks;
        std::vector<Equipment> equipment;
        std::vector<MarkCell> mark;
        int tab = 0, page = 1, page_count = 1, selected = -1, memberPage = 1, memberPages = 1, missionSelected = -1;
        bool empty = true, hasProfile = false, publicEquipment = false, ownPublic = false, preferenceReady = false;
    };
    void BindModel(Rml::DataModelConstructor& constructor, Model& model);
    void BindActions(Rml::DataModelConstructor& constructor);
    void SyncModel();
    void BuildRows();
    void SyncDetails();
    void SyncEquipment();
    void UpdatePreviews();
    void BuildCharacter();
    void ClearItems();
    void RequestTab(int tab);
    void ChangePage(int delta);
    void Select(int index);
    void SelectMember(int index);
    void SelectMission(int index);
    void DrawItem(int slot, const Rml::Vector2f& offset, const Rml::Vector2f& size);
    CManager* m_manager = nullptr;
    int m_tab = 0, m_page = 0, m_memberPage = 0, m_selectedMember = 0;
    std::uint64_t m_seenList = 0, m_seenDetails = ~0ull, m_seenEquipment = ~0ull, m_seenProfile = ~0ull;
    bool m_rowsDirty = true, m_tooltipShown = false;
    Rml::String m_query;
    std::vector<int> m_filtered;
    std::array<tagITEM*, 12> m_items{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, 12> m_targets;
    std::unique_ptr<CUIPhotoViewer> m_character;
    std::array<std::unique_ptr<Render::MonsterPreviews::MonsterPreviewTarget>, 7> m_monsters;
    Rml::DataModelHandle m_handle;
    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_ranking",
        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/paris_ranking.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};

extern CParisRankingWindow* g_pParisRankingWindow;
} // namespace mu::ui::window
