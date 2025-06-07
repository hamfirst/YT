module;

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
        ImageReference(ImageHandle handle, uint32_t width, uint32_t height, uint32_t image_index) noexcept;
        ImageReference(const ImageReference&) = delete;
        ImageReference(ImageReference&& rhs) noexcept;
        ImageReference& operator=(const ImageReference&) = delete;
        ImageReference& operator=(ImageReference&& rhs) noexcept;
        ~ImageReference() noexcept;

        [[nodiscard]] ImageHandle GetHandle() const noexcept;
        [[nodiscard]] uint32_t GetWidth() const noexcept;
        [[nodiscard]] uint32_t GetHeight() const noexcept;
        [[nodiscard]] uint32_t GetImageIndex() const noexcept;

        operator bool() const noexcept;

        void Release();

    private:
        ImageHandle m_Handle = InvalidImageHandle;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint32_t m_ImageIndex = UINT32_MAX;
    };
}