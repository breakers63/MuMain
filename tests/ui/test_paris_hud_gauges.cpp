#include "stdafx.h"
#include <doctest.h>
#include "RmlLayoutFixture.h"
#include <RmlUi/Core/ElementUtilities.h>
#include <cmath>
#include <array>
#include <regex>

namespace
{
// Retain the real artwork bindings, without requiring unrelated networked HUD models.
std::string BoundArtworkMarkup()
{
    auto markup=UI::Tests::AssetMarkup("paris-gold","main_frame");
    const auto original=UI::Tests::ReadFile(std::filesystem::path(MU_RMLUI_DIR)/"main_frame.rml");
    const std::regex artwork(R"art(<div class="(?:gauge-art|exp-art)"[^>]*>)art");
    size_t cursor=0;
    for(auto it=std::sregex_iterator(original.begin(),original.end(),artwork);it!=std::sregex_iterator();++it)
    {
        std::smatch stripped;
        const auto remaining=markup.substr(cursor);
        REQUIRE(std::regex_search(remaining,stripped,artwork));
        const auto position=cursor+static_cast<size_t>(stripped.position());
        markup.replace(position,static_cast<size_t>(stripped.length()),it->str());
        cursor=position+it->str().size();
    }
    const auto body=markup.find("<body");
    REQUIRE(body!=std::string::npos);
    markup.insert(body+5," data-model=\"main_frame\"");
    return markup;
}
}

TEST_CASE("Paris dragon empty artwork follows live resource bindings [ui][hud]")
{
    UI::Tests::RmlLayoutFixture fixture;
    fixture.Configure({1024,768},100);
    auto constructor=fixture.context->CreateDataModel("main_frame");
    REQUIRE(constructor);
    constexpr std::array names={"hp_fraction","mp_fraction","sd_fraction","ag_fraction","exp_level_fraction"};
    constexpr std::array selectors={"#bar_hp .gauge-art","#bar_mp .gauge-art","#bar_sd .gauge-art","#bar_ag .gauge-art","#exp_level_fill .exp-art"};
    std::array<float,names.size()> values={1.f,1.f,1.f,1.f,1.f};
    for(size_t i=0;i<names.size();++i) REQUIRE(constructor.Bind(names[i],&values[i]));
    auto handle=constructor.GetModelHandle();
    auto* document=fixture.LoadMarkup(BoundArtworkMarkup());
    // Include depletion, initial empty state and recovery, independently for each gauge.
    for(size_t i=0;i<names.size();++i)
    {
        auto* art=document->QuerySelector(selectors[i]);
        REQUIRE(art);
        for(const float value:{1.f,.25f,0.f,0.f,.5f,1.f})
        {
            CAPTURE(names[i]); CAPTURE(value);
            values[i]=value;
            handle.DirtyVariable(names[i]);
            fixture.Refresh();fixture.Refresh();
            CHECK(art->IsVisible(true)==(value>0.f));
            CHECK(art->GetParentNode()->IsVisible(true));
        }
    }
}

TEST_CASE("Paris dragon resources clip fixed artwork to the current value [ui][hud]")
{
    UI::Tests::RmlLayoutFixture fixture;
    for (const auto viewport : {Rml::Vector2i{800,600}, {1024,768}, {1280,720}, {1920,1080}, {2560,1440}, {3440,1440}, {3840,2160}})
    {
        for (const int scale : {75,100,125,150})
        {
            fixture.Configure(viewport,scale);
            auto* document=fixture.Load("paris-gold","main_frame");
            for (const char* id : {"bar_hp","bar_mp","bar_sd","bar_ag"})
            {
                auto* gauge=document->GetElementById(id);
                REQUIRE(gauge);
                auto* fill=gauge->QuerySelector(".gauge-fill-vertical");
                auto* art=gauge->QuerySelector(".gauge-art");
                REQUIRE(fill);
                REQUIRE(art);
                for (const float ratio : {1.f,.5f,.25f,0.f})
                {
                    CAPTURE(viewport.x); CAPTURE(viewport.y); CAPTURE(scale); CAPTURE(id); CAPTURE(ratio);
                    fill->SetProperty("height",std::to_string(ratio*100.f)+"%");
                    fixture.Refresh();fixture.Refresh();
                    Rml::Rectanglei clip;
                    REQUIRE(Rml::ElementUtilities::GetClippingRegion(art,clip));
                    const auto gaugeHeight=gauge->GetBox().GetSize().y;
                    CHECK(std::abs(clip.Height()-gaugeHeight*ratio)<=2.f);
                    CHECK(art->GetBox().GetSize().y==doctest::Approx(gaugeHeight));
                }
            }
            auto* track=document->GetElementById("exp_track");
            auto* fill=document->GetElementById("exp_level_fill");
            auto* art=fill->QuerySelector(".exp-art");
            REQUIRE(track); REQUIRE(art);
            for (const float ratio : {1.f,.6f,.1f,0.f})
            {
                fill->SetProperty("width",std::to_string(ratio*100.f)+"%");
                fixture.Refresh();fixture.Refresh();
                Rml::Rectanglei clip;
                REQUIRE(Rml::ElementUtilities::GetClippingRegion(art,clip));
                CHECK(std::abs(clip.Width()-track->GetBox().GetSize().x*ratio)<=2.f);
            }
            document->Close();fixture.Refresh();
        }
    }
}
