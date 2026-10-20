#pragma once
namespace UI::GmEvents
{
void Load();
void Unload();
void Toggle();
void Close();
void Render();
bool Mouse();
bool IsOpen();
bool CanOpen();
bool HasInputFocus();
} // namespace UI::GmEvents
