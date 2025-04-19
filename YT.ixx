module;

#include <cstddef>
#include <cstdint>
#include <memory>

export module YT;

export import :Types;
export import :Init;
export import :Delegate;
export import :Window;
export import :Widget;
export import :Drawer;
export import :RenderTypes;
export import :RenderReflect;
export import :WindowTypes;
export import :JobManager;

namespace YT
{
    export WindowHandle CreateWindow(const WindowInitInfo & window_info);

    export WindowRef CreateWindow_GetRef(const WindowInitInfo & init_info) noexcept
    {
        return WindowRef(init_info);
    }

    export void RegisterShader(uint8_t* shader_data, std::size_t shader_data_size) noexcept;

    export template <int ArraySize>
    void RegisterShader(uint8_t (&shader_data)[ArraySize]) noexcept
    {
        RegisterShader(shader_data, ArraySize);
    }

    export MaybeInvalid<PSOHandle> CreatePSO(const PSOCreateInfo & create_info) noexcept;
}
