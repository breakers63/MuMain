#pragma once
#include <SDL3/SDL_gpu.h>
#include "Data/GameConfig/HdGraphicsSettings.h"

namespace Render::Hd
{
bool Available();
class SdlGpuHdPostProcessor
{
public:
    bool Initialize(SDL_GPUDevice* device, SDL_GPUTextureFormat format, const char* driver);
    void Release();
    SDL_GPUTexture* SceneTarget(Uint32 width, Uint32 height);
    bool Apply(SDL_GPUCommandBuffer* commands, SDL_GPUTexture* output, const Data::Graphics::HdGraphicsSettings& settings);
private:
    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_format = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    SDL_GPUTexture* m_scene = nullptr;
    Uint32 m_width = 0, m_height = 0;
};
}
