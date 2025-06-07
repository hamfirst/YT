module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:StagingBuffer;

import :Types;

namespace YT
{
    class StagingBuffer
    {
    public:
        StagingBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator, const void * data, size_t size)
            : m_Device(device), m_Allocator(allocator), m_AllocationSize(size)
        {
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = size;
            buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
            allocation_create_info.usage = vma::MemoryUsage::eAutoPreferHost;

            auto [buffer, allocation] =
                allocator->createBufferUnique(buffer_create_info, allocation_create_info);

            m_Buffer = std::move(buffer);
            m_Allocation = std::move(allocation);

            std::byte * ptr = static_cast<std::byte *>(m_Allocator->mapMemory(m_Allocation.get()));

            if (!ptr)
            {
                throw Exception("Could not map allocation memory");
            }

            memcpy(ptr, data, size);

            m_Allocator->unmapMemory(m_Allocation.get());
        }

        StagingBuffer(const StagingBuffer&) = delete;
        StagingBuffer(StagingBuffer&&) noexcept = default;
        StagingBuffer& operator=(const StagingBuffer&) = delete;
        StagingBuffer& operator=(StagingBuffer&&) noexcept = delete;
        ~StagingBuffer() noexcept = default;

        vk::Buffer GetBuffer() const noexcept
        {
            return m_Buffer.get();
        }

        size_t GetAllocationSize() const noexcept
        {
            return m_AllocationSize;
        }

        void Transfer(vk::CommandBuffer & command_buffer, vk::Buffer & target_buffer, size_t offset) noexcept
        {
            vk::BufferCopy copy_region;
            copy_region.size = m_AllocationSize;
            copy_region.srcOffset = 0;
            copy_region.dstOffset = offset;

            command_buffer.copyBuffer(m_Buffer.get(), target_buffer, 1, &copy_region);
        }

        void Transfer(vk::CommandBuffer & command_buffer, vk::Image & target_image, uint32_t width, uint32_t height)
        {
            // Specify the buffer to image copy operation
            vk::BufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = vk::Offset3D{0, 0, 0};
            region.imageExtent = vk::Extent3D{width, height, 1};

            // Execute the copy
            command_buffer.copyBufferToImage(
                m_Buffer.get(),
                target_image,
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &region
            );
        }

    private:

        vk::UniqueDevice & m_Device;
        vma::UniqueAllocator & m_Allocator;
        vma::UniqueBuffer m_Buffer;
        vma::UniqueAllocation m_Allocation;
        size_t m_AllocationSize;
    };
}