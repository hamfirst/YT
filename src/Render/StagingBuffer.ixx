module;

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <span>
#include <type_traits>
#include <stdexcept>

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

        StagingBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator, std::size_t buffer_size)
            : m_Device(device), m_Allocator(allocator), m_AllocationSize(buffer_size)
        {
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = buffer_size;
            buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
            allocation_create_info.usage = vma::MemoryUsage::eAutoPreferHost;

            auto [allocation, buffer] =
                allocator->createBufferUnique(buffer_create_info, allocation_create_info);

            m_Buffer = std::move(buffer);
            m_Allocation = std::move(allocation);
        }

        StagingBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator, const Span<const std::byte> & data)
            : StagingBuffer{device, allocator, data.size()}
        {
            auto buffer_data = Lock();

            memcpy(buffer_data.data(), data.data(), data.size());

            Unlock();
        }

        StagingBuffer(const StagingBuffer&) = delete;
        StagingBuffer(StagingBuffer&&) noexcept = default;
        StagingBuffer& operator=(const StagingBuffer&) = delete;
        StagingBuffer& operator=(StagingBuffer&&) noexcept = delete;
        ~StagingBuffer() noexcept = default;

        [[nodiscard]] vk::Buffer GetBuffer() const noexcept
        {
            return m_Buffer.get();
        }

        [[nodiscard]] std::size_t GetAllocationSize() const noexcept
        {
            return m_AllocationSize;
        }

        void Transfer(vk::CommandBuffer & command_buffer, vk::Buffer & target_buffer, std::size_t offset) noexcept
        {
            vk::BufferCopy copy_region;
            copy_region.size = m_AllocationSize;
            copy_region.srcOffset = 0;
            copy_region.dstOffset = offset;

            command_buffer.copyBuffer(m_Buffer.get(), target_buffer, 1, &copy_region);
        }

        void Transfer(vk::CommandBuffer & command_buffer, vk::Image & target_image, std::uint32_t width, std::uint32_t height)
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

        static vk::BufferImageCopy GetPartialTransferInfo(
            std::uint32_t buffer_offset, std::uint32_t x, std::uint32_t y,
            std::uint32_t width, std::uint32_t height) noexcept
        {
            vk::BufferImageCopy region;
            region.bufferOffset = buffer_offset;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = vk::Offset3D{ static_cast<int32_t>(x), static_cast<int32_t>(y), 0 };
            region.imageExtent = vk::Extent3D{ width, height, 1 };
            return region;
        }

        Span<std::byte> Lock()
        {
            assert(!m_IsLocked);

            auto * ptr = static_cast<std::byte *>(m_Allocator->mapMemory(m_Allocation.get()));

            if (!ptr)
            {
                throw Exception("Could not map allocation memory");
            }

            m_IsLocked = true;
            return Span{ ptr, m_AllocationSize };
        }

        void Unlock()
        {
            assert(m_IsLocked);
            m_Allocator->unmapMemory(m_Allocation.get());
            m_IsLocked = false;
        }

    private:

        vk::UniqueDevice & m_Device;
        vma::UniqueAllocator & m_Allocator;
        vma::UniqueBuffer m_Buffer;
        vma::UniqueAllocation m_Allocation;
        std::size_t m_AllocationSize =  0;
        bool m_IsLocked = false;
    };
}