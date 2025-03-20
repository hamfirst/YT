
module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

module YT:DrawerImpl;

import :Types;
import :RenderTypes;
import :RenderManager;

namespace YT
{
    Drawer::Drawer(const vk::CommandBuffer & command_buffer,
        const PSODeferredSettings & deferred_settings) noexcept
        : m_CommandBuffer(command_buffer)
        , m_PSODeferredSettings(deferred_settings)
    {

    }

    void Drawer::DrawRaw(PSOHandle pso_handle, uint32_t num_verts) noexcept
    {
        if (g_RenderManager->BindPSO(m_CommandBuffer, m_PSODeferredSettings, pso_handle))
        {
            m_CommandBuffer.draw(num_verts, 1, 0, 0);
        }
    }
}
