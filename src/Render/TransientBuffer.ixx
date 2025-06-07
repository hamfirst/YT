module;

#include <atomic>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:TransientBuffer;

import :Types;

namespace YT
{
    template <typename T>
    class TransientBufferSize
    {
    public:
        TransientBufferSize() noexcept
            : m_Size(0)
        {

        }

        TransientBufferSize(const TransientBufferSize &) noexcept = default;
        TransientBufferSize(TransientBufferSize &&) noexcept = default;
        TransientBufferSize & operator=(const TransientBufferSize &) noexcept = default;
        TransientBufferSize & operator=(TransientBufferSize &&) noexcept = default;
        ~TransientBufferSize() noexcept = default;

        void Reset() noexcept
        {
            if constexpr (std::is_same_v<T, size_t>)
            {
                m_Size = 0;
            }
            else
            {
                m_Size.store(0, std::memory_order_release);
            }
        }

        size_t IncreaseSize(size_t size) noexcept
        {
            if constexpr (std::is_same_v<T, size_t>)
            {
                size_t old_size = m_Size;
                m_Size += size;
                return old_size;
            }
            else
            {
                return m_Size.fetch_add(size, std::memory_order_acq_rel);
            }
        }

        size_t GetSize() const noexcept
        {
            if constexpr (std::is_same_v<T, size_t>)
            {
                return m_Size;
            }
            else
            {
                return m_Size.load(std::memory_order_relaxed);
            }
        }

        T m_Size;
    };

    class TransientBuffer
    {
    public:
        TransientBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator, size_t size)
            : m_Device(device), m_Allocator(allocator), m_AllocationSize(size)
        {
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = size;
            buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
            allocation_create_info.usage = vma::MemoryUsage::eAutoPreferHost;

            auto [buffer, allocation] =
                allocator->createBufferUnique(buffer_create_info, allocation_create_info);

            m_Buffer = std::move(buffer);
            m_Allocation = std::move(allocation);
        }

        TransientBuffer(const TransientBuffer&) = delete;
        TransientBuffer(TransientBuffer&&) = delete;
        TransientBuffer& operator=(const TransientBuffer&) = delete;
        TransientBuffer& operator=(TransientBuffer&&) = delete;

        bool Begin() noexcept
        {
            m_Ptr = static_cast<std::byte *>(m_Allocator->mapMemory(m_Allocation.get()));
            m_BufferSize.Reset();
            return true;
        }

        uint64_t WriteData(const void * data, size_t size) noexcept
        {
            if (!m_Ptr)
            {
                FatalPrint("Buffer is null");
                return UINT64_MAX;
            }

            uint64_t start = m_BufferSize.IncreaseSize(size);

            if (start + size > m_AllocationSize)
            {
                FatalPrint("Buffer is not big enough");
                return UINT64_MAX;
            }

            memcpy(m_Ptr + start, data, size);
            return start;
        }

        MaybeInvalid<Pair<std::byte *, uint64_t>> ReserveSpace(size_t size) noexcept
        {
            if (!m_Ptr)
            {
                FatalPrint("Buffer is null");
                return MakePair(nullptr, UINT64_MAX);
            }

            uint64_t start = m_BufferSize.IncreaseSize(size);

            if (start + size > m_AllocationSize)
            {
                FatalPrint("Buffer is not big enough");
                return MakePair(nullptr, UINT64_MAX);
            }

            return MakePair(m_Ptr + start, start);
        }

        [[nodiscard]] size_t GetSize() const noexcept
        {
            return m_BufferSize.GetSize();
        }

        void End() noexcept
        {
            m_Allocator->unmapMemory(m_Allocation.get());
            m_Ptr = nullptr;
        }

        vk::Buffer GetBuffer() const noexcept
        {
            return m_Buffer.get();
        }

        size_t GetAllocationSize() const noexcept
        {
            return m_AllocationSize;
        }

    private:

        vk::UniqueDevice & m_Device;
        vma::UniqueAllocator & m_Allocator;
        size_t m_AllocationSize;
        vma::UniqueBuffer m_Buffer;
        vma::UniqueAllocation m_Allocation;

        using BufferSizeType = std::conditional_t<Threading::NumThreads == 1, TransientBufferSize<size_t>, TransientBufferSize<std::atomic<size_t>>>;
        BufferSizeType m_BufferSize;

        std::byte * m_Ptr = nullptr;
    };
}