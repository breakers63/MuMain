#pragma once
#include <RmlUi/Core/Types.h>
struct TargetHealthModel
{
    Rml::String name;
    Rml::String ranking;
    Rml::String remaining;
    float percent = 100.f;
};
