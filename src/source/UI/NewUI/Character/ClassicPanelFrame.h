#pragma once
namespace UI::Paris
{
    void LoadGoldTheme();
    void UnloadGoldTheme();
    void GoldBorder(int x, int y, int width, int height);
    void Color(float red, float green, float blue, float alpha);
    void Frame(int x, int y, int width = 190, int height = 429);
    void Clip(int x, int y, int width, int height);
    void ResetClip();
    void ConsumeMouse();
}
