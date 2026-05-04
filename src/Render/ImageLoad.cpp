module;

#include <cstdint>
#include <cstddef>
#include <coroutine>
#include <new>

#include <stb_image.h>

module YT:ImageLoadImpl;

import :ImageLoad;
import :Types;
import :Coroutine;
import :FileMapper;
import :RenderManager;

namespace YT
{
    void FreeStbiPixels(void * pixels) noexcept
    {
        stbi_image_free(pixels);
    }

    ImageLoadInfo DecodeImage(const Span<const std::byte> & image_file_data) noexcept
    {
        int tex_width = 0;
        int tex_height = 0;
        int tex_channels = 0;
        void * pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(image_file_data.data()),
            static_cast<int>(image_file_data.size()), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

        if (pixels == nullptr)
        {
            return ImageLoadInfo{};
        }

        const auto w = static_cast<std::uint32_t>(tex_width);
        const auto h = static_cast<std::uint32_t>(tex_height);
        const auto ch = static_cast<std::uint32_t>(tex_channels);

        return ImageLoadInfo{
            w,
            h,
            ch,
            ImageFormat::R8G8B8A8Unorm,
            pixels,
            &FreeStbiPixels,
            Span<const std::byte>{
                static_cast<const std::byte *>(pixels),
                static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * std::size_t{4},
            },
        };
    }

    BackgroundTask<ImageLoadInfo> DecodeImageAsync(const Span<const std::byte> & image_file_data) noexcept
    {
        co_return DecodeImage(image_file_data);
    }

    BackgroundTask<ImageLoadInfo> DecodeImageFileAsync(const StringView & file_name) noexcept
    {
        MappedFile file = co_await MapFileAsync(file_name);
        co_return DecodeImage(file.GetData());
    }

    ImageReference CreateImage(const ImageLoadInfo & load_info) noexcept
    {
        if (load_info)
        {
            ImageReference image_reference = g_RenderManager->CreateImageFromPixels(load_info.GetTexData(),
                load_info.GetTexWidth(), load_info.GetTexHeight(), load_info.GetFormat());

            return image_reference;
        }

        return {};
    }

    MainThreadTask<ImageReference> LoadImageFromMemory(const Span<const std::byte> & image_data) noexcept
    {
        ImageLoadInfo load_info = co_await DecodeImageAsync(image_data);
        co_await CoroEventWait(g_RenderManager->GetImageGenerationReadyEvent());
        co_return CreateImage(load_info);
    }

    MainThreadTask<ImageReference> LoadImageFromFile(const StringView & file_name) noexcept
    {
        ImageLoadInfo load_info = co_await DecodeImageFileAsync(file_name);
        co_await CoroEventWait(g_RenderManager->GetImageGenerationReadyEvent());
        co_return CreateImage(load_info);
    }
}
