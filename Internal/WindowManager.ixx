module;

#include <memory>

#include <wayland-client.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "../External/XDG/XDGShell.h"

import YT.Types;
import YT.WindowResource;
import YT.WindowTypes;
import YT.WindowTable;
import YT.Widget;

export module YT.WindowManager;

namespace YT
{
    export class WindowManager final
    {
    public:
        static bool CreateWindowManager(const ApplicationInitInfo & init_info) noexcept;

        explicit WindowManager(const ApplicationInitInfo & init_info);
        ~WindowManager();

        void DispatchEvents() noexcept;
        void RenderWindows() noexcept;
        void WaitForNextFrame() noexcept;

        [[nodiscard]] bool ShouldExit() const noexcept;
        [[nodiscard]] bool HasOpenWindows() const noexcept;

        [[nodiscard]] MaybeInvalid<WindowHandleData> CreateWindow(const WindowInitInfo & init_info) noexcept;

        [[nodiscard]] bool IsValidWindow(WindowHandleData handle) const noexcept;

        bool SetWindowContent(WindowHandleData handle, const WidgetRef<WidgetBase> & widget_ref) noexcept;
        bool SetWindowContent(WindowHandleData handle, WidgetRef<WidgetBase> && widget_ref) noexcept;

        void CloseWindow(WindowHandleData handle) noexcept;

        void CloseAllWindows() noexcept;

        constexpr static WindowHandleData s_InvalidWindowHandle = {};

        [[nodiscard]] static const char * GetSurfaceExtensionName() noexcept;
        [[nodiscard]] bool CheckDeviceSupport(vk::PhysicalDevice physical_device, uint32_t queue_index) const noexcept;
        [[nodiscard]] bool CreateRenderSurface(vk::UniqueInstance & instance, WindowResource & resource) const noexcept;

    private:
        static void HandleGlobal(void * data, wl_registry * registry, uint32_t name,
                                 const char * interface, uint32_t version) noexcept;

        static void HandleGlobalRemove(void * data, wl_registry * registry, uint32_t name) noexcept;

        static void HandleShellPing(void * data, struct xdg_wm_base * shell, uint32_t serial) noexcept;
        static void HandleShellSurfaceConfigure(void * data, struct xdg_surface * shellSurface, uint32_t serial) noexcept;

        static void HandleToplevelConfigure(void * data, struct xdg_toplevel * toplevel,
                                            int32_t width, int32_t height, struct wl_array * states) noexcept;
        static void HandleToplevelClose(void * data, struct xdg_toplevel * toplevel) noexcept;

    private:
        friend class WindowManagerInternal;

        String m_ApplicationName;

        wl_display * m_Display = nullptr;
        wl_registry * m_Registry = nullptr;
        wl_compositor * m_Compositor = nullptr;

        xdg_wm_base * m_Shell = nullptr;
        bool m_DisplayDisconnected = true;

        UniquePtr<WindowTable> m_WindowTable;
    };

    export UniquePtr<WindowManager> g_WindowManager;

}
