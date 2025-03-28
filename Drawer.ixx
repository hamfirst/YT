module;

#include <glm/glm.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>


export module YT:Drawer;

import :Types;
import :RenderTypes;
import :DrawCombiner;

namespace YT
{
    export class Drawer final
    {
    public:
        Drawer(const vk::CommandBuffer & command_buffer, const DrawerData & drawer_data,
            const PSODeferredSettings & deferred_settings) noexcept;

        void DrawRaw(PSOHandle pso_handle, uint32_t num_verts) noexcept;

        void DrawQuad(glm::vec2 start, glm::vec2 size, glm::vec4 color) noexcept;

    private:

        template<typename T>
        void Flush(const DrawList<T> & t);

        void FlushIfNeeded() noexcept;

    private:
        vk::CommandBuffer m_CommandBuffer;
        DrawerData m_DrawerData;
        PSODeferredSettings m_PSODeferredSettings;

        DrawCombiner<QuadData> m_DrawCombiner;
    };

}

