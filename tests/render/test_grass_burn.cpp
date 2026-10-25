#include <doctest.h>
#include "Render/Terrain/GrassBurnState.h"
namespace {bool Grass(int x,int y){return x>=10&&x<=30&&y>=10&&y<=30;}bool NoGrass(int,int){return false;}}
TEST_CASE("Flame scars only grass, expands, regrows and resets on a map change")
{
    Render::Terrain::GrassBurnState state;
    state.Ignite(20,20,1000,Grass);CHECK(state.Amount(20,20,1000)>0);
    CHECK(state.Amount(28,20,1000)==0);CHECK(state.Amount(40,20,1000)==0);
    state.Step(10000,Grass);CHECK(state.Amount(24,20,10000)>0);
    CHECK(state.Amount(20,20,151000)==doctest::Approx(.5f));
    CHECK(state.Amount(20,20,302000)==0);
    state.Reset();CHECK(state.Amount(24,20,10000)==0);
    state.Ignite(20,20,1000,NoGrass);CHECK(state.Amount(20,20,1000)==0);
    state.Ignite(-2,20,1000,Grass);CHECK(state.Amount(0,20,1000)==0);
}
TEST_CASE("Flame simulation caps live fires and expires fronts independently of scars")
{
    Render::Terrain::GrassBurnState state;
    for(int i=0;i<20;++i)state.Ignite(10+(i%7)*3.f,10+(i/7)*3.f,1000,Grass);
    int active=0;for(const auto& fire:state.Fires())active+=fire.active;
    CHECK(active<=state.MaximumFires);CHECK(active>0);
    state.Step(13000,Grass);for(const auto& fire:state.Fires())CHECK_FALSE(fire.active);
    CHECK(state.Amount(10,10,13000)>0);
}
