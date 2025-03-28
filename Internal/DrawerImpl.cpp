
module;

#include <glm/glm.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

module YT:DrawerImpl;

import :Types;
import :RenderTypes;
import :RenderManager;
import :QuadRender;

namespace YT
{
    Drawer::Drawer(const vk::CommandBuffer & command_buffer, const DrawerData & drawer_data,
        const PSODeferredSettings & deferred_settings) noexcept
        : m_CommandBuffer(command_buffer)
        , m_DrawerData(drawer_data)
        , m_PSODeferredSettings(deferred_settings)
    {

    }

    void Drawer::DrawRaw(PSOHandle pso_handle, uint32_t num_verts) noexcept
    {
        if (g_RenderManager->BindPSO(m_CommandBuffer, nullptr, 0, m_PSODeferredSettings, pso_handle))
        {
            m_CommandBuffer.draw(num_verts, 1, 0, 0);
        }
    }

    void Drawer::DrawQuad(glm::vec2 start, glm::vec2 size, glm::vec4 color) noexcept
    {
        QuadData quad_data
        {
            .m_Start = start / m_DrawerData.m_Size,
            .m_End = (start + size) / m_DrawerData.m_Size,
            .m_Color = color,
        };

        m_DrawCombiner.PushData(quad_data);

        FlushIfNeeded();
    }

    template <typename T>
    void Drawer::Flush(const DrawList<T> & t)
    {
        Vector<Span<const T>> draw_datas = t.Summarize();

        size_t total_size = 0;
        for (Span<const T> & draw_data : draw_datas)
        {
            total_size += draw_data.size();
        }

        if constexpr (std::is_same_v<T, QuadData>)
        {
            auto [ptr, data_handle] =
                g_RenderManager->ReserveBufferSpace(g_QuadRender->GetQuadBufferTypeUd(), sizeof(T) * total_size);

            if (ptr)
            {
                for (Span<const T> & draw_data : draw_datas)
                {
                    size_t copy_size = draw_data.size() * sizeof(T);
                    memcpy(ptr, draw_data.data(), copy_size);
                    ptr += copy_size;
                }
            }

            QuadRenderData render_data;
            render_data.m_QuadIndex = data_handle.m_Index;
            render_data.m_Count = static_cast<int>(total_size);

            if (g_RenderManager->BindPSO(m_CommandBuffer, &render_data, sizeof(render_data),
                m_PSODeferredSettings, g_QuadRender->GetQuadPSOHandle()))
            {
                m_CommandBuffer.draw(total_size * 6, 1, 0, 0);
            }
        }
    }

    void Drawer::FlushIfNeeded() noexcept
    {
        if (m_DrawCombiner.NeedsFlush())
        {
            m_DrawCombiner.Flush([this](auto & draw_data) { Flush(draw_data); });
        }
    }
}
