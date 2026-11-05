#include "stdafx.h"
#include "ActiveInvasionWindow.h"
#include "Network/Paris/ParisPacketDispatch.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Core/WindowManager.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "Scenes/SceneCore.h"
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <algorithm>
namespace mu::ui::window
{
CActiveInvasionWindow::~CActiveInvasionWindow()
{
    Release();
}
bool CActiveInvasionWindow::Create(CManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    constexpr DWORD InterfaceId = 0x41494E56u;
    manager->AddUIObj(InterfaceId, this);
    m_registration = Network::Paris::RegisterTextHandler(
        [this](std::wstring_view text)
        {
            if (!text.starts_with(L"[#AINV:"))
                return false;
            if (auto update = Network::Paris::ActiveInvasion::Decode(text))
            {
                std::lock_guard lock(m_mutex);
                constexpr std::size_t MaximumPending = 512;
                if (m_pending.size() < MaximumPending)
                    m_pending.push_back(std::move(*update));
            }
            return true;
        });
    CObject::Show(true);
    return true;
}
void CActiveInvasionWindow::Release()
{
    m_preview.reset();
    Network::Paris::UnregisterTextHandler(m_registration);
    m_registration = 0;
    m_view.Release();
    if (m_manager)
        m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CActiveInvasionWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name);
    row.RegisterMember("count", &Row::count);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("rows", &model.rows);
    c.Bind("active", &model.active);
    c.Bind("open", &model.open);
    c.Bind("title", &model.title);
    c.Bind("time", &model.time);
    c.Bind("preview_visible", &model.previewVisible);
    c.Bind("preview_name", &model.previewName);
    c.Bind("preview_count", &model.previewCount);
    c.Bind("summary", &model.summary);
    c.BindEventCallback("invasion_toggle",
                        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
                        {
                            auto& model = m_view.GetModel();
                            model.open = !model.open;
                            m_view.MarkDirty("open");
                        });
    c.BindEventCallback("invasion_close",
                        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
                        {
                            m_view.GetModel().open = false;
                            m_view.MarkDirty("open");
                        });
}
void CActiveInvasionWindow::Apply(const Network::Paris::ActiveInvasion::Update& update)
{
    if (update.cleared) {m_events.clear(); return;}
    if (update.ended)
    {
        m_events.erase(update.event);
        return;
    }
    constexpr std::size_t MaximumEvents = 32, MaximumRows = 32;
    if (!m_events.contains(update.event) && m_events.size() >= MaximumEvents)
        return;
    auto& event = m_events[update.event];
    event.name = StringUtils::WideToNarrow(update.eventName.c_str());
    event.deadline = GetTickCount64() + static_cast<unsigned long long>(update.seconds) * 1000;
    auto found = std::find_if(event.rows.begin(), event.rows.end(),
                              [&](const Row& row) { return row.monster == update.monster; });
    Row row{StringUtils::WideToNarrow(update.monsterName.c_str()),
            std::to_string(update.alive) + " / " + std::to_string(update.total), update.monster, update.alive, update.total};
    if (found != event.rows.end())
        *found = std::move(row);
    else if (event.rows.size() < MaximumRows)
        event.rows.push_back(std::move(row));
}
bool CActiveInvasionWindow::Update()
{
    if (!m_view.Ensure())
        return true;
    std::vector<Network::Paris::ActiveInvasion::Update> pending;
    {
        std::lock_guard lock(m_mutex);
        pending.swap(m_pending);
    }
    for (const auto& update : pending)
        Apply(update);
    if (!pending.empty() || GetTickCount64() >= m_nextSync)
    {
        m_nextSync = GetTickCount64() + 1000;
        Sync();
    }
    if (auto* document = m_view.Document(); document && !document->IsVisible())
        document->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    return true;
}
void CActiveInvasionWindow::Sync()
{
    const auto now = GetTickCount64();
    std::erase_if(m_events, [&](const auto& pair) { return pair.second.deadline <= now; });
    auto& model = m_view.GetModel();
    model.active = !m_events.empty();
    if (model.active)
    {
        if (!m_events.contains(m_selected))
            m_selected = m_events.begin()->first;
        const auto& event = m_events.at(m_selected);
        model.title = event.name;
        model.rows = event.rows;
        int alive = 0, total = 0;
        for (const auto& row : event.rows) { alive += row.alive; total += row.total; }
        model.summary = "Restantes: " + std::to_string(alive) + " / " + std::to_string(total);
        const auto seconds = (event.deadline - now + 999) / 1000;
        model.time = "Tiempo: " + std::to_string(seconds / 60) + ":" + (seconds % 60 < 10 ? "0" : "") +
                     std::to_string(seconds % 60);
    }
    else
    {
        model.open = false;
        model.rows.clear();
    }
    for (const char* name : {"active", "open", "title", "time", "rows", "summary"})
        m_view.MarkDirty(name);
}

bool CActiveInvasionWindow::Render()
{
    UpdatePreview();
    return true;
}

void CActiveInvasionWindow::UpdatePreview()
{
    auto* document = m_view.Document();
    auto& model = m_view.GetModel();
    int hovered = -1;
    if (SceneFlag == MAIN_SCENE && document && document->IsVisible() && model.active && model.open)
    {
        auto* list = document->QuerySelector(".invasion-rows");
        if (list && UI::RmlBridge::IsPointerWithin(list))
        {
            static thread_local Rml::ElementList rows;
            rows.clear();
            document->QuerySelectorAll(rows, ".invasion-row");
            for (auto* row : rows)
                if (UI::RmlBridge::IsPointerWithin(row)) hovered = row->GetAttribute<int>("data-index", -1);
        }
    }
    const bool visible = hovered >= 0 && hovered < static_cast<int>(model.rows.size());
    if (model.previewVisible != visible)
    {
        model.previewVisible = visible;
        m_view.MarkDirty("preview_visible");
    }
    if (!visible)
    {
        m_hovered = -1;
        if (m_preview) m_preview->Disable();
        return;
    }
    const auto& row = model.rows[hovered];
    if (m_hovered != hovered || model.previewName != row.name || model.previewCount != row.count)
    {
        m_hovered = hovered;
        model.previewName = row.name;
        model.previewCount = row.count;
        m_view.MarkDirty("preview_name");
        m_view.MarkDirty("preview_count");
    }
    if (!m_preview) m_preview = std::make_unique<Render::MonsterPreviews::MonsterPreviewTarget>();
    m_preview->Sync(document->GetElementById("invasion-monster-preview"), row.monster);
}
} // namespace mu::ui::window
