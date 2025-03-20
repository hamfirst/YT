module;

#include <memory>
#include <chrono>

#include <wayland-client.h>
#include <wayland-xdg-shell-client-protocol.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

module YT:WindowManager;

import :Types;
import :WindowResource;
import :WindowTypes;
import :WindowTable;
import :Widget;
import :Delegate;

namespace YT
{
    export class WindowManager final
    {
    public:
        static bool CreateWindowManager(const ApplicationInitInfo & init_info) noexcept;

        explicit WindowManager(const ApplicationInitInfo & init_info);
        ~WindowManager();

        void DispatchEvents() noexcept;
        void UpdateWindows() noexcept;
        void RenderWindows() noexcept;
        void WaitForNextFrame() noexcept;

        [[nodiscard]] bool ShouldExit() const noexcept;
        [[nodiscard]] bool HasOpenWindows() const noexcept;

        [[nodiscard]] MaybeInvalid<WindowHandleData> CreateWindow(const WindowInitInfo & init_info) noexcept;

        [[nodiscard]] bool IsValidWindow(WindowHandleData handle) const noexcept;

        bool SetWindowContent(WindowHandleData handle, const WidgetRef<WidgetBase> & widget_ref) noexcept;
        bool SetWindowContent(WindowHandleData handle, WidgetRef<WidgetBase> && widget_ref) noexcept;

        OptionalPtr<Delegate<bool()>> GetOnCloseCallback(WindowHandleData handle) noexcept;

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

        static void HandleFrameCallback(void *data, struct wl_callback *cb, uint32_t time) noexcept;

        static void HandleShellPing(void * data, struct xdg_wm_base * shell, uint32_t serial) noexcept;
        static void HandleShellSurfaceConfigure(void * data, struct xdg_surface * shellSurface, uint32_t serial) noexcept;

        static void HandleToplevelConfigure(void * data, struct xdg_toplevel * toplevel,
                                            int32_t width, int32_t height, struct wl_array * states) noexcept;
        static void HandleToplevelClose(void * data, struct xdg_toplevel * toplevel) noexcept;


    private:
        friend class WindowManagerInternal;

        String m_ApplicationName;

        wl_display * m_Display = nullptr;
        int m_DisplayFD = 0;

        wl_registry * m_Registry = nullptr;
        wl_compositor * m_Compositor = nullptr;

        xdg_wm_base * m_Shell = nullptr;
        bool m_DisplayDisconnected = true;

        UniquePtr<WindowTable> m_WindowTable;

        std::chrono::time_point<std::chrono::steady_clock> m_LastFrameTime;
        double m_UpdateInterval = 0.0;
        bool m_HasDirtyWindows = false;
    };

    export UniquePtr<WindowManager> g_WindowManager;

}
