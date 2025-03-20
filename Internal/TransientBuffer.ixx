module;

#include <atomic>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:TransientBuffer;

import :Types;

namespace YT
{
    export class TransientBuffer
    {
    public:
        TransientBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator, size_t size, bool is_staging = false)
            : m_Device(device), m_Allocator(allocator), m_AllocationSize(size)
        {
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = size;
            buffer_create_info.usage = is_staging ? vk::BufferUsageFlagBits::eTransferSrc :
                vk::BufferUsageFlagBits::eStorageBuffer;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.usage = vma::MemoryUsage::eAutoPreferHost;

            auto [buffer, allocation] =
                allocator->createBufferUnique(buffer_create_info, allocation_create_info);

            m_Buffer = std::move(buffer);
            m_Allocation = std::move(allocation);
        }

        bool Begin() noexcept
        {
            m_Ptr = static_cast<std::byte *>(m_Allocator->mapMemory(m_Allocation.get()));
            m_BufferSize = 0;
            return true;
        }

        uint64_t WriteData(const void * data, size_t size) noexcept
        {
            if (!m_Ptr)
            {
                FatalPrint("Buffer is null");
                return UINT64_MAX;
            }

            uint64_t start = m_BufferSize.fetch_add(size);

            if (start + size > m_AllocationSize)
            {
                FatalPrint("Buffer is not big enough");
                return UINT64_MAX;
            }

            memcpy(m_Ptr + start, data, size);
            return start;
        }

        [[nodiscard]] size_t GetSize() const noexcept
        {
            return m_BufferSize;
        }

        void Transfer(vk::CommandBuffer & command_buffer, vk::Buffer & buffer, size_t offset) noexcept
        {
            vk::BufferCopy copy_region;
            copy_region.size = GetSize();
            copy_region.srcOffset = 0;
            copy_region.dstOffset = offset;

            command_buffer.copyBuffer(m_Buffer.get(), buffer, 1, &copy_region);
        }

        void End() noexcept
        {
            m_Allocator->unmapMemory(m_Allocation.get());
            m_Ptr = nullptr;
        }

    private:

        vk::UniqueDevice & m_Device;
        vma::UniqueAllocator & m_Allocator;
        size_t m_AllocationSize;
        vma::UniqueBuffer m_Buffer;
        vma::UniqueAllocation m_Allocation;
        std::atomic<size_t> m_BufferSize;

        std::byte * m_Ptr = nullptr;
    };
}