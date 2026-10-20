#include "SdlGpuHdPostProcessor.h"
#include "Core/Utilities/Log/MuLogger.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <string>
#include <vector>

namespace mu
{
std::string GetShaderBlobPath(const char* driver, const char* stage, const char* name);
}
namespace Render::Hd
{
namespace
{
bool available = false;
SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* driver, const char* name, bool fragment)
{
    const auto path = mu::GetShaderBlobPath(driver, fragment ? "frag" : "vert", name);
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input || input.tellg() <= 0) return nullptr;
    std::vector<Uint8> bytes(static_cast<std::size_t>(input.tellg()));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) return nullptr;
    SDL_GPUShaderCreateInfo info{};
    info.code = bytes.data();
    info.code_size = bytes.size();
    const std::string backend = driver ? driver : "vulkan";
    info.format = backend == "direct3d12" ? SDL_GPU_SHADERFORMAT_DXIL
        : backend == "metal" ? SDL_GPU_SHADERFORMAT_MSL : SDL_GPU_SHADERFORMAT_SPIRV;
    info.entrypoint = info.format == SDL_GPU_SHADERFORMAT_MSL ? "main0" : "main";
    info.stage = fragment ? SDL_GPU_SHADERSTAGE_FRAGMENT : SDL_GPU_SHADERSTAGE_VERTEX;
    info.num_samplers = fragment ? 1 : 0;
    info.num_uniform_buffers = fragment ? 1 : 0;
    return SDL_CreateGPUShader(device, &info);
}
SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device, SDL_GPUTextureFormat format, const char* driver)
{
    SDL_GPUShader* vertex = LoadShader(device, driver, "hd_fullscreen", false);
    SDL_GPUShader* fragment = LoadShader(device, driver, "hd_post", true);
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    if (vertex && fragment)
    {
        SDL_GPUColorTargetDescription target{};
        target.format = format;
        target.blend_state.src_color_blendfactor = target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        target.blend_state.dst_color_blendfactor = target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        target.blend_state.color_blend_op = target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        target.blend_state.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G
            | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vertex;
        info.fragment_shader = fragment;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        SDL_GPUStencilOpState stencil{};
        stencil.fail_op = stencil.pass_op = stencil.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
        stencil.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.depth_stencil_state.front_stencil_state = info.depth_stencil_state.back_stencil_state = stencil;
        info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.target_info.num_color_targets = 1;
        info.target_info.color_target_descriptions = &target;
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    }
    if (vertex) SDL_ReleaseGPUShader(device, vertex);
    if (fragment) SDL_ReleaseGPUShader(device, fragment);
    return pipeline;
}
}
bool Available() { return available; }
bool SdlGpuHdPostProcessor::Initialize(SDL_GPUDevice* device, SDL_GPUTextureFormat format, const char* driver)
{
    Release();
    m_device = device;
    m_format = format;
    m_pipeline = CreatePipeline(device, format, driver);
    SDL_GPUSamplerCreateInfo sampler{};
    sampler.min_filter = sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler.address_mode_u = sampler.address_mode_v = sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    m_sampler = SDL_CreateGPUSampler(device, &sampler);
    available = m_pipeline && m_sampler;
    if (!available)
    {
        mu::log::Get("render")->warn("Shaders HD unavailable: {}", SDL_GetError());
        Release();
    }
    else mu::log::Get("render")->info("Shaders HD ready (color grading, sharpen, bloom)");
    return available;
}
void SdlGpuHdPostProcessor::Release()
{
    available = false;
    if (m_device)
    {
        if (m_scene) SDL_ReleaseGPUTexture(m_device, m_scene);
        if (m_sampler) SDL_ReleaseGPUSampler(m_device, m_sampler);
        if (m_pipeline) SDL_ReleaseGPUGraphicsPipeline(m_device, m_pipeline);
    }
    m_scene = nullptr;
    m_sampler = nullptr;
    m_pipeline = nullptr;
    m_device = nullptr;
    m_width = m_height = 0;
}
SDL_GPUTexture* SdlGpuHdPostProcessor::SceneTarget(Uint32 width, Uint32 height)
{
    if (!m_pipeline || !width || !height) return nullptr;
    if (m_scene && m_width == width && m_height == height) return m_scene;
    if (m_scene) SDL_ReleaseGPUTexture(m_device, m_scene);
    SDL_GPUTextureCreateInfo texture{};
    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.format = m_format;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture.width = width;
    texture.height = height;
    texture.layer_count_or_depth = texture.num_levels = 1;
    texture.sample_count = SDL_GPU_SAMPLECOUNT_1;
    m_scene = SDL_CreateGPUTexture(m_device, &texture);
    m_width = width;
    m_height = height;
    return m_scene;
}
bool SdlGpuHdPostProcessor::Apply(SDL_GPUCommandBuffer* commands, SDL_GPUTexture* output,
    const Data::Graphics::HdGraphicsSettings& settings)
{
    if (!m_scene || !m_pipeline || !output || output == m_scene) return false;
    SDL_GPUColorTargetInfo target{};
    target.texture = output;
    target.load_op = SDL_GPU_LOADOP_DONT_CARE;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
    if (!pass) return false;
    SDL_BindGPUGraphicsPipeline(pass, m_pipeline);
    SDL_GPUViewport viewport{0.f, 0.f, static_cast<float>(m_width), static_cast<float>(m_height), 0.f, 1.f};
    SDL_SetGPUViewport(pass, &viewport);
    SDL_Rect scissor{0, 0, static_cast<int>(m_width), static_cast<int>(m_height)};
    SDL_SetGPUScissor(pass, &scissor);
    SDL_GPUTextureSamplerBinding binding{m_scene, m_sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    const float uniforms[]{1.f / m_width, 1.f / m_height, settings.Sharpen ? settings.Sharpness / 100.f : 0.f,
        settings.Bloom ? settings.BloomStrength / 100.f : 0.f, settings.Exposure / 100.f,
        settings.ColorGrading ? 1.f : 0.f, 0.f, 0.f};
    SDL_PushGPUFragmentUniformData(commands, 0, uniforms, sizeof(uniforms));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return true;
}
}
