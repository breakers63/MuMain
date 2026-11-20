#include "stdafx.h"
#include <doctest.h>
#include "RmlLayoutFixture.h"
#include <RmlUi/Core/ElementUtilities.h>
#include <cmath>

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
