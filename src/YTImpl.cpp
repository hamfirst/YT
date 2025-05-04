module;

#include <cstdint>
#include <cstddef>

module YT:Impl;

import :RenderManager;

namespace YT
{
    void RegisterShader(uint8_t* shader_data, std::size_t shader_data_size) noexcept
    {
        g_RenderManager->RegisterShader(shader_data, shader_data_size);
    }

    MaybeInvalid<PSOHandle> CreatePSO(const PSOCreateInfo & create_info) noexcept
    {
        return g_RenderManager->RegisterPSO(create_info);
    }
}