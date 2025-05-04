module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <wayland-client.h>
#include <wayland-xdg-shell-client-protocol.h>


module YT:WindowResource;

import :Types;
import :Widget;
import :Delegate;

namespace YT
{
    export struct WindowResource final
    {
        wl_surface * m_WaylandSurface = nullptr;
        xdg_surface * m_ShellSurface = nullptr;
        xdg_toplevel * m_Toplevel = nullptr;
        wl_callback * m_Callback = nullptr;

        uint32_t m_SwapChainImageIndex = 0;
        int m_FrameIndex = 0;
        vk::UniqueSurfaceKHR m_VkSurface;
        vk::UniqueSwapchainKHR m_SwapChain;
        Vector<vk::Image> m_SwapChainImages;
        Vector<vk::UniqueImageView> m_SwapChainImageViews;
        Vector<vk::UniqueSemaphore> m_ImageAvailableSemaphores;
        Vector<vk::UniqueSemaphore> m_RenderFinishedSemaphores;
        Vector<vk::UniqueFence> m_RenderFinishedFences;
        Vector<vk::UniqueCommandBuffer> m_CommandBuffers;

        vk::Extent2D m_RequestedExtent = {};
        vk::Extent2D m_SwapChainExtent = {};
        vk::Format m_SwapChainFormat = vk::Format::eUndefined;

        bool m_WantsRedraw : 1 = false;
        bool m_AlphaBackground : 1 = false;

        WidgetRef<WidgetBase> m_Widget;
        Delegate<bool()> m_OnCloseCallback;
    };
}
