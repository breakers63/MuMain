#pragma once
namespace UI::Inventory::EquipmentHints
{
    void Load();
    void Unload();
    int HintIndex(int slot);
    void RenderSlotFrame(int x, int y, int width, int height);
    void RenderStorageFrames(int gridX, int gridY, int zenX, int zenY);
    void Render(int slot, int x, int y, int width, int height);
}
