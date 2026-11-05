#pragma once
#include <array>
#include <optional>

class BMD;
class OBJECT;

namespace Render::Items::Preview
{
// Fits an isolated item to the image rectangle supplied by its RmlUi camera target.
class ScopedFit
{
public:
    ScopedFit(float x, float y, float width, float height);
    ~ScopedFit();
    ScopedFit(const ScopedFit&) = delete;
    ScopedFit& operator=(const ScopedFit&) = delete;
private:
    std::optional<std::array<float, 4>> m_previous;
};

void FitPosedModel(BMD* model, OBJECT* object);
}
