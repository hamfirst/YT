module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>


export module YT:Drawer;

import :Types;
import :RenderTypes;

namespace YT
{
    export class Drawer final
    {
    public:
        Drawer(const vk::CommandBuffer & command_buffer,
            const PSODeferredSettings & deferred_settings) noexcept;

        void DrawRaw(PSOHandle pso_handle, uint32_t num_verts) noexcept;

    private:
        vk::CommandBuffer m_CommandBuffer;
        PSODeferredSettings m_PSODeferredSettings;
    };

}

