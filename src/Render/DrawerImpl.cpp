
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
        FlushIfNeeded(DrawType::None);

        if (g_RenderManager->BindPSO(m_CommandBuffer, nullptr, 0, m_PSODeferredSettings, pso_handle))
        {
            m_CommandBuffer.draw(num_verts, 1, 0, 0);
        }
    }

    void Drawer::DrawQuad(glm::vec2 start, glm::vec2 size, glm::vec4 color) noexcept
    {
        FlushIfNeeded(DrawType::Quad);

        auto [ptr, data_handle] = g_RenderManager->ReserveBufferSpace(
            g_QuadRender->GetQuadBufferTypeId(), sizeof(QuadData));

        new (ptr) QuadData
        {
            .m_Start = start / m_DrawerData.m_Size,
            .m_End = (start + size) / m_DrawerData.m_Size,
            .m_Color = color,
        };

        m_DrawCount++;

        uint32_t index = static_cast<uint32_t>(data_handle.m_Index);
        if (!m_FirstDrawElemIndex.has_value())
        {
            m_FirstDrawElemIndex = index;
        }

        if constexpr (Threading::NumThreads > 1)
        {
            if (m_PreviousDrawElemIndex.has_value() && index != m_PreviousDrawElemIndex.value() + 1)
            {
                m_ConsecutiveDraws = false;
            }

            m_PreviousDrawElemIndex = index;

            m_DrawElemIndexData.push_back(IndexData
                {
                    .m_Index = index
                });
        }
    }

    void Drawer::Flush() noexcept
    {
        FlushIfNeeded(DrawType::None);
    }

    void Drawer::FlushIfNeeded(DrawType pending_draw_type) noexcept
    {
        if (m_DrawType != pending_draw_type)
        {
            if (m_DrawType == DrawType::Quad)
            {
                if constexpr (Threading::NumThreads > 1 && !m_ConsecutiveDraws)
                {
                    auto [ptr, handle] =
                        g_RenderManager->ReserveBufferSpace(g_RenderManager->GetIndexBufferTypeId(), m_DrawElemIndexData.size());

                    memcpy(ptr, m_DrawElemIndexData.data(), m_DrawElemIndexData.size() * sizeof(IndexData));
                    m_DrawElemIndexData.clear();

                    QuadRenderData render_data;
                    render_data.m_QuadIndex = handle.m_Index;
                    render_data.m_Count = static_cast<int>(m_DrawCount);

                    if (g_RenderManager->BindPSO(m_CommandBuffer, &render_data, sizeof(render_data),
                        m_PSODeferredSettings, g_QuadRender->GetQuadIndexedPSOHandle()))
                    {
                        m_CommandBuffer.draw(m_DrawCount * 6, 1, 0, 0);
                    }
                }
                else
                {
                    QuadRenderData render_data;
                    render_data.m_QuadIndex = m_FirstDrawElemIndex.value();
                    render_data.m_Count = static_cast<int>(m_DrawCount);

                    if (g_RenderManager->BindPSO(m_CommandBuffer, &render_data, sizeof(render_data),
                        m_PSODeferredSettings, g_QuadRender->GetQuadConsecutivePSOHandle()))
                    {
                        m_CommandBuffer.draw(m_DrawCount * 6, 1, 0, 0);
                    }
                }
            }

            m_DrawType = pending_draw_type;
            m_DrawCount = 0;
            m_FirstDrawElemIndex = {};
            m_PreviousDrawElemIndex = {};
            m_ConsecutiveDraws = true;
        }
    }
}
