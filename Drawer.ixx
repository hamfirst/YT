module;

#include <glm/glm.hpp>

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
        Drawer(const vk::CommandBuffer & command_buffer, const DrawerData & drawer_data,
            const PSODeferredSettings & deferred_settings) noexcept;

        void DrawRaw(PSOHandle pso_handle, uint32_t num_verts) noexcept;

        void DrawQuad(glm::vec2 start, glm::vec2 size, glm::vec4 color) noexcept;

        void Flush() noexcept;
    private:

        enum class DrawType
        {
            None,
            Quad,
        };

        void FlushIfNeeded(DrawType pending_draw_type) noexcept;

    private:

        vk::CommandBuffer m_CommandBuffer;
        DrawerData m_DrawerData;
        PSODeferredSettings m_PSODeferredSettings;

        DrawType m_DrawType = DrawType::None;
        size_t m_DrawCount = 0;

        Optional<uint32_t> m_FirstDrawElemIndex = {};
        Optional<uint32_t> m_PreviousDrawElemIndex = {};
        bool m_ConsecutiveDraws = true;

        Vector<IndexData> m_DrawElemIndexData;
    };

}

