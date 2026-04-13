module;

//import_std
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <limits>
#include <atomic>
#include <format>

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
            if constexpr (std::is_same_v<T, std::size_t>)
            {
                m_Size = 0;
            }
            else
            {
                m_Size.store(0, std::memory_order_release);
            }
        }

        std::size_t IncreaseSize(std::size_t size) noexcept
        {
            if constexpr (std::is_same_v<T, std::size_t>)
            {
                std::size_t old_size = m_Size;
                m_Size += size;
                return old_size;
            }
            else
            {
                return m_Size.fetch_add(size, std::memory_order_acq_rel);
            }
        }

        std::size_t GetSize() const noexcept
        {
            if constexpr (std::is_same_v<T, std::size_t>)
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

        static constexpr std::uint64_t MaxUINT64 = std::numeric_limits<std::uint64_t>::max();

        TransientBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator, std::size_t size)
            : m_Device(device), m_Allocator(allocator), m_AllocationSize(size)
        {
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = size;
            buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped |
                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
            allocation_create_info.usage = vma::MemoryUsage::eAutoPreferHost;

            auto [allocation, buffer] =
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

        std::uint64_t WriteData(const void * data, std::size_t size) noexcept
        {
            if (!m_Ptr)
            {
                FatalPrint("Buffer is null");
                return MaxUINT64;
            }

            std::uint64_t start = m_BufferSize.IncreaseSize(size);

            if (start + size > m_AllocationSize)
            {
                FatalPrint("Buffer is not big enough");
                return MaxUINT64;
            }

            memcpy(m_Ptr + start, data, size);
            return start;
        }

        MaybeInvalid<Pair<std::byte *, std::uint64_t>> ReserveSpace(std::size_t size) noexcept
        {
            if (!m_Ptr)
            {
                FatalPrint("Buffer is null");
                return MakePair(nullptr, MaxUINT64);
            }

            std::uint64_t start = m_BufferSize.IncreaseSize(size);

            if (start + size > m_AllocationSize)
            {
                FatalPrint("Buffer is not big enough");
                return MakePair(nullptr, MaxUINT64);
            }

            return MakePair(m_Ptr + start, start);
        }

        [[nodiscard]] std::size_t GetSize() const noexcept
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

        std::size_t GetAllocationSize() const noexcept
        {
            return m_AllocationSize;
        }

    private:

        vk::UniqueDevice & m_Device;
        vma::UniqueAllocator & m_Allocator;
        std::size_t m_AllocationSize;
        vma::UniqueBuffer m_Buffer;
        vma::UniqueAllocation m_Allocation;

        using BufferSizeType = std::conditional_t<Threading::NumThreads == 1, TransientBufferSize<std::size_t>, TransientBufferSize<std::atomic<std::size_t>>>;
        BufferSizeType m_BufferSize;

        std::byte * m_Ptr = nullptr;
    };
}