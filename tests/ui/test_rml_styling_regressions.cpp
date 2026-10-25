#include "stdafx.h"
#include <doctest.h>

#include "RmlLayoutFixture.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Models/ItemDyeAppearance.h"
#include "../../src/ThirdParty/RmlUi/Source/Core/DecoratorTiled.h"
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/TextShapingContext.h>

TEST_CASE("range thumb follows a bound size after initial layout and remains movable [ui][styling]")
{
    UI::Tests::RmlLayoutFixture fixture;
    float fraction = 1.f;
    int page = 0;
    auto model = fixture.context->CreateDataModel("dynamic_thumb");
    REQUIRE(model);
    model.Bind("fraction", &fraction);
    model.Bind("page", &page);
    auto* document = fixture.LoadMarkup(R"(<rml><head><style>
        input { width:496px; height:10px; }
        slidertrack { height:5px; }
        sliderbar { width:var(--thumb-width,24%); height:5px; }
        sliderarrowdec, sliderarrowinc { width:0; height:0; }
        </style></head><body data-model="dynamic_thumb">
        <input id="range" type="range" min="0" max="19" step="1" data-value="page"
            data-style---thumb-width="(fraction * 100) + '%'"/></body></rml>)");
    REQUIRE(document);
    auto* range = document->GetElementById("range");
    REQUIRE(range);
    Rml::Element* thumb = nullptr;
    for (int index = 0; index < range->GetNumChildren(true); ++index)
        if (range->GetChild(index)->GetTagName() == "sliderbar") thumb = range->GetChild(index);
    REQUIRE(thumb);
    CHECK(thumb->GetBox().GetSize().x == doctest::Approx(496.f));
    auto handle = model.GetModelHandle();
    for (float share : {.24f, .5f})
    {
        fraction = share;
        handle.DirtyVariable("fraction");
        fixture.Refresh();
        fixture.Refresh();
        CHECK(range->GetBox().GetSize().x == doctest::Approx(496.f));
        CHECK(thumb->GetBox().GetSize().x == doctest::Approx(496.f * share));
    }
    REQUIRE(range->Focus());
    fixture.context->ProcessKeyDown(Rml::Input::KI_RIGHT, 0);
    fixture.Refresh();
    CHECK(page == 1);
    CHECK(thumb->GetRelativeOffset().x > 0.f);
    const auto start = thumb->GetAbsoluteOffset() + thumb->GetBox().GetSize() * .5f;
    const auto end = range->GetAbsoluteOffset() + Rml::Vector2f{range->GetBox().GetSize().x - 1.f, 3.f};
    fixture.context->ProcessMouseMove(static_cast<int>(start.x), static_cast<int>(start.y), 0);
    fixture.context->ProcessMouseButtonDown(0, 0);
    fixture.context->ProcessMouseMove(static_cast<int>(end.x), static_cast<int>(end.y), 0);
    fixture.context->ProcessMouseButtonUp(0, 0);
    fixture.Refresh();
    CHECK(page == 19);
}

namespace
{
Rml::DecoratorTiled::Tile AtlasTile(Rml::DecoratorTiled::TileFitMode mode)
{
    Rml::DecoratorTiled::Tile tile;
    tile.fit_mode = mode;
    tile.size = {8.f, 6.f};
    tile.tile_data_calculated = true;
    tile.tile_data.size = tile.size;
    tile.tile_data.texcoords[0] = {0.25f, 0.25f};
    tile.tile_data.texcoords[1] = {0.5f, 0.5f};
    tile.align[0] = {Rml::Style::LengthPercentage::Percentage, 0.f};
    tile.align[1] = {Rml::Style::LengthPercentage::Percentage, 0.f};
    return tile;
}
}

TEST_CASE("atlas repetition clips partial tiles without sampling adjacent sprites [ui][styling]")
{
    UI::Tests::RmlLayoutFixture fixture;
    auto* document = fixture.LoadMarkup(R"(<rml><body><div id="panel"></div></body></rml>)");
    REQUIRE(document);
    auto* panel = document->GetElementById("panel");
    for (auto mode : {Rml::DecoratorTiled::REPEAT, Rml::DecoratorTiled::REPEAT_X, Rml::DecoratorTiled::REPEAT_Y})
    {
        CAPTURE(mode);
        Rml::Mesh mesh;
        auto tile = AtlasTile(mode);
        tile.GenerateGeometry(mesh, panel->GetComputedValues(), {}, {19.f, 14.f}, {8.f, 6.f});
        const size_t quads = mode == Rml::DecoratorTiled::REPEAT ? 9 : 3;
        CHECK(mesh.vertices.size() == quads * 4);
        CHECK(mesh.indices.size() == quads * 6);
        for (const auto& vertex : mesh.vertices)
        {
            CHECK(vertex.position.x >= 0.f);
            CHECK(vertex.position.x <= 19.f);
            CHECK(vertex.position.y >= 0.f);
            CHECK(vertex.position.y <= 14.f);
            CHECK(vertex.tex_coord.x >= 0.25f);
            CHECK(vertex.tex_coord.x <= 0.5f);
            CHECK(vertex.tex_coord.y >= 0.25f);
            CHECK(vertex.tex_coord.y <= 0.5f);
        }
        CHECK(mesh.vertices.back().position.y == (mode == Rml::DecoratorTiled::REPEAT_X ? 6.f : 14.f));
    }
}

TEST_CASE("fractional letter spacing accumulates consistently in measurement and drawing [ui][styling]")
{
    UI::Tests::RmlLayoutFixture fixture;
    auto* fonts = Rml::GetFontEngineInterface();
    const auto face = fonts->GetFontFaceHandle("liberation sans", Rml::Style::FontStyle::Normal,
                                              Rml::Style::FontWeight::Normal, 16);
    REQUIRE(face);
    const Rml::String text = "iiiiiiii";
    const Rml::String language = "en";
    Rml::TextShapingContext shaping{language};
    const int baseline = fonts->GetStringWidth(face, text, shaping);
    for (float spacing : {0.25f, -0.25f, 2.f})
    {
        CAPTURE(spacing);
        shaping.letter_spacing = spacing;
        const int measured = fonts->GetStringWidth(face, text, shaping);
        CHECK(measured == baseline + static_cast<int>(spacing * text.size()));
        Rml::TexturedMeshList meshes;
        const int drawn = fonts->GenerateString(fixture.context->GetRenderManager(), face, 0, text, {},
                                                Rml::ColourbPremultiplied(255, 255, 255, 255), 1.f, shaping, meshes);
        CHECK(drawn == measured);
    }
}

TEST_CASE("chat fill respects theme insets independently of modal backdrop styles [ui][styling]")
{
    UI::Tests::RmlLayoutFixture fixture;
    for (const std::string theme : {"legacy", "modern"})
    {
        CAPTURE(theme);
        auto* document = fixture.Load(theme, "chat_log");
        REQUIRE(document);
        auto* panel = document->GetElementById("panel");
        auto* fill = document->GetElementById("chat_backdrop");
        REQUIRE(panel);
        REQUIRE(fill);
        panel->SetClass("framed", true);
        panel->SetProperty("width", "281px");
        panel->SetProperty("height", "100px");
        fill->SetProperty("left", "10px");
        fill->SetProperty("right", "10px");
        fill->SetProperty("top", "5px");
        fill->SetProperty("bottom", "5px");
        fixture.Refresh();
        CHECK(fill->GetBox().GetSize(Rml::BoxArea::Border).x == doctest::Approx(261.f));
        CHECK(fill->GetBox().GetSize(Rml::BoxArea::Border).y == doctest::Approx(90.f));
        document->Close();
    }
}

TEST_CASE("conditional disabled bindings keep editable fields and ranges live [ui][styling]")
{
    UI::Tests::RmlLayoutFixture fixture;
    bool busy = false;
    Rml::String amount = "0";
    auto model = fixture.context->CreateDataModel("conditional_disabled");
    REQUIRE(model);
    model.Bind("busy", &busy);
    model.Bind("amount", &amount);
    auto* document = fixture.LoadMarkup(R"(<rml><body data-model="conditional_disabled">
        <input id="amount" type="text" data-value="amount" data-attrif-disabled="busy"/>
        <input id="range" type="range" data-attrif-disabled="busy"/>
        </body></rml>)");
    REQUIRE(document);
    auto* input = document->GetElementById("amount");
    auto* range = document->GetElementById("range");
    REQUIRE(input);
    REQUIRE(range);
    CHECK_FALSE(input->HasAttribute("disabled"));
    CHECK_FALSE(range->HasAttribute("disabled"));
    busy = true;
    model.GetModelHandle().DirtyVariable("busy");
    fixture.Refresh();
    CHECK(input->HasAttribute("disabled"));
    CHECK(range->HasAttribute("disabled"));
    busy = false;
    model.GetModelHandle().DirtyVariable("busy");
    fixture.Refresh();
    CHECK_FALSE(input->HasAttribute("disabled"));
    CHECK_FALSE(range->HasAttribute("disabled"));
}

TEST_CASE("RGB keeps its controls inside the frame and leaves the inventory interactive [ui][rgb]")
{
    for (const auto* theme : {"legacy", "modern", "paris-gold"})
    {
        CAPTURE(theme);
        UI::Tests::RmlLayoutFixture fixture;
        fixture.Configure({1024,768},100);
        auto* inventory = fixture.LoadMarkup(R"(<rml><head><style>
            body {width:100%;height:100%;margin:0;pointer-events:none;} #inventory {position:absolute;left:804px;top:0;width:220px;height:768px;pointer-events:auto;}
            </style></head><body><div id="inventory"></div></body></rml>)");
        auto* document = fixture.Load(theme,"paris_item_dye");
        auto* panel = document->GetElementById("panel");
        REQUIRE(panel);
        const auto origin = panel->GetAbsoluteOffset();
        const auto size = panel->GetBox().GetSize(Rml::BoxArea::Border);
        CHECK(origin.y >= 0.f);
        CHECK(origin.y + size.y <= 768.f);
        for (int column = 0; column < 5; ++column)
        {
            auto* slot = document->GetElementById("dye_piece_" + std::to_string(column));
            REQUIRE(slot);
            CHECK(slot->GetAbsoluteOffset().x >= origin.x);
            CHECK(slot->GetAbsoluteOffset().x + slot->GetBox().GetSize().x <= origin.x + size.x);
        }
        auto* picker = document->GetElementById("dye_sv");
        REQUIRE(picker);
        CHECK(picker->GetBox().GetSize().y > 100.f);
        auto* white = document->QuerySelector(".dye-white");
        REQUIRE(white);
        CHECK(white->GetAbsoluteOffset().x == doctest::Approx(picker->GetAbsoluteOffset().x));
        CHECK(white->GetAbsoluteOffset().y == doctest::Approx(picker->GetAbsoluteOffset().y));
        CHECK(white->GetBox().GetSize().x <= picker->GetBox().GetSize().x + 1.f);
        CHECK(white->GetBox().GetSize().y <= picker->GetBox().GetSize().y + 1.f);
        auto* hue = document->GetElementById("dye_hue");
        REQUIRE(hue);
        CHECK(hue->GetBox().GetSize().x > 150.f);
        CHECK(hue->GetBox().GetSize().y > 10.f);
        REQUIRE(hue->GetProperty("decorator"));
        CHECK(hue->GetProperty("decorator")->ToString().find("linear-gradient") != Rml::String::npos);
        for (const auto* sectionName : {".dye-colors", ".dye-materials"})
        {
            auto* section = document->QuerySelector(sectionName);
            REQUIRE(section);
            CHECK(section->GetBox().GetSize().x > 180.f);
        }
        Rml::ElementList paletteRows;
        document->GetElementsByClassName(paletteRows,"dye-swatch-row");
        REQUIRE(paletteRows.size() == 9);
        for (auto* row : paletteRows)
        {
            REQUIRE(row->GetNumChildren() == 14);
            const float rowBottom = row->GetAbsoluteOffset().y + row->GetBox().GetSize().y;
            for (int column = 0; column < 14; ++column)
            {
                auto* cell = row->GetChild(column);
                CHECK(cell->GetBox().GetSize().x > 8.f);
                CHECK(cell->GetBox().GetSize().y > 4.f);
                CHECK(cell->GetAbsoluteOffset().y + cell->GetBox().GetSize().y <= rowBottom + 1.f);
            }
        }
        Rml::ElementList hints;
        document->GetElementsByClassName(hints,"dye-hint");
        REQUIRE(hints.size() == 5);
        for (auto* hint : hints)
        {
            CHECK(hint->GetBox().GetSize().y > 30.f);
            REQUIRE(hint->GetProperty("decorator"));
            CHECK(hint->GetProperty("decorator")->ToString() != "none");
        }
        for (const auto* id : {"dye_original", "dye_new"})
        {
            auto* image = document->GetElementById(id);
            REQUIRE(image);
            image->SetProperty("visibility","hidden");
            fixture.Refresh();
            auto* frame = image->GetParentNode();
            REQUIRE(frame);
            CHECK(frame->IsVisible(true));
            CHECK(frame->GetBox().GetSize().x > 60.f);
            CHECK(frame->GetBox().GetSize().y > 60.f);
        }
        for (int column = 0; column < 5; ++column)
        {
            auto* image = document->GetElementById("dye_piece_" + std::to_string(column));
            image->SetProperty("visibility","hidden");
            fixture.Refresh();
            CHECK(image->GetParentNode()->IsVisible(true));
        }
        auto* footer = document->QuerySelector(".dye-status");
        REQUIRE(footer);
        CHECK(footer->GetAbsoluteOffset().y + footer->GetBox().GetSize().y <= origin.y + size.y);
        fixture.context->ProcessMouseMove(950,400,0);
        fixture.Refresh();
        REQUIRE(fixture.context->GetHoverElement());
        CHECK(fixture.context->GetHoverElement()->GetOwnerDocument() == inventory);
    }
}

TEST_CASE("Saved item tint resolves for the correct character and preview scope [render][rgb]")
{
    auto character = std::make_unique<CHARACTER>();
    character->Key = 23;
    auto previous = std::move(Render::ItemDye::World);
    Render::ItemDye::World.clear();
    constexpr int model = MODEL_ARMOR + 16;
    Render::ItemDye::Equipped equipped;
    equipped.Character = character.get(); equipped.Key = character->Key;
    equipped.Colors[model] = 0x33FFFF;
    Render::ItemDye::World[&character->Object] = std::move(equipped);
    CHECK(Render::ItemDye::Resolve(&character->Object,model) == 0x33FFFF);
    CHECK(Render::ItemDye::Resolve(&character->Object,model + 1) == -1);
    {
        Render::ItemDye::PreviewScope preview(0xFF0000);
        CHECK(Render::ItemDye::Resolve(&character->Object,model) == 0xFF0000);
    }
    CHECK(Render::ItemDye::Resolve(&character->Object,model) == 0x33FFFF);
    ++character->Key;
    CHECK(Render::ItemDye::Resolve(&character->Object,model) == -1);
    Render::ItemDye::World = std::move(previous);
}

TEST_CASE("Short skill rows stay centered and Dark Spirit has no separate upper row [ui][skills]")
{
    UI::Tests::RmlLayoutFixture fixture;
    fixture.Configure({1024,768},100);
    auto* document=fixture.Load("paris-gold","main_frame");
    auto* grid=document->GetElementById("skill_grid");
    REQUIRE(grid);
    grid->SetClass("hidden",false);
    auto* list=document->GetElementById("skill_list");
    REQUIRE(list);
    list->SetClass("hidden",false);
    auto prototype=grid->GetChild(0)->Clone();
    while(grid->GetNumChildren())grid->RemoveChild(grid->GetChild(0));
    for(int i=0;i<7;++i)grid->AppendChild(prototype->Clone());
    fixture.Refresh();fixture.Refresh();
    auto* first=grid->GetChild(0);auto* last=grid->GetChild(6);
    CHECK(first->GetAbsoluteOffset().y==doctest::Approx(last->GetAbsoluteOffset().y));
    const float rowCenter=(first->GetAbsoluteOffset().x+last->GetAbsoluteOffset().x+last->GetBox().GetSize().x)*.5f;
    const float gridCenter=grid->GetAbsoluteOffset().x+grid->GetBox().GetSize().x*.5f;
    CHECK(rowCenter==doctest::Approx(gridCenter));
    auto* petRow=document->GetElementById("pet_skill_row");
    REQUIRE(petRow);
    CHECK_FALSE(petRow->IsVisible(true));
}

