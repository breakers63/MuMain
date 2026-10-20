#pragma once
namespace UI::Hud::Status
{
    inline constexpr int MenuX = 20, MenuY = 434, MenuWidth = 58, MenuHeight = 12;
    bool MenuHit();
    void RenderMenu();
    void Render();
    bool Mouse();
}
