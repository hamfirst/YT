module;

//import_std

#include <cstdint>
#include <cstddef>
#include <memory>
#include <chrono>

module YT:Impl;

import :Init;
import :RenderManager;

namespace YT
{
    void RegisterShader(const std::uint8_t* shader_data, std::size_t shader_data_size) noexcept
    {
        g_RenderManager->RegisterShader(shader_data, shader_data_size);
    }

    MaybeInvalid<PSOHandle> CreatePSO(const PSOCreateInfo & create_info) noexcept
    {
        return g_RenderManager->RegisterPSO(create_info);
    }

    double GetApplicationTime() noexcept
    {
        return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - g_InitTime).count();
    }
}
