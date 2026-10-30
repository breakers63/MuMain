#include "stdafx.h"
#include "Network/Paris/EventEntry.h"

#include "UI/Events/EventEntryView.h"

#include "I18N/All.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/RmlBridge/RmlSyncField.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlNativeTextSize.h"

#include <RmlUi/Core/ElementDocument.h>

using namespace mu::ui::window;

mu::ui::window::EventEntryView::EventEntryView(const char* modelName, const char* documentPath)
    : m_View(modelName, [this](Rml::DataModelConstructor& c, EventEntryRmlModel& model) { BindModel(c, model); },
             {{documentPath}}, {.stacking = UI::RmlBridge::ThemedStacking::Front})
{
}

void mu::ui::window::EventEntryView::BindModel(Rml::DataModelConstructor& c, EventEntryRmlModel& model)
{
    c.Bind("clan_race", &model.clanRace);
    c.Bind("profile_generals", &model.profileGenerals);
    c.Bind("profile_race", &model.profileRace);
    c.Bind("profile_guild", &model.profileGuild);
    c.Bind("reward_paging", &model.rewardPaging);
    c.Bind("clan_count", &model.clanCount);
    c.Bind("player_count", &model.playerCount);
    c.Bind("requirement", &model.requirement);
    c.Bind("stage_left", &model.stageLeft);
    c.Bind("stage_middle", &model.stageMiddle);
    c.Bind("stage_right", &model.stageRight);
    c.Bind("reward_page", &model.rewardPage);
    c.Bind("rich", &model.rich);c.Bind("profile_tvt", &model.profileTvt);c.Bind("profile_sky", &model.profileSky);c.Bind("entry_enabled", &model.entryEnabled);
    c.Bind("event_status", &model.eventStatus);c.Bind("participant_count", &model.participantCount);c.Bind("event_clock", &model.eventClock);
    c.Bind("minimum_text", &model.minimumText);c.Bind("missing_text", &model.missingText);c.Bind("entry_limits", &model.entryLimits);c.Bind("entry_ticket", &model.entryTicket);c.Bind("development", &model.development);
    c.Bind("reward_0", &model.reward0);c.Bind("reward_1", &model.reward1);c.Bind("reward_2", &model.reward2);c.Bind("action_text", &model.actionText);
    c.Bind("text_px", &model.textPx);
    c.Bind("bold_text_px", &model.boldTextPx);
    c.Bind("title_text", &model.titleText);
    c.Bind("exit_tooltip", &model.exitTooltip);

    auto line = c.RegisterStruct<EventEntryLineEntry>();
    line.RegisterMember("text", &EventEntryLineEntry::text);
    c.RegisterArray<std::vector<EventEntryLineEntry>>();
    c.Bind("lines", &model.lines);

    auto button = c.RegisterStruct<EventEntryButtonEntry>();
    button.RegisterMember("label", &EventEntryButtonEntry::label);
    button.RegisterMember("enabled", &EventEntryButtonEntry::enabled);
    c.RegisterArray<std::vector<EventEntryButtonEntry>>();
    c.Bind("buttons", &model.buttons);

    c.BindEventCallback("entry_press",
                        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& arguments)
                        {
                            if (arguments.size() == 1)
                                m_PressedButton = arguments[0].Get<int>(-1);
                        });
    c.BindEventCallback("entry_exit", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
                        { m_ExitPressed = true; });
    c.BindEventCallback("entry_reward_page", [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        namespace E = Network::Paris::Entry;
        if (args.size() != 1 || !E::Rich()) return;
        const int pages = std::max(1, (E::RewardCount + 1) / 2);
        E::RewardPage = (E::RewardPage + pages + args[0].Get<int>(0)) % pages;
    });
    model.exitTooltip = StringUtils::WideToNarrow(I18N::Game::Close388);
}

void mu::ui::window::EventEntryView::Build()
{
    m_View.Ensure();
}

void mu::ui::window::EventEntryView::SetContent(const wchar_t* title, const std::vector<std::wstring>& lines,
                                                const std::vector<Button>& buttons)
{
    Build();
    EventEntryRmlModel& model = m_View.GetModel();
    model.titleText = StringUtils::WideToNarrow(title != nullptr ? title : L"");
    model.lines.clear();
    for (const std::wstring& text : lines)
        model.lines.push_back({StringUtils::WideToNarrow(text.c_str())});
    model.buttons.clear();
    for (const Button& button : buttons)
        model.buttons.push_back({StringUtils::WideToNarrow(button.label.c_str()), button.enabled});
    m_View.MarkDirty("title_text");
    m_View.MarkDirty("lines");
    m_View.MarkDirty("buttons");
}

bool mu::ui::window::EventEntryView::IsPointerOver() const
{
    return UI::RmlBridge::IsPointerOver(m_View.Document());
}

void mu::ui::window::EventEntryView::Sync(bool visible)
{
    Build();
    if (!m_View.Document())
        return;

    // Layer depth 4.0/4.1: over the HUD like every panel the original opened.
    UI::RmlBridge::SyncDocumentVisibilityInFront(m_View.Document(), visible);
    if (!visible)
        return;

    SyncTextSizes();
    SyncProfile();
}

void mu::ui::window::EventEntryView::SyncTextSizes()
{
    SyncField(m_View.Binder(), &EventEntryRmlModel::textPx, "text_px",
              UI::RmlBridge::NativeTextPx(UI::Scaling::FontRole::Normal));
    SyncField(m_View.Binder(), &EventEntryRmlModel::boldTextPx, "bold_text_px",
              UI::RmlBridge::NativeTextPx(UI::Scaling::FontRole::Bold));
}

int mu::ui::window::EventEntryView::TakePressedButton()
{
    const int pressed = m_PressedButton;
    m_PressedButton = -1;
    const EventEntryRmlModel& model = m_View.GetModel();
    if (pressed < 0 || pressed >= static_cast<int>(model.buttons.size()) || !model.buttons[pressed].enabled)
        return -1;
    return pressed;
}

bool mu::ui::window::EventEntryView::TakeExitPressed()
{
    const bool pressed = m_ExitPressed;
    m_ExitPressed = false;
    return pressed;
}

void EventEntryView::SyncProfile()
{
    namespace E=Network::Paris::Entry;
    auto& model=m_View.GetModel();
    const bool rich=E::Rich() && model.titleText==StringUtils::WideToNarrow(E::Title.c_str());
    const auto flag=[this](bool& field,bool value,const char* key){if(field!=value){field=value;m_View.MarkDirty(key);}};
    const auto text=[this](Rml::String& field,const std::wstring& value,const char* key){auto converted=StringUtils::WideToNarrow(value.c_str());if(field!=converted){field=std::move(converted);m_View.MarkDirty(key);}};
    flag(model.rich,rich,"rich");if(!rich)return;
    flag(model.profileTvt,E::Mode==L"tvt","profile_tvt");flag(model.profileSky,E::Mode==L"sky","profile_sky");flag(model.entryEnabled,E::Available(0),"entry_enabled");
    text(model.eventStatus,E::Status(),"event_status");text(model.participantCount,std::to_wstring(E::Participants)+L" / "+std::to_wstring(E::Maximum),"participant_count");text(model.eventClock,E::Clock(),"event_clock");
    text(model.minimumText,L"Minimo para iniciar: "+std::to_wstring(E::Minimum)+L" jugadores","minimum_text");text(model.missingText,E::Participants<E::Minimum?L"Faltan "+std::to_wstring(E::Minimum-E::Participants)+L" jugadores":L"Minimo de inscritos alcanzado","missing_text");
    text(model.entryLimits,E::Limits(),"entry_limits");text(model.entryTicket,L"Entrada: "+E::Ticket,"entry_ticket");text(model.development,E::Mode==L"tvt"?L"Equipos equilibrados | "+std::to_wstring(E::Duration/60)+L" minutos":std::to_wstring(E::Stages)+L" fases | Puntos para el ranking","development");
    if (!model.clanRace) { text(model.reward0,E::Rewards[0].empty()?L"Sin premios configurados":E::Rewards[0],"reward_0");text(model.reward1,E::Rewards[1],"reward_1");text(model.reward2,E::Rewards[2],"reward_2");} text(model.actionText,E::Available(0)?L"INSCRIBIRME":L"NO DISPONIBLE","action_text");
    SyncClanProfile();
    if(!model.buttons.empty()&&model.buttons[0].enabled!=E::Available(0)){model.buttons[0].enabled=E::Available(0);m_View.MarkDirty("buttons");}
}

void EventEntryView::SyncClanProfile()
{
    namespace E = Network::Paris::Entry;
    auto& m = m_View.GetModel();
    const bool generals = E::Mode == L"generals", race = E::Mode == L"race", guild = E::Mode == L"guild-battle";
    const auto flag = [this](bool& field, bool value, const char* key)
    { if (field != value) { field = value; m_View.MarkDirty(key); } };
    flag(m.clanRace, generals || race || guild, "clan_race");
    flag(m.profileGenerals, generals, "profile_generals");
    flag(m.profileRace, race, "profile_race");
    flag(m.profileGuild, guild, "profile_guild");
    if (!m.clanRace) return;
    const auto set = [this](Rml::String& field, const std::wstring& value, const char* key)
    { auto converted = StringUtils::WideToNarrow(value.c_str()); if (field != converted) { field = std::move(converted); m_View.MarkDirty(key); } };
    set(m.clanCount, race ? std::to_wstring(E::Participants)+L" / "+std::to_wstring(E::Maximum) : std::to_wstring(E::Clans)+L" / "+std::to_wstring(E::MaxClans), "clan_count");
    set(m.playerCount, L"Personas: "+std::to_wstring(E::Participants)+L"/"+std::to_wstring(E::Maximum), "player_count");
    set(m.requirement, race ? E::Requirement : L"Minimo "+std::to_wstring(E::MinClans)+L" clanes | "+std::to_wstring(E::MinMembers)+L" por clan", "requirement");
    set(m.minimumText, E::Requirement, "minimum_text");
    if (generals) set(m.entryLimits, L"Nivel "+std::to_wstring(E::MinLevel)+L"-"+std::to_wstring(E::MaxLevel)+L" | RR "+std::to_wstring(E::MinReset)+L"-"+std::to_wstring(E::MaxReset), "entry_limits");
    set(m.stageLeft, race?L"Salida":generals?L"Tu general":L"Tu clan", "stage_left");
    set(m.stageMiddle, race?L"Recorrido":L"Combate", "stage_middle");
    set(m.stageRight, race?L"Meta":generals?L"General rival":L"Clan rival", "stage_right");
    set(m.development, L"Duracion: "+std::to_wstring(E::Duration/60)+L" min"+(race?L" | Vueltas: "+std::to_wstring(E::Laps):L""), "development");
    set(m.reward0, E::RewardCount ? E::Rewards[E::RewardPage*2] : L"Sin premios configurados", "reward_0");
    set(m.reward1, E::RewardPage*2+1<E::RewardCount?E::Rewards[E::RewardPage*2+1]:L"", "reward_1");
    set(m.rewardPage, std::to_wstring(E::RewardPage+1)+L" / "+std::to_wstring(std::max(1,(E::RewardCount+1)/2)), "reward_page");
    flag(m.rewardPaging, E::RewardCount > 2, "reward_paging");
}
