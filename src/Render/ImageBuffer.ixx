module;

#include <cstddef>
#include <cstdint>
#include <utility>
#include <stdexcept>
#include <type_traits>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:ImageBuffer;

import :Types;
import :RenderTypes;
import :StagingBuffer;

namespace YT
{
    vk::Format GetVkFormat(ImageFormat format)
    {
        switch (format)
        {
        default:
        case ImageFormat::R8G8B8A8Unorm:
            return vk::Format::eR8G8B8A8Unorm;
            break;
        }
    }

    class ImageBuffer final
    {
    public:
        ImageBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator,
            Vector<vk::ImageMemoryBarrier2> & pre_transfer_memory_barriers,
            Vector<vk::ImageMemoryBarrier2> & post_transfer_memory_barriers,
            std::uint32_t width, std::uint32_t height, ImageFormat format) :
            m_Device(device), m_Allocator(allocator), m_Width(width), m_Height(height), m_Format(format)
        {
            vk::ImageCreateInfo image_create_info;
            image_create_info.imageType = vk::ImageType::e2D;
            image_create_info.extent.width = width;
            image_create_info.extent.height = height;
            image_create_info.extent.depth = 1;
            image_create_info.mipLevels = 1;
            image_create_info.arrayLayers = 1;
            image_create_info.format = GetVkFormat(format);

            image_create_info.tiling = vk::ImageTiling::eOptimal;
            image_create_info.initialLayout = vk::ImageLayout::eUndefined;
            image_create_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            image_create_info.sharingMode = vk::SharingMode::eExclusive;
            image_create_info.samples = vk::SampleCountFlagBits::e1;
            image_create_info.flags = {};

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

            auto [allocation, image] =
                allocator->createImageUnique(image_create_info, allocation_create_info);

            if (!image || !allocation)
            {
                throw Exception("Could not create image");
            }

            m_Image = std::move(image);
            m_Allocation = std::move(allocation);

            vk::ImageViewCreateInfo view_create_info;
            view_create_info.image = m_Image.get();
            view_create_info.viewType = vk::ImageViewType::e2D;
            view_create_info.format = image_create_info.format;
            view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            m_ImageView = m_Device->createImageViewUnique(view_create_info, nullptr);
            if (!m_ImageView)
            {
                throw Exception("Could not create image view");
            }

            vk::ImageMemoryBarrier2 & pre_memory_barrier = pre_transfer_memory_barriers.emplace_back();
            pre_memory_barrier.oldLayout = vk::ImageLayout::eUndefined;
            pre_memory_barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
            pre_memory_barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            pre_memory_barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            pre_memory_barrier.image = m_Image.get();
            pre_memory_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            pre_memory_barrier.subresourceRange.baseMipLevel = 0;
            pre_memory_barrier.subresourceRange.levelCount = 1;
            pre_memory_barrier.subresourceRange.baseArrayLayer = 0;
            pre_memory_barrier.subresourceRange.layerCount = 1;
            pre_memory_barrier.srcAccessMask = {};
            pre_memory_barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
            pre_memory_barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            pre_memory_barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;

            vk::ImageMemoryBarrier2 & post_memory_barrier = post_transfer_memory_barriers.emplace_back();
            post_memory_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            post_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            post_memory_barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            post_memory_barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            post_memory_barrier.image = m_Image.get();
            post_memory_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            post_memory_barrier.subresourceRange.baseMipLevel = 0;
            post_memory_barrier.subresourceRange.levelCount = 1;
            post_memory_barrier.subresourceRange.baseArrayLayer = 0;
            post_memory_barrier.subresourceRange.layerCount = 1;
            post_memory_barrier.srcAccessMask = {};
            post_memory_barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
            post_memory_barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
            post_memory_barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;

            //staging_buffer.Transfer(command_buffer, *m_Image, width, height);
        }

        ImageBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator,
            vk::Image non_owning_image, std::uint32_t width, std::uint32_t height, ImageFormat format) :
            m_Device(device), m_Allocator(allocator), m_Width(width), m_Height(height), m_Format(format)
        {
            m_NonOwningImage = non_owning_image;

            vk::ImageViewCreateInfo view_create_info;
            view_create_info.image = m_Image.get();
            view_create_info.viewType = vk::ImageViewType::e2D;
            view_create_info.format = GetVkFormat(format);
            view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            m_ImageView = m_Device->createImageViewUnique(view_create_info, nullptr);
            if (!m_ImageView)
            {
                throw Exception("Could not create image view");
            }
        }

        ImageBuffer(const ImageBuffer&) = delete;
        ImageBuffer(ImageBuffer&&) = delete;
        ImageBuffer& operator=(const ImageBuffer&) = delete;
        ImageBuffer& operator=(ImageBuffer&&) = delete;
        ~ImageBuffer() noexcept = default;

        [[nodiscard]] vk::Image GetImage() const noexcept
        {
            return m_Image ? m_Image.get() : m_NonOwningImage;
        }

        [[nodiscard]] vk::ImageView GetImageView() const noexcept
        {
            return m_ImageView.get();
        }

        [[nodiscard]] std::uint32_t GetWidth() const noexcept
        {
            return m_Width;
        }

        [[nodiscard]] std::uint32_t GetHeight() const noexcept
        {
            return m_Height;
        }

        [[nodiscard]] ImageFormat GetFormat() const noexcept
        {
            return m_Format;
        }

    private:
        vk::UniqueDevice & m_Device;
        vma::UniqueAllocator & m_Allocator;
        vma::UniqueImage m_Image;
        vk::Image m_NonOwningImage;
        vk::UniqueImageView m_ImageView;
        vma::UniqueAllocation m_Allocation;

        std::uint32_t m_Width = 0;
        std::uint32_t m_Height = 0;
        ImageFormat m_Format = ImageFormat::R8G8B8A8Unorm;
    };
}