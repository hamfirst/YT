module;

#include <cstddef>
#include <cstdint>
#include <new>
#include <cstdio>

export module YT:ImageLoad;

import :Types;
import :RenderTypes;
import :Coroutine;
import :ImageReference;

namespace YT
{
    export class ImageLoadInfo final
    {
    public:
        ImageLoadInfo() noexcept
        {
        }

        ImageLoadInfo(std::uint32_t tex_width, std::uint32_t tex_height, std::uint32_t tex_channels,
                      ImageFormat format, void * pixels, void (*free_pixels)(void *),
                      Span<const std::byte> tex_data) noexcept
            : m_TexWidth(tex_width)
            , m_TexHeight(tex_height)
            , m_TexChannels(tex_channels)
            , m_Format(format)
            , m_Pixels(pixels)
            , m_FreePixels(free_pixels)
            , m_TexData(tex_data)
        {
        }

        ImageLoadInfo(ImageLoadInfo && rhs) noexcept
            : m_TexWidth(rhs.m_TexWidth)
            , m_TexHeight(rhs.m_TexHeight)
            , m_TexChannels(rhs.m_TexChannels)
            , m_Format(rhs.m_Format)
            , m_Pixels(rhs.m_Pixels)
            , m_FreePixels(rhs.m_FreePixels)
            , m_TexData(rhs.m_TexData)
        {

            rhs.m_Pixels = nullptr;
            rhs.m_FreePixels = nullptr;
            rhs.m_TexData = {};
        }

        ImageLoadInfo &operator=(ImageLoadInfo && rhs) noexcept
        {
            if (this != &rhs)
            {
                if (m_Pixels != nullptr && m_FreePixels != nullptr)
                {
                    m_FreePixels(m_Pixels);
                }
                m_Pixels = nullptr;
                m_FreePixels = nullptr;
                m_TexData = {};

                m_TexWidth = rhs.m_TexWidth;
                m_TexHeight = rhs.m_TexHeight;
                m_TexChannels = rhs.m_TexChannels;
                m_Format = rhs.m_Format;
                m_Pixels = rhs.m_Pixels;
                m_FreePixels = rhs.m_FreePixels;
                m_TexData = rhs.m_TexData;

                rhs.m_Pixels = nullptr;
                rhs.m_FreePixels = nullptr;
                rhs.m_TexData = {};
            }

            return *this;
        }

        ImageLoadInfo(const ImageLoadInfo &) = delete;
        ImageLoadInfo &operator=(const ImageLoadInfo &) = delete;

        ~ImageLoadInfo() noexcept
        {
            if (m_Pixels != nullptr && m_FreePixels != nullptr)
            {
                m_FreePixels(m_Pixels);
            }
            m_Pixels = nullptr;
            m_FreePixels = nullptr;
            m_TexData = {};
        }

        operator bool() const noexcept
        {
            return m_Pixels != nullptr;
        }

        [[nodiscard]] std::uint32_t GetTexWidth() const noexcept { return m_TexWidth; }
        [[nodiscard]] std::uint32_t GetTexHeight() const noexcept { return m_TexHeight; }
        [[nodiscard]] std::uint32_t GetTexChannels() const noexcept { return m_TexChannels; }
        [[nodiscard]] Span<const std::byte> GetTexData() const noexcept { return m_TexData; }
        [[nodiscard]] ImageFormat GetFormat() const noexcept { return m_Format; }

    private:
        std::uint32_t m_TexWidth = 0;
        std::uint32_t m_TexHeight = 0;
        std::uint32_t m_TexChannels = 0;
        ImageFormat m_Format = ImageFormat::R8G8B8A8Unorm;
        void * m_Pixels = nullptr;
        void (*m_FreePixels)(void *) = nullptr;
        Span<const std::byte> m_TexData = {};
    };

    ImageLoadInfo DecodeImage(const Span<const std::byte> & image_file_data) noexcept;

    BackgroundTask<ImageLoadInfo> DecodeImageAsync(const Span<const std::byte> & image_file_data) noexcept;
    BackgroundTask<ImageLoadInfo> DecodeImageFileAsync(const StringView & file_name) noexcept;

    ImageReference CreateImage(const ImageLoadInfo & load_info) noexcept;

    MainThreadTask<ImageReference> LoadImageFromMemory(const Span<const std::byte> & image_data) noexcept;
    MainThreadTask<ImageReference> LoadImageFromFile(const StringView & file_name) noexcept;
}
