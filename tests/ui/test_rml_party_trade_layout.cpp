#include <doctest.h>

#include "RmlLayoutFixture.h"



#include "stdafx.h"

#include "Data/GameConfig/GameConfig.h"

#include "UI/RmlBridge/RmlElementBox.h"

#include "UI/RmlBridge/RmlPanelGeometry.h"

#include "UI/RmlBridge/RmlPointer.h"

#include "UI/RmlBridge/RmlScaleInputs.h"

#include "UI/RmlBridge/ThemeFileInterface.h"

#include "UI/Scaling/UITransform.h"



#include <RmlUi/Core.h>



#include <filesystem>

#include <cmath>

#include <string>

#include <utility>



namespace

{

using UI::Tests::ReadFile;

using LayoutFixture = UI::Tests::RmlLayoutFixture;



void OpenSlot(Rml::ElementDocument* workspace, const char* name, Rml::Vector2f size)

{

    Rml::ElementList slots;

    workspace->QuerySelectorAll(slots, ".slot");

    for (auto* slot : slots)

    {

        if (slot->GetAttribute<Rml::String>("data-window", "") != name)

            continue;

        slot->SetClass("open", true);

        slot->SetProperty("width", std::to_string(size.x) + "px");

        slot->SetProperty("height", std::to_string(size.y) + "px");

        return;

    }

    FAIL("workspace slot is missing");

}



void Drawn(Rml::Element* element, Rml::Vector2f& offset, Rml::Vector2f& size)

{

    REQUIRE(element != nullptr);

    REQUIRE(UI::RmlBridge::DrawnBox(*element, Rml::BoxArea::Border, offset, size));

}



void CheckParty(LayoutFixture& fixture, Rml::Element* panel, float top, float bottom, float worldRight, float scale)

{

    Rml::Vector2f offset, size;

    Drawn(panel, offset, size);

    CHECK(offset.y >= top);

    CHECK(offset.y + size.y <= bottom);

    // Project() reconstructs the affine transform from floats; allow a screen pixel at large x.

    constexpr float PixelTolerance = 1.f;

    CHECK(std::abs(offset.x + size.x - (worldRight - 2.f * scale)) <= PixelTolerance);

    for (int member = 0; member < MAX_PARTYS; ++member)

    {

        auto* current = panel->GetChild(member);

        Drawn(current, offset, size);

        CHECK(offset.y >= top);

        CHECK(offset.y + size.y <= bottom);

        fixture.context->ProcessMouseMove(static_cast<int>(offset.x + size.x / 2.f),

                                         static_cast<int>(offset.y + size.y / 2.f), 0);

        CHECK(UI::RmlBridge::IsPointerOver(current));

        // The companion button sits outside the bar; it must still consume UI input at every scale.

        auto* follow = current->QuerySelector(".card-follow");

        REQUIRE(follow != nullptr);

        Rml::Vector2f buttonOffset, buttonSize;

        Drawn(follow, buttonOffset, buttonSize);

        CHECK(buttonOffset.x + buttonSize.x <= offset.x);

        fixture.context->ProcessMouseMove(static_cast<int>(buttonOffset.x + buttonSize.x / 2.f),

                                         static_cast<int>(buttonOffset.y + buttonSize.y / 2.f), 0);

        CHECK(UI::RmlBridge::IsPointerOver(follow));

        CHECK(UI::RmlBridge::IsPointerOver(current->GetOwnerDocument()));



    }

}



void FillTradeHelp(Rml::ElementDocument* trade, const std::string& locale)

{

    const auto path = std::filesystem::path(MU_LOCALIZATION_DIR) / ("Game." + locale + ".resx");

    const std::string resource = ReadFile(path);

    for (const auto& [id, key] : {std::pair{"warning_label", "Warning!"},

                                {"notice_line1", "Notice! Please check out"},

                                {"notice_line2", "the level of the player"},

                                {"notice_line3", "and the items before trading."}})

    {

        const size_t entry = resource.find(std::string("<data name=\"") + key + "\"");

        REQUIRE(entry != std::string::npos);

        const size_t start = resource.find("<value>", entry);

        REQUIRE(start != std::string::npos);

        constexpr size_t ValueTagLength = sizeof("<value>") - 1;

        const size_t end = resource.find("</value>", start);

        REQUIRE(end != std::string::npos);

        trade->GetElementById(id)->SetInnerRML(resource.substr(start + ValueTagLength, end - start - ValueTagLength));

    }

}



void CheckHelpExcludesConfirm(Rml::ElementDocument* trade)

{

    auto* confirm = trade->GetElementById("your_confirm");

    const auto confirmOffset = confirm->GetAbsoluteOffset(Rml::BoxArea::Border);

    const auto confirmSize = confirm->GetBox().GetSize(Rml::BoxArea::Border);

    Rml::ElementList text;

    trade->QuerySelectorAll(text, ".warning-label, .notice-line");

    REQUIRE(text.size() == 4);

    for (auto* span : text)

    for (int index = 0; index < span->GetNumBoxes(); ++index)

    {

        Rml::Vector2f relative;

        const auto size = span->GetBox(index, relative).GetSize(Rml::BoxArea::Border);

        const auto offset = span->GetAbsoluteOffset(Rml::BoxArea::Border) + relative;

        CHECK((offset.x + size.x <= confirmOffset.x || offset.x >= confirmOffset.x + confirmSize.x ||

               offset.y + size.y <= confirmOffset.y || offset.y >= confirmOffset.y + confirmSize.y));

    }

}



void CheckTradeAnchors(Rml::ElementDocument* trade, float scale)

{

    Rml::Vector2f offset, size;

    Drawn(trade->GetElementById("your_confirm"), offset, size);

    CHECK(offset.x == doctest::Approx(146.f * scale));

    CHECK(offset.y == doctest::Approx(186.f * scale));

    CHECK(size.x == doctest::Approx(36.f * scale));

    CHECK(size.y == doctest::Approx(29.f * scale));

    Drawn(trade->GetElementById("partner_grid"), offset, size);

    CHECK(offset.x == doctest::Approx(16.f * scale));

    CHECK(offset.y == doctest::Approx(68.f * scale));

    Drawn(trade->GetElementById("item_grid"), offset, size);

    CHECK(offset.x == doctest::Approx(16.f * scale));

    CHECK(offset.y == doctest::Approx(274.f * scale));

}

} // namespace



TEST_CASE("full party stays below the shell and above the HUD in both themes [ui][party]")

{

    LayoutFixture fixture;

    for (const char* theme : {"legacy", "modern"})

    for (int percent : {75, 100, 125, 150})

    for (Rml::Vector2i viewport : {Rml::Vector2i(800, 600), {1280, 720}, {1920, 1080}, {3440, 1440}})

    {

        CAPTURE(std::string(theme));

        CAPTURE(percent);

        CAPTURE(viewport.x);

        CAPTURE(viewport.y);

        fixture.Configure(viewport, percent);

        const float scale = UI::Scaling::DockRightTransform(viewport.x, viewport.y).scaleX;

        auto* workspace = fixture.Load(theme, "workspace");

        const float hudScale = UI::Scaling::BottomHudScale(viewport.x, viewport.y);

        OpenSlot(workspace, "mu_helper_bar", {207.f * hudScale, 25.f * hudScale});

        OpenSlot(workspace, "top_bar", {256.f * hudScale, 24.f * hudScale});

        OpenSlot(workspace, "main_hud", {640.f * hudScale, 51.f * hudScale});

        fixture.Refresh();

        auto* safe = workspace->GetElementById("safe_area");

        const float top = safe->GetAbsoluteOffset(Rml::BoxArea::Border).y;

        const float bottom = top + safe->GetBox().GetSize(Rml::BoxArea::Border).y;

        auto* root = fixture.context->GetRootElement();

        root->SetProperty("--workspace-top", std::to_string(top) + "px");

        root->SetProperty("--workspace-bottom", std::to_string(bottom) + "px");

        auto* party = fixture.Load(theme, "party_list");

        auto* panel = party->GetElementById("panel");

        auto* card = panel->GetFirstChild();

        REQUIRE(card != nullptr);

        for (int member = 1; member < MAX_PARTYS; ++member)

            panel->AppendChild(card->Clone());

        for (int docks : {0, 1})

        {

            const float worldRight = static_cast<float>(viewport.x) - static_cast<float>(docks) * 190.f * scale;

            root->SetProperty("--world-right", std::to_string(worldRight) + "px");

            fixture.Refresh();

            CheckParty(fixture, panel, top, bottom, worldRight, scale);

        }

        fixture.context->UnloadDocument(party);

        fixture.context->UnloadDocument(workspace);

    }

}



TEST_CASE("legacy trade notice flows around confirmation without moving item grids [ui][trade]")

{

    LayoutFixture fixture;

    for (const std::string locale : {"en", "de", "es", "pl", "ru"})

    for (int percent : {75, 100, 125, 150})

    {

        CAPTURE(locale);

        CAPTURE(percent);

        const Rml::Vector2i Viewport(1024, 768);

        fixture.Configure(Viewport, percent);

        const float scale = UI::Scaling::DockRightTransform(Viewport.x, Viewport.y).scaleX;

        auto* trade = fixture.Load("legacy", "trade");

        UI::RmlBridge::SlotPlacement placement;

        placement.Set(0.f, 0.f, scale);

        placement.Apply(trade, "panel");

        auto* help = trade->GetElementById("trade_help");

        help->SetProperty("font-size", std::to_string(UI::Scaling::NativeTextPixelSize(

            UI::Scaling::FontRole::Normal, Viewport.x, Viewport.y)) + "px");

        FillTradeHelp(trade, locale);

        fixture.Refresh();

        Rml::Vector2f offset, size;

        Drawn(help, offset, size);

        CHECK(offset.y >= 176.f * scale);

        CHECK(offset.y + size.y <= 241.f * scale);

        CheckTradeAnchors(trade, scale);

        CheckHelpExcludesConfirm(trade);

        fixture.context->UnloadDocument(trade);

    }

}



TEST_CASE("party finder keeps its reserved list and controls inside the inventory sized panel [ui][party]")

{

    LayoutFixture fixture;

    for (const std::string theme : {"legacy", "modern", "paris-gold"})

    for (int percent : {75, 100, 125, 150})

    {

        CAPTURE(theme); CAPTURE(percent);

        fixture.Configure({1024, 768}, percent);

        auto* doc = fixture.Load(theme, "party_info");

        doc->QuerySelector(".party-toolbar")->SetClass("hidden", true);

        doc->QuerySelector(".party-list")->SetClass("hidden", true);

        Rml::ElementList tabs;
        doc->QuerySelectorAll(tabs, ".party-tabs button");
        REQUIRE(tabs.size() == 3);
        for (size_t t = 0; t < tabs.size(); ++t)
        {
            Rml::Vector2f offset, size; Drawn(tabs[t], offset, size);
            CHECK(size.x > 0.f); CHECK(size.y > 0.f);
            if (t > 0) CHECK(tabs[t-1]->GetAbsoluteOffset().x + tabs[t-1]->GetBox().GetSize().x <= tabs[t]->GetAbsoluteOffset().x);
        }
        auto* rows = doc->QuerySelector(".finder-rows");

        auto* row = rows->GetFirstChild();

        REQUIRE(row != nullptr);

        for (int n = 1; n < 4; ++n) rows->AppendChild(row->Clone());

        doc->QuerySelector(".finder-random")->SetInnerRML("Unirse aleatoriamente");

        doc->QuerySelector(".settings-create")->SetInnerRML("Crear party pública");

        fixture.Refresh();

        const auto box = [](Rml::Element* el) { return el->GetAbsoluteOffset(); };

        auto* refresh = doc->QuerySelector(".finder-refresh");

        auto* pagination = doc->QuerySelector(".finder-pagination");

        CHECK(box(rows).y + rows->GetBox().GetSize().y <= box(refresh).y);

        CHECK(box(refresh).y + refresh->GetBox().GetSize().y <= box(pagination).y);

        auto* publish = doc->QuerySelector(".settings-create");

        CHECK(box(pagination).y + pagination->GetBox().GetSize().y < 390.f);

        CHECK(box(publish).y + publish->GetBox().GetSize().y < 390.f);

        auto* panel = doc->GetElementById("panel");

        CHECK(panel->GetBox().GetSize().x == doctest::Approx(190.f));

        CHECK(panel->GetBox().GetSize().y == doctest::Approx(429.f));

        // Empty results retain exactly the same pagination/refresh position.

        const float pageY = box(pagination).y;

        rows->SetInnerRML(""); fixture.Refresh();

        CHECK(box(pagination).y == doctest::Approx(pageY));

        fixture.context->UnloadDocument(doc);

    }

}


TEST_CASE("party labels retain native text size when the panel scales [ui][party]")
{
    LayoutFixture fixture;
    fixture.Configure({1024, 768}, 100);
    auto* doc = fixture.Load("paris-gold", "party_info");
    auto* panel = doc->GetElementById("panel");
    auto* label = doc->QuerySelector(".party-tabs .party-button-label");
    REQUIRE(label != nullptr);
    label->SetProperty("font-family", "DejaVu Sans");
    label->SetInnerRML("Miembros");
    panel->SetProperty("--text-px", "13px");
    float firstTextHeight = 0.f, firstButtonWidth = 0.f;
    for (float scale : {1.f, 2.f})
    {
        UI::RmlBridge::SlotPlacement placement;
        placement.Set(0.f, 0.f, scale);
        placement.Apply(doc, "panel");
        fixture.Refresh();
        Rml::Vector2f offset, size;
        Drawn(label, offset, size);
        const float textHeight = size.y;
        Drawn(label->GetParentNode(), offset, size);
        if (scale == 1.f) { firstTextHeight = textHeight; firstButtonWidth = size.x; }
        else
        {
            CHECK(textHeight == doctest::Approx(firstTextHeight).epsilon(0.01));
            CHECK(size.x == doctest::Approx(firstButtonWidth * scale));
        }
    }
    auto* rows = doc->QuerySelector(".finder-rows");
    auto* empty = doc->QuerySelector(".finder-empty");
    REQUIRE(rows != nullptr); REQUIRE(empty != nullptr);
    CHECK(rows->GetNextSibling() == empty);
    fixture.context->UnloadDocument(doc);
}

TEST_CASE("party vitals and stacked setting checkboxes stay inside the panel [ui][party]")
{
    LayoutFixture fixture;
    fixture.Configure({1024,768},100);
    for(const char* theme : {"legacy","modern","paris-gold"})
    {
        auto* doc=fixture.Load(theme,"party_info");
        Rml::Element* previous=nullptr;
        for(const char* selector : {".settings-privacy",".settings-follow",".settings-auto",".settings-buffs",".settings-summon"})
        {
            auto* option=doc->QuerySelector(selector);
            REQUIRE(option != nullptr);
            REQUIRE(option->QuerySelector(".party-check-square") != nullptr);
            CHECK(option->GetBox().GetSize().x <= 166.f);
            if(previous) CHECK(previous->GetAbsoluteOffset().y+previous->GetBox().GetSize().y <= option->GetAbsoluteOffset().y);
            previous=option;
        }
        auto* hp=doc->QuerySelector(".party-hp-bar");
        auto* mana=doc->QuerySelector(".party-mana-bar");
        REQUIRE(hp != nullptr); REQUIRE(mana != nullptr);
        CHECK(hp->GetAbsoluteOffset().x+hp->GetBox().GetSize().x <= mana->GetAbsoluteOffset().x);
        CHECK(mana->GetAbsoluteOffset().x+mana->GetBox().GetSize().x <= doc->GetElementById("panel")->GetAbsoluteOffset().x+190.f);
        fixture.context->UnloadDocument(doc);
    }
}

TEST_CASE("party checkbox hidden marks and hidden action rows stay hidden [ui][party]")
{
    LayoutFixture fixture;
    fixture.Configure({1024,768},100);
    auto* doc=fixture.Load("paris-gold","party_info");
    auto* row=doc->QuerySelector(".party-setting-check");
    REQUIRE(row != nullptr);
    auto* tick=row->QuerySelector(".party-check-tick");
    REQUIRE(tick != nullptr);
    tick->SetClass("hidden",true);
    fixture.Refresh();
    CHECK(tick->GetComputedValues().display() == Rml::Style::Display::None);
    tick->SetClass("hidden",false);
    row->SetClass("hidden",true);
    fixture.Refresh();
    CHECK(row->GetComputedValues().display() == Rml::Style::Display::None);
    row->SetClass("hidden",false);
    fixture.Refresh();
    CHECK(tick->GetComputedValues().display() != Rml::Style::Display::None);
    fixture.context->UnloadDocument(doc);
}

TEST_CASE("party mana fill has visible geometry at full mana [ui][party]")
{
    LayoutFixture fixture;
    fixture.Configure({1024,768},100);
    auto* doc=fixture.Load("paris-gold","party_info");
    auto* fill=doc->QuerySelector(".party-mana-fill");
    REQUIRE(fill != nullptr);
    fill->SetProperty("width","100%");
    fixture.Refresh();
    CHECK(fill->GetBox().GetSize().x > 70.f);
    CHECK(fill->GetBox().GetSize().y == doctest::Approx(4.f));
    fixture.context->UnloadDocument(doc);
}
