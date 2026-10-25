#pragma once
#include "UI/RmlBridge/RmlRenderTarget.h"
#include <memory>
#include <array>
class CHARACTER;
namespace Rml { class Element; }
namespace Render::MonsterPreviews
{
class MonsterPreviewTarget
{
public:
    MonsterPreviewTarget();
    ~MonsterPreviewTarget();
    void Sync(Rml::Element* image, int monster);
    void Disable();
private:
    bool Build(int monster);
    void MeasureModel();
    void Clear();
    void Draw(std::uint32_t width, std::uint32_t height);
    int m_monster = -1;
    std::array<float, 3> m_center{};
    std::array<float, 3> m_halfExtents{50.f, 50.f, 50.f};
    std::unique_ptr<CHARACTER> m_character;
    UI::RmlBridge::RenderTarget m_target;
    std::uint64_t m_previewTick = 0;
};
}
