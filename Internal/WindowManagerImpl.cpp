
module;

#include <wayland-client.h>
#include <poll.h>

#include <memory>
#include <cstdio>
#include <cstring>
#include <chrono>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_wayland.h>
#include <wayland-xdg-shell-client-protocol.h>

module YT:WindowManagerImpl;

import :Types;
import :WindowTypes;
import :WindowTable;
import :WindowResource;
import :RenderManager;
import :WindowManager;

namespace YT
{
    bool WindowManager::CreateWindowManager(const ApplicationInitInfo & init_info) noexcept
    {
        try
        {
            g_WindowManager = std::make_unique<WindowManager>(init_info);
            return true;
        }
        catch (const Exception & e)
        {
            FatalPrint("Error creating window manager: {}", e.what());
        }
        catch (...)
        {
            FatalPrint("Unknown exception creating window manager");
        }

        return false;
    }

    WindowManager::WindowManager(const ApplicationInitInfo & init_info)
    {
        m_WindowTable = std::make_unique<WindowTable>();

        m_Display = wl_display_connect(nullptr);
        if (!m_Display)
        {
            throw Exception("Failed to connect to display");
        }

        m_DisplayFD = wl_display_get_fd(m_Display);

        m_Registry = wl_display_get_registry(m_Display);
        if (!m_Registry)
        {
            throw Exception("Failed to get registry");
        }

        constexpr wl_registry_listener registry_init
        {
            .global = HandleGlobal,
            .global_remove = HandleGlobalRemove
        };

        wl_registry_add_listener(m_Registry, &registry_init, this);
        wl_display_roundtrip(m_Display);

        if (!m_Compositor)
        {
            throw Exception("Failed to get compositor");
        }

        m_DisplayDisconnected = false;
        m_LastFrameTime = std::chrono::steady_clock::now();

        double update_rate = init_info.m_UpdateRate > 0 ? init_info.m_UpdateRate : 60.0;
        m_UpdateInterval = 1.0 / update_rate;
    }

    WindowManager::~WindowManager()
    {
        if (m_Shell)
        {
            xdg_wm_base_destroy(m_Shell);
        }

        if (m_Compositor)
        {
            wl_compositor_destroy(m_Compositor);
        }

        if (m_Registry)
        {
            wl_registry_destroy(m_Registry);
        }

        if (m_Display)
        {
            wl_display_disconnect(m_Display);
        }
    }

    void WindowManager::DispatchEvents() noexcept
    {
        wl_display_roundtrip(m_Display);
    }

    void WindowManager::UpdateWindows() noexcept
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> elapsed = now - m_LastFrameTime;

        double delta_time = elapsed.count() * 1000.0;
        if (elapsed.count() > m_UpdateInterval)
        {
            m_LastFrameTime = now;
        }
    }

    void WindowManager::RenderWindows() noexcept
    {
        m_WindowTable->VisitAllValidHandles([&](const WindowHandleData & handle)
        {
            if (WindowResource* resource = m_WindowTable->ResolveHandle(handle))
            {
                g_RenderManager->RenderWindowResource(*resource);
            }
        });

        m_HasDirtyWindows = false;
    }

    void WindowManager::WaitForNextFrame() noexcept
    {
        pollfd pfd = {};
        pfd.fd = m_DisplayFD;
        pfd.events = POLLIN;

        while (true)
        {
            int ret = poll(&pfd, 1, 1);
            switch (ret)
            {
            case -1:
                FatalPrint("Poll failed");
                break;
            case 0:
                UpdateWindows();
                break;
            default:
                wl_display_roundtrip(m_Display);

                UpdateWindows();

                if (m_HasDirtyWindows)
                {
                    return;
                }
            }
        }
    }

    bool WindowManager::ShouldExit() const noexcept
    {
        return m_DisplayDisconnected;
    }

    bool WindowManager::HasOpenWindows() const noexcept
    {
        return m_WindowTable->GetNumWindows() > 0;
    }

    MaybeInvalid<WindowHandleData> WindowManager::CreateWindow(const WindowInitInfo & init_info) noexcept
    {
        wl_surface * wl_surface = wl_compositor_create_surface(m_Compositor);
        xdg_surface * shell_surface = xdg_wm_base_get_xdg_surface(m_Shell, wl_surface);
        xdg_toplevel * toplevel = xdg_surface_get_toplevel(shell_surface);

        WindowHandleData new_handle = m_WindowTable->AllocateWindowHandle(WindowResource
        {
            .m_WaylandSurface = wl_surface,
            .m_ShellSurface = shell_surface,
            .m_Toplevel = toplevel,
            .m_RequestedExtent = { init_info.m_Width, init_info.m_Height }
        });

        OptionalPtr<WindowResource> window_resource = m_WindowTable->ResolveHandle(new_handle);
        if (!window_resource)
        {
            FatalPrint("Failed to get window resource");
            return s_InvalidWindowHandle;
        }

        void * handle_ptr = WindowHandleData::ConvertToPtr(new_handle);

        static constexpr xdg_surface_listener shell_surface_listener =
        {
            .configure = HandleShellSurfaceConfigure
        };

        xdg_surface_add_listener(shell_surface, &shell_surface_listener, handle_ptr);

        static constexpr xdg_toplevel_listener toplevel_listener =
        {
            .configure = HandleToplevelConfigure,
            .close = HandleToplevelClose
        };

        xdg_toplevel_add_listener(toplevel, &toplevel_listener, handle_ptr);

        xdg_toplevel_set_title(toplevel, init_info.m_WindowName.c_str());
        xdg_toplevel_set_app_id(toplevel, m_ApplicationName.c_str());

        wl_surface_commit(wl_surface);
        static constexpr wl_callback_listener surface_frame_listener =
        {
            .done = HandleFrameCallback
        };

        window_resource->m_Callback = wl_surface_frame(window_resource->m_WaylandSurface);
        wl_callback_add_listener(window_resource->m_Callback, &surface_frame_listener, handle_ptr);

        wl_display_roundtrip(m_Display);
        wl_surface_commit(wl_surface);

        if (!g_RenderManager->CreateWindowResources(init_info, *window_resource))
        {
            FatalPrint("Failed to create window render resources");
            return s_InvalidWindowHandle;
        }

        return new_handle;
    }

    bool WindowManager::IsValidWindow(WindowHandleData handle) const noexcept
    {
        if (const WindowResource* resource = m_WindowTable->ResolveHandle(handle))
        {
            return true;
        }

        return false;
    }

    bool WindowManager::SetWindowContent(WindowHandleData handle, const WidgetRef<WidgetBase> & widget_ref) noexcept
    {
        if (WindowResource* resource = m_WindowTable->ResolveHandle(handle))
        {
            resource->m_Widget = widget_ref;
            return true;
        }

        return false;
    }

    bool WindowManager::SetWindowContent(WindowHandleData handle, WidgetRef<WidgetBase> && widget_ref) noexcept
    {
        if (WindowResource* resource = m_WindowTable->ResolveHandle(handle))
        {
            resource->m_Widget = std::move(widget_ref);
            return true;
        }

        return false;
    }

    OptionalPtr<Delegate<bool()>> WindowManager::GetOnCloseCallback(WindowHandleData handle) noexcept
    {
        if (WindowResource* resource = m_WindowTable->ResolveHandle(handle))
        {
            return &resource->m_OnCloseCallback;
        }

        return nullptr;
    }

    void WindowManager::CloseWindow(WindowHandleData handle) noexcept
    {
        if (WindowResource* resource = m_WindowTable->ResolveHandle(handle))
        {
            g_RenderManager->ReleaseWindowResource(*resource);

            if (resource->m_Callback)
            {
                wl_callback_destroy(resource->m_Callback);
            }

            if (resource->m_Toplevel)
            {
                xdg_toplevel_destroy(resource->m_Toplevel);
            }

            if (resource->m_ShellSurface)
            {
                xdg_surface_destroy(resource->m_ShellSurface);
            }

            if (resource->m_WaylandSurface)
            {
                wl_surface_destroy(resource->m_WaylandSurface);
            }
        }

        m_WindowTable->ReleaseWindowHandle(handle);
    }

    void WindowManager::CloseAllWindows() noexcept
    {
        m_WindowTable->VisitAllValidHandles([&](const WindowHandleData & handle)
        {
           CloseWindow(handle);
        });
    }

    const char * WindowManager::GetSurfaceExtensionName() noexcept
    {
        return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    }

    bool WindowManager::CheckDeviceSupport(vk::PhysicalDevice physical_device, uint32_t queue_index) const noexcept
    {
        return vkGetPhysicalDeviceWaylandPresentationSupportKHR(physical_device, queue_index, m_Display);
    }

    bool WindowManager::CreateRenderSurface(vk::UniqueInstance & instance, WindowResource & resource) const noexcept
    {
        VkWaylandSurfaceCreateInfoKHR create_info { };
        create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.display = m_Display;
        create_info.surface = resource.m_WaylandSurface;
        create_info.flags = 0;

        VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
        VkResult result;
        if ((result = vkCreateWaylandSurfaceKHR(instance.get(), &create_info, nullptr, &vk_surface)) == VK_SUCCESS)
        {
            vk::detail::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(instance.get());
            resource.m_VkSurface = vk::UniqueSurfaceKHR( vk::SurfaceKHR(vk_surface), deleter);
            return true;
        }

        FatalPrint("Failed to create surface {}", vk::to_string(static_cast<vk::Result>(result)));
        return false;
    }

    void WindowManager::HandleGlobal(void * data, wl_registry * registry, uint32_t name,
                                     const char * interface, uint32_t version) noexcept
    {
        auto * self = static_cast<WindowManager *>(data);

        VerbosePrint("interface: '{}', version: {}, name: {}",
                     interface, version, name);

        if (strcmp(interface, wl_compositor_interface.name) == 0)
        {
            VerbosePrint("registering wl_compositor");
            self->m_Compositor = static_cast<wl_compositor *>(
                wl_registry_bind(self->m_Registry, name, &wl_compositor_interface, 4));

            if (!self->m_Compositor)
            {
                FatalPrint("Failed to register wl_compositor interface");
            }
        }
        else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
        {
            VerbosePrint("registering xdg_wm_base");
            self->m_Shell = static_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));

            if (!self->m_Shell)
            {
                FatalPrint("Failed to register xdg_wm_base interface");
            }

            static constexpr struct xdg_wm_base_listener shell_listener =
            {
                .ping = HandleShellPing
            };

            xdg_wm_base_add_listener(self->m_Shell, &shell_listener, nullptr);
        }
    }

    void WindowManager::HandleGlobalRemove(void * data, wl_registry * registry, uint32_t name) noexcept
    {
        VerbosePrint("Global remove: {}", name);
    }

    void WindowManager::HandleShellPing(void * data, struct xdg_wm_base * shell, uint32_t serial) noexcept
    {
        VerbosePrint("Ping: {}", serial);
        xdg_wm_base_pong(shell, serial);
    }

    void WindowManager::HandleFrameCallback(void *data, struct wl_callback *cb, uint32_t time) noexcept
    {
        wl_callback_destroy(cb);

        WindowHandleData handle = WindowHandleData::CreateFromPtr(data);
        if (WindowResource* resource = g_WindowManager->m_WindowTable->ResolveHandle(handle))
        {
            static constexpr wl_callback_listener surface_frame_listener =
            {
                .done = HandleFrameCallback
            };

            resource->m_WantsRedraw = true;
            resource->m_Callback = wl_surface_frame(resource->m_WaylandSurface);
            wl_callback_add_listener(resource->m_Callback, &surface_frame_listener, data);

            g_WindowManager->m_HasDirtyWindows = true;
        }
    }

    void WindowManager::HandleShellSurfaceConfigure(void * data, struct xdg_surface * shell_surface, uint32_t serial) noexcept
    {
        VerbosePrint("Shell surface configure: {}", serial);
        xdg_surface_ack_configure(shell_surface, serial);
    }

    void WindowManager::HandleToplevelConfigure(void * data, struct xdg_toplevel * toplevel,
                                                int32_t width, int32_t height, struct wl_array * states) noexcept
    {
        VerbosePrint("Top level configure: {} {}", width, height);
        WindowHandleData handle = WindowHandleData::CreateFromPtr(data);
        if (WindowResource* resource = g_WindowManager->m_WindowTable->ResolveHandle(handle))
        {
            if (width > 0 && height > 0)
            {
                resource->m_RequestedExtent = vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
            }
        }
    }

    void WindowManager::HandleToplevelClose(void * data, struct xdg_toplevel * toplevel) noexcept
    {
        VerbosePrint("Close");
        WindowHandleData handle = WindowHandleData::CreateFromPtr(data);
        if (WindowResource* resource = g_WindowManager->m_WindowTable->ResolveHandle(handle))
        {
            if (resource->m_OnCloseCallback.ExecuteWithDefault(true))
            {
                g_WindowManager->CloseWindow(handle);
            }
        }

    }
}
