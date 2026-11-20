#include "stdafx.h"
#include <doctest.h>
#include "RmlLayoutFixture.h"
#include "UI/Events/ParisEventPages.h"
#include <regex>

TEST_CASE("Paris event guide prioritizes active registration and upcoming events [ui][events]")
{
    using Network::Paris::EventGuide::Event;
    std::vector<Event> events{
        {"devil", "Devil", "classic", "idle", 3600},
        {"blood", "Blood", "classic", "idle", 60},
        {"permanent", "Permanent", "classic", "available", -1},
        {"disabled", "Disabled", "classic", "disabled", 0},
        {"active", "Active", "classic", "running", 900},
        {"open", "Open", "classic", "registration", 120},
        {"tie", "Tie", "classic", "idle", 60}};
    const auto pages = UI::Events::BuildGuidePages(events);
    REQUIRE(pages.size() == 1);
    const std::vector<std::string> expected{"active", "open", "blood", "tie", "devil", "permanent", "disabled"};
    for (std::size_t i = 0; i < expected.size(); ++i) CHECK(pages[0].events[i]->key == expected[i]);
}

namespace
{
struct RequirementRow { Rml::String name, value; int state = 1; };
float Right(Rml::Element* element) { return element->GetAbsoluteOffset().x + element->GetBox().GetSize().x; }
float Bottom(Rml::Element* element) { return element->GetAbsoluteOffset().y + element->GetBox().GetSize().y; }
}

TEST_CASE("Paris event guide selects the actual runtime artwork bindings [ui][events]")
{
    UI::Tests::RmlLayoutFixture fixture;
    Rml::String title;
    auto model = fixture.context->CreateDataModel("event_art");
    model.Bind("title", &title);
    auto handle = model.GetModelHandle();
    const auto raw = UI::Tests::ReadFile(std::filesystem::path(MU_RMLUI_DIR) / "paris_events.rml");
    const auto start = raw.find("<div class=\"guide-banner\">");
    const auto end = raw.find("<img id=\"guide-boss-model\"", start);
    REQUIRE(start != std::string::npos);
    REQUIRE(end != std::string::npos);
    const auto variants = raw.substr(start + 26, end - start - 26);
    const auto styled = UI::Tests::AssetMarkup("paris-gold", "paris_events");
    auto* doc = fixture.LoadMarkup(styled.substr(0, styled.find("</head>") + 7) +
        "<body data-model=\"event_art\"><div id=\"panel\"><div class=\"guide-banner\">" + variants + "</div></div></body></rml>");
    for (const auto& [name, slug] : std::vector<std::pair<Rml::String, Rml::String>>{
        {"Devil Square 7", "devil-square"}, {"Kanturu Refinery Tower", "kanturu"},
        {"Erohim Invasion", "erohim"}, {"Medusa Boss", "medusa"},
        {"Happy Hour", "happy-hour"}, {"Torre Eiffel", "tower"},
        {"Guerra de Generales", "generals"}, {"Kardamahal", "kardamahal"}})
    {
        CAPTURE(name);
        title = name;
        handle.DirtyVariable("title");
        fixture.Refresh();
        Rml::ElementList images;
        doc->QuerySelectorAll(images, ".guide-landscape");
        std::vector<Rml::Element*> visible;
        for (auto* item : images) if (item->IsVisible()) visible.push_back(item);
        REQUIRE(visible.size() == 1);
        CHECK(visible.front()->IsClassSet("art-" + slug));
    }
}

TEST_CASE("Paris event guide keeps list and detail columns separated [ui][events]")
{
    for (const auto* theme : {"paris-gold", "legacy", "modern"})
    {
        for (const auto viewport : {Rml::Vector2i{1024, 768}, Rml::Vector2i{1280, 720}})
        {
            CAPTURE(theme); CAPTURE(viewport.x); CAPTURE(viewport.y);
            UI::Tests::RmlLayoutFixture fixture;
            fixture.Configure(viewport, 100);
            std::vector<RequirementRow> rows(10, {"Entrada necesaria", "Cloak +3 x1", 1});
            auto model = fixture.context->CreateDataModel("event_layout");
            auto rowType = model.RegisterStruct<RequirementRow>();
            rowType.RegisterMember("name", &RequirementRow::name);
            rowType.RegisterMember("value", &RequirementRow::value);
            rowType.RegisterMember("state", &RequirementRow::state);
            model.RegisterArray<std::vector<RequirementRow>>();
            model.Bind("requirements", &rows);
            int split = 5;
            model.Bind("requirement_split", &split);
            auto handle = model.GetModelHandle();
            auto markup = UI::Tests::AssetMarkup(theme, "paris_events");
            markup.replace(markup.find("<body"), 5, "<body data-model=\"event_layout\"");
            const auto compactAt = markup.find("class=\"guide-scroll\"");
            REQUIRE(compactAt != std::string::npos);
            markup.insert(compactAt, "data-class-compact=\"requirement_split &gt; 3\" ");
            auto* doc = fixture.LoadMarkup(markup);
            auto* list = doc->QuerySelector(".guide-events");
            REQUIRE(list);
            list->SetInnerRML(R"(<button class="guide-event"><span>Imperial Guardian Sunday</span><strong>00:55:34</strong></button><button class="guide-event"><span>Blood Castle 8</span><strong>Disponible</strong></button>)");
            auto* scroll = doc->QuerySelector(".guide-scroll");
            REQUIRE(scroll);
            scroll->SetInnerRML(R"(<div class="guide-requirement-column left"><div class="guide-requirement" data-for="row, i : requirements" data-if="i &lt; requirement_split"><span>{{row.name}}</span><strong>{{row.value}}</strong></div></div><div class="guide-requirement-column right"><div class="guide-requirement" data-for="row, i : requirements" data-if="i >= requirement_split"><span>{{row.name}}</span><strong>{{row.value}}</strong></div></div>)");
            fixture.Refresh();
            fixture.Refresh();

            auto* row = list->QuerySelector(".guide-event");
            auto* label = row->QuerySelector("span");
            auto* value = row->QuerySelector("strong");
            CHECK(row->GetBox().GetSize().x > list->GetBox().GetSize().x * .9f);
            CHECK(label->GetBox().GetSize().x > row->GetBox().GetSize().x * .45f);
            CHECK(Right(label) <= value->GetAbsoluteOffset().x + 1.f);
            CHECK(Right(value) <= Right(row) + 1.f);
            CHECK(Bottom(row) <= list->GetChild(1)->GetAbsoluteOffset().y + 1.f);

            auto* leftColumn = scroll->QuerySelector(".left");
            auto* rightColumn = scroll->QuerySelector(".right");
            REQUIRE(leftColumn);
            REQUIRE(rightColumn);
            Rml::ElementList renderedRows;
            leftColumn->QuerySelectorAll(renderedRows, ".guide-requirement");
            auto visibleCount = std::count_if(renderedRows.begin(), renderedRows.end(), [](auto* item) { return item->IsVisible(); });
            CHECK(visibleCount == 5);
            for (auto* item : renderedRows)
                if (item->IsVisible()) CHECK(Bottom(item) <= Bottom(leftColumn) + 1.f);
            auto* requirement = *std::find_if(renderedRows.begin(), renderedRows.end(), [](auto* item) { return item->IsVisible(); });
            CHECK(Bottom(requirement->QuerySelector("span")) <= requirement->QuerySelector("strong")->GetAbsoluteOffset().y + 1.f);
            CHECK(Right(leftColumn) <= rightColumn->GetAbsoluteOffset().x + 1.f);
            CHECK(Bottom(requirement->QuerySelector("strong")) <= Bottom(requirement) + 1.f);
            auto* requirements = doc->QuerySelector(".guide-requirements");
            auto* monsters = doc->QuerySelector(".guide-monsters");
            CHECK(Right(requirements) <= monsters->GetAbsoluteOffset().x + 1.f);
            auto* metrics = doc->QuerySelector(".guide-metrics");
            auto* banner = doc->QuerySelector(".guide-banner");
            const auto bannerAspect = banner->GetBox().GetSize().x / banner->GetBox().GetSize().y;
            CHECK(bannerAspect > 4.5f);
            CHECK(bannerAspect < 5.5f);
            CHECK(Bottom(banner) <= metrics->GetAbsoluteOffset().y + 1.f);
            CHECK(Right(metrics->GetChild(0)) <= metrics->GetChild(1)->GetAbsoluteOffset().x + 1.f);
            CHECK(Bottom(metrics) <= requirements->GetAbsoluteOffset().y + 1.f);
            CHECK(Bottom(doc->QuerySelector(".guide-footer")) < Bottom(doc->GetElementById("panel")));
            rows.resize(4); split = 2;
            handle.DirtyVariable("requirements"); handle.DirtyVariable("requirement_split");
            fixture.Refresh(); fixture.Refresh();
            for (auto* column : {leftColumn, rightColumn})
            {
                Rml::ElementList balanced;
                column->QuerySelectorAll(balanced, ".guide-requirement");
                CHECK(std::count_if(balanced.begin(), balanced.end(), [](auto* item) { return item->IsVisible(); }) == 2);
            }

        }
    }
}

TEST_CASE("Paris event guide category pages never mix sections or lose events [ui][events]")
{
    using Network::Paris::EventGuide::Event;
    std::vector<Event> events{{"timer-7", "Erohim", "boss"}, {"dungeon-1", "Kanturu", "custom"}, {"mini-1", "Devil Square", "classic"}};
    for (int i = 0; i < 11; ++i) events.push_back({"custom-" + std::to_string(i), "Custom", "custom"});
    const auto pages = UI::Events::BuildGuidePages(events);
    REQUIRE(pages.size() == 5);
    CHECK(pages[0].category == "classic");
    CHECK(pages[1].category == "boss");
    CHECK(pages[2].category == "dungeon");
    CHECK(pages[3].events.size() == 9);
    CHECK(pages[4].events.size() == 2);
    std::size_t total = 0;
    for (const auto& page : pages)
    {
        total += page.events.size();
        for (const auto* event : page.events) CHECK(UI::Events::GuideCategory(*event) == page.category);
    }
    CHECK(total == events.size());
    CHECK(UI::Events::BuildGuidePages({}).empty());
}

namespace
{
void CheckRequirementTextFits(Rml::Element* column)
{
    Rml::ElementList rows;
    column->QuerySelectorAll(rows, ".guide-requirement");
    Rml::Element* previous = nullptr;
    for (auto* row : rows)
    {
        auto* label = row->QuerySelector("span");
        auto* value = row->QuerySelector("strong");
        REQUIRE(label);
        REQUIRE(value);
        CAPTURE(value->GetInnerRML());
        CHECK(label->GetScrollHeight() <= label->GetClientHeight() + 1.f);
        CHECK(value->GetScrollHeight() <= value->GetClientHeight() + 1.f);
        CHECK(value->GetScrollWidth() <= value->GetClientWidth() + 1.f);
        CHECK(Bottom(label) <= value->GetAbsoluteOffset().y + 1.f);
        CHECK(Bottom(value) <= Bottom(row) + 1.f);
        if (previous) CHECK(Bottom(previous) <= row->GetAbsoluteOffset().y + 1.f);
        previous = row;
    }
    REQUIRE(previous);
    CHECK(Bottom(previous) <= Bottom(column) + 1.f);
}
}

TEST_CASE("Paris event guide wraps complete live access instructions [ui][events]")
{
    for (const auto* theme : {"paris-gold", "legacy", "modern"})
    {
        for (const auto viewport : {Rml::Vector2i{1024, 768}, Rml::Vector2i{1280, 720}, Rml::Vector2i{1920, 1080}})
        {
            CAPTURE(theme); CAPTURE(viewport.x);
            UI::Tests::RmlLayoutFixture fixture;
            fixture.Configure(viewport, 100);
            auto* doc = fixture.Load(theme, "paris_events");
            auto* scroll = doc->QuerySelector(".guide-scroll");
            REQUIRE(scroll);
            scroll->SetClass("compact", true);
            scroll->SetInnerRML(R"(
                <div class="guide-requirement-column left">
                  <div class="guide-requirement"><span>Acceso</span><strong>Habla con Dungeon Entrance y elige dificultad</strong></div>
                  <div class="guide-requirement"><span>Resets requeridos</span><strong>0 - 5</strong></div>
                  <div class="guide-requirement"><span>Tu nivel / resets</span><strong>400 / 0</strong></div>
                  <div class="guide-requirement"><span>Entradas restantes hoy</span><strong>3 / 3</strong></div>
                  <div class="guide-requirement"><span>Party</span><strong>Opcional</strong></div>
                </div>
                <div class="guide-requirement-column right">
                  <div class="guide-requirement"><span>Mapa</span><strong>Fortress of Imperial Guardian 4</strong></div>
                  <div class="guide-requirement"><span>Acceso</span><strong>Registro de clanes en Guardsman</strong></div>
                  <div class="guide-requirement"><span>Entrada necesaria</span><strong>Armor of Guardsman +7 x1</strong></div>
                  <div class="guide-requirement"><span>Duración</span><strong>20 min</strong></div>
                </div>)");
            fixture.Refresh(); fixture.Refresh();
            CheckRequirementTextFits(scroll->QuerySelector(".left"));
            CheckRequirementTextFits(scroll->QuerySelector(".right"));
            auto* panel = doc->GetElementById("panel");
            CHECK(panel->GetBox().GetSize().x <= viewport.x);
            CHECK(panel->GetBox().GetSize().y <= viewport.y);
        }
    }
}

TEST_CASE("Paris event guide portrait cards reserve room for names and timers [ui][events]")
{
    UI::Tests::RmlLayoutFixture fixture;
    fixture.Configure({1024, 768}, 100);
    REQUIRE(Rml::LoadFontFace(std::string(MU_FONT_DIR) + "/Cinzel-Variable.ttf"));
    auto* doc = fixture.Load("paris-gold", "paris_events");
    auto* list = doc->QuerySelector(".guide-events");
    REQUIRE(list);
    std::string cards;
    for (const auto* name : {"Golden Invasion", "Mini Invasion 1", "Mini Invasion 2", "Mini Invasion 3", "Mini Invasion 4",
                            "Mini Invasion 5", "Red Dragon Invasion", "White Wizard Invasion", "Karutan Underground"})
        cards += std::string("<button class=\"guide-event\"><div class=\"guide-event-portrait\"></div><span class=\"guide-event-name\">") +
                 name + "</span><strong>Disponible</strong></button>";
    list->SetInnerRML(cards);
    fixture.Refresh(); fixture.Refresh();
    Rml::ElementList rows;
    list->QuerySelectorAll(rows, ".guide-event");
    REQUIRE(rows.size() == 9);
    for (auto* row : rows)
    {
        auto* name = row->QuerySelector(".guide-event-name");
        auto* state = row->QuerySelector("strong");
        CAPTURE(name->GetInnerRML());
        CHECK(Right(row->QuerySelector(".guide-event-portrait")) < name->GetAbsoluteOffset().x);
        CHECK(name->GetScrollWidth() <= name->GetClientWidth() + 1.f);
        CHECK(name->GetScrollHeight() <= name->GetClientHeight() + 1.f);
        CHECK(Right(name) < state->GetAbsoluteOffset().x);
        CHECK(state->GetScrollWidth() <= state->GetClientWidth() + 1.f);
        CHECK(Right(state) <= Right(row) - 1.f);
    }
    CHECK(Bottom(rows.back()) <= Bottom(list));
    CHECK(Bottom(list) <= doc->QuerySelector(".guide-sidebar>.guide-paging")->GetAbsoluteOffset().y + 1.f);
}

