module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:ImageBuffer;

import :Types;
import :RenderTypes;
import :StagingBuffer;

namespace YT
{
    class ImageBuffer final
    {
    public:
        ImageBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator, vk::CommandBuffer & command_buffer,
            StagingBuffer & staging_buffer, uint32_t width, uint32_t height, ImageFormat format) :
            m_Device(device), m_Allocator(allocator), m_Width(width), m_Height(height), m_Format(format)
        {
            vk::ImageCreateInfo image_create_info;
            image_create_info.imageType = vk::ImageType::e2D;
            image_create_info.extent.width = width;
            image_create_info.extent.height = height;
            image_create_info.extent.depth = 1;
            image_create_info.mipLevels = 1;
            image_create_info.arrayLayers = 1;

            switch (format)
            {
            default:
            case ImageFormat::R8G8B8A8Unorm:
                image_create_info.format = vk::Format::eR8G8B8A8Unorm;
                break;
            }

            image_create_info.tiling = vk::ImageTiling::eOptimal;
            image_create_info.initialLayout = vk::ImageLayout::eUndefined;
            image_create_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            image_create_info.sharingMode = vk::SharingMode::eExclusive;
            image_create_info.samples = vk::SampleCountFlagBits::e1;
            image_create_info.flags = {};

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

            auto [image, allocation] =
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

            vk::ImageMemoryBarrier memory_barrier;
            memory_barrier.oldLayout = vk::ImageLayout::eUndefined;
            memory_barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
            memory_barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            memory_barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            memory_barrier.image = m_Image.get();
            memory_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            memory_barrier.subresourceRange.baseMipLevel = 0;
            memory_barrier.subresourceRange.levelCount = 1;
            memory_barrier.subresourceRange.baseArrayLayer = 0;
            memory_barrier.subresourceRange.layerCount = 1;
            memory_barrier.srcAccessMask = {};
            memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                {}, {}, {}, {memory_barrier});

            staging_buffer.Transfer(command_buffer, *m_Image, width, height);

            memory_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                {}, {}, {}, {memory_barrier});
        }

        ImageBuffer(const ImageBuffer&) = delete;
        ImageBuffer(ImageBuffer&&) = delete;
        ImageBuffer& operator=(const ImageBuffer&) = delete;
        ImageBuffer& operator=(ImageBuffer&&) = delete;
        ~ImageBuffer() noexcept = default;

        [[nodiscard]] vk::Image GetImage() const noexcept
        {
            return m_Image.get();
        }

        [[nodiscard]] vk::ImageView GetImageView() const noexcept
        {
            return m_ImageView.get();
        }

        [[nodiscard]] uint32_t GetWidth() const noexcept
        {
            return m_Width;
        }

        [[nodiscard]] uint32_t GetHeight() const noexcept
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
        vk::UniqueImageView m_ImageView;
        vma::UniqueAllocation m_Allocation;

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        ImageFormat m_Format = ImageFormat::R8G8B8A8Unorm;
    };
}