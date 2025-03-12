module;

#include <cstdint>

import YT.WindowManager;
import YT.RenderManager;

export module YT;

export import YT.Types;
export import YT.Init;
export import YT.Drawer;
export import YT.Window;
export import YT.Widget;

namespace YT
{
    export WindowHandle CreateWindow(const WindowInitInfo & window_info)
    {
        return WindowHandle(g_WindowManager->CreateWindow(window_info));
    }

    export WindowRef CreateWindow_GetRef(const WindowInitInfo & init_info) noexcept
    {
        return WindowRef(init_info);
    }

    export template <int ArraySize>
    void RegisterShader(uint8_t (&shader_data)[ArraySize]) noexcept
    {
        g_RenderManager->RegisterShader(shader_data, ArraySize);
    }

    void RegisterShader(uint8_t* shader_data, std::size_t shader_data_size) noexcept
    {
        g_RenderManager->RegisterShader(shader_data, shader_data_size);
    }

    export MaybeInvalid<PSOHandle> CreatePSO(const PSOCreateInfo & create_info) noexcept
    {
        return g_RenderManager->RegisterPSO(create_info);
    }

}
