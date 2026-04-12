module;

#include <cstddef>
#include <cstdint>
#include <limits>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

export module YT:ImageReference;

import :Types;
import :RenderTypes;

namespace YT
{
    export class ImageReference
    {
    public:
        ImageReference() noexcept = default;
        ImageReference(ImageHandle handle, std::uint32_t width, std::uint32_t height, std::uint32_t image_index) noexcept;
        ImageReference(const ImageReference&) = delete;
        ImageReference(ImageReference&& rhs) noexcept;
        ImageReference& operator=(const ImageReference&) = delete;
        ImageReference& operator=(ImageReference&& rhs) noexcept;
        ~ImageReference() noexcept;

        [[nodiscard]] ImageHandle GetHandle() const noexcept;
        [[nodiscard]] std::uint32_t GetWidth() const noexcept;
        [[nodiscard]] std::uint32_t GetHeight() const noexcept;
        [[nodiscard]] std::uint32_t GetImageIndex() const noexcept;

        operator bool() const noexcept;

        void Release();

    private:
        ImageHandle m_Handle = InvalidImageHandle;
        std::uint32_t m_Width = 0;
        std::uint32_t m_Height = 0;
        std::uint32_t m_ImageIndex = std::numeric_limits<std::uint32_t>::max();
    };
}