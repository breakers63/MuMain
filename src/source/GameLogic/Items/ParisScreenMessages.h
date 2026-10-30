#pragma once
namespace UI::ScreenMessages {
inline bool hidden = false;
inline bool IsHidden() { return hidden; }
inline void Toggle() { hidden = !hidden; }
}
