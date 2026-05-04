module;

//import_std

#include <cstddef>
#include <cstdint>
#include <memory>

export module YT;

export import :Types;
export import :OwnedBuffer;
export import :JobTypes;
export import :BlockTable;
export import :FixedBlockAllocator;
export import :MultiProducerSingleConsumer;
export import :MultiProducerMultiConsumer;
export import :Init;
export import :Delegate;
export import :Coroutine;
export import :CoroEvent;
export import :Wait;
export import :Window;
export import :Widget;
export import :Drawer;
export import :RenderTypes;
export import :RenderReflect;
export import :ImageReference;
export import :ImageLoad;
export import :DeferredImageLoad;
export import :WindowTypes;
export import :FileMapper;
export import :FontTypes;
export import :FontReference;
export import :FontManager;
export import :FontLoad;
export import :DeferredFontLoad;

namespace YT
{
    export [[nodiscard]] MaybeInvalid<WindowHandle> CreateWindow(const WindowInitInfo & window_info);

    export [[nodiscard]] MaybeInvalid<WindowRef> CreateWindow_GetRef(const WindowInitInfo & init_info) noexcept
    {
        return WindowRef(init_info);
    }

    export void RegisterShader(const std::uint8_t* shader_data, std::size_t shader_data_size) noexcept;

    export template <std::size_t ArraySize>
    void RegisterShader(std::uint8_t (&shader_data)[ArraySize]) noexcept
    {
        RegisterShader(shader_data, ArraySize);
    }

    export [[nodiscard]] MaybeInvalid<PSOHandle> CreatePSO(const PSOCreateInfo & create_info) noexcept;

    export [[nodiscard]] MaybeInvalid<ImageReference> LoadImageFromFile(const char * filename) noexcept;
    export [[nodiscard]] MaybeInvalid<ImageReference> LoadImageFromData(const Span<std::byte>& image_data) noexcept;
    export [[nodiscard]] MaybeInvalid<ImageReference> CreateImageFromNativeHandle(
        void * native_handle, std::uint32_t width, std::uint32_t height) noexcept;

    export [[nodiscard]] double GetApplicationTime() noexcept;
}
