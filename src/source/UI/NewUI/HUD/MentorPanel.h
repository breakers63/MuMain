#pragma once
namespace UI::Mentor { void Load(); void Unload(); void Toggle(); void Close(); void Render(); bool Mouse(); bool IsOpen(); bool HasInputFocus(); }
namespace UI::Mentor { void Render3D(); }

namespace UI::Mentor { void RenderActiveBuff(int x,int y); }
