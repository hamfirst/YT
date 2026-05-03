
module;

//import_std

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <memory>
#include <stdexcept>
#include <coroutine>

#include <stb_image.h>

module YT:DeferredImageLoadImpl;

import :Types;
import :ImageReference;
import :DeferredImageLoad;
import :RenderManager;
import :FileMapper;
import :Coroutine;

namespace YT
{
    DeferredImageLoad::DeferredImageLoad(const Span<const std::byte> & image_data) noexcept
        : m_ImageData(image_data)
    {
        m_Next = g_DeferredImageLoadHead;
        g_DeferredImageLoadHead = this;
    }

    DeferredImageLoad::DeferredImageLoad(const StringView & file_name) noexcept
        : m_FileName(file_name)
    {
        m_Next = g_DeferredImageLoadHead;
        g_DeferredImageLoadHead = this;
    }

    DeferredImageLoad::operator const ImageReference & () const noexcept
    {
        return m_ImageRef.value();
    }

    const ImageReference & DeferredImageLoad::operator -> () const noexcept
    {
        return *m_ImageRef;
    }

    const ImageReference & DeferredImageLoad::operator * () const noexcept
    {
        return *m_ImageRef;
    }

    void DeferredImageLoad::Start() noexcept
    {
    }

    struct ImageLoadInfo
    {
        std::uint32_t m_TexWidth = 0;
        std::uint32_t m_TexHeight = 0;
        std::uint32_t m_TexChannels = 0;
        ImageFormat m_Format = ImageFormat::R8G8B8A8Unorm;
        stbi_uc * m_Pixels = nullptr;
        Span<const std::byte> m_TexData = {};
    };

    ImageLoadInfo DecodeImage(const Span<const std::byte> & image_file_data) noexcept
    {
        ImageLoadInfo load_info;
        int tex_width, tex_height, tex_channels;
        load_info.m_Pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(image_file_data.data()),
            static_cast<int>(image_file_data.size()), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

        load_info.m_TexWidth = tex_width;
        load_info.m_TexHeight = tex_height;
        load_info.m_TexChannels = tex_channels;

        load_info.m_Format = ImageFormat::R8G8B8A8Unorm;
        load_info.m_TexData = { reinterpret_cast<const std::byte *>(load_info.m_Pixels), load_info.m_TexWidth * load_info.m_TexHeight * 4 };

        return load_info;
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
        if (load_info.m_Pixels)
        {
            ImageReference image_reference = g_RenderManager->CreateImageFromPixels(load_info.m_TexData,
                load_info.m_TexWidth, load_info.m_TexHeight, load_info.m_Format);

            stbi_image_free(load_info.m_Pixels);

            return std::move(image_reference);
        }

        return ImageReference{};
    }

    MainThreadTask<ImageReference> LoadImageFromMemory(const Span<const std::byte> & image_data) noexcept
    {
        ImageLoadInfo load_info = co_await DecodeImageAsync(image_data);
        co_return CreateImage(load_info);
    }

    MainThreadTask<ImageReference> LoadImageFromFile(const StringView & file_name) noexcept
    {
        ImageLoadInfo load_info = co_await DecodeImageFileAsync(file_name);
        co_return CreateImage(load_info);
    }

    void DeferredImageLoad::Finalize() noexcept
    {
        if (!g_DeferredImageLoadHead)
        {
            return;
        }

        Vector<DeferredImageLoad*> deferred_images;

        DeferredImageLoad * ptr = g_DeferredImageLoadHead;
        while (ptr != nullptr)
        {
            deferred_images.emplace_back(ptr);
            ptr = ptr->m_Next;
        }

        CoroBundle<ImageReference> loads;
        loads.Reserve(deferred_images.size());

        for (DeferredImageLoad * ptr : deferred_images)
        {
            if (ptr->m_ImageData.data())
            {
                loads.PushJob(LoadImageFromMemory(ptr->m_ImageData));
            }
            else
            {
                loads.PushJob(LoadImageFromFile(ptr->m_FileName));
            }
        }

        loads.WaitForCompletion();

        int load_index = 0;
        for (DeferredImageLoad * ptr : deferred_images)
        {
            ptr->m_ImageRef = std::move(loads[load_index]);
            ++load_index;
        }

        g_DeferredImageLoadHead = nullptr;

    }

}