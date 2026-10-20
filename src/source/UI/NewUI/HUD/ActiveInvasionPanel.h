#pragma once
#include <string_view>
namespace UI::Invasions
{
    void Load(); void Unload(); void Close(); void Reset();
    bool ConsumeMetadata(std::wstring_view text);
    bool Mouse(); void Render(); void Render3D(); bool IsOpen();
}
