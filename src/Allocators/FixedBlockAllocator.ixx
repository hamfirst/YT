module;

#include <cstddef>
#include <cassert>
#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <glm/vector_relational.hpp>

export module YT:FixedBlockAllocator;

import :Types;

namespace YT
{
    /**
     * @brief A fixed-size block allocator that manages memory in blocks of type T.
     * 
     * This allocator maintains a pool of fixed-size blocks and provides fast allocation
     * and deallocation operations. It grows automatically as needed but does not shrink.
     * The allocator can be configured with a maximum size to prevent unbounded growth.
     * 
     * @tparam T The type of objects to allocate. Must be at least as large as std::size_t.
     */
    export template <typename T>
    class FixedBlockAllocator final
    {
    public:
        static_assert(sizeof(T) >= sizeof(std::size_t));

        /**
         * @brief Constructs a FixedBlockAllocator with the specified initial allocation size and maximum size.
         * 
         * @param initial_allocation The number of blocks to allocate initially.
         *                          If 0, no initial allocation is performed.
         * @param max_size The maximum number of blocks that can be allocated.
         *                 If 0, no limit is enforced.
         */
        explicit FixedBlockAllocator(std::size_t initial_allocation = 128, std::size_t max_size = 0) noexcept
            : m_MaxSize(max_size)
        {
            if (initial_allocation > 0)
            {
                Grow(initial_allocation);
                m_NextGrowSize = initial_allocation * 2;
            }
        }

        /**
         * @brief Deleted copy constructor.
         */
        FixedBlockAllocator(const FixedBlockAllocator &) = delete;

        /**
         * @brief Move constructor.
         * 
         * @param rhs The allocator to move from.
         */
        FixedBlockAllocator(FixedBlockAllocator && rhs) noexcept :
            m_Allocations(std::move(rhs.m_Allocations)),
            m_Head(rhs.m_Head),
            m_NextGrowSize(rhs.m_NextGrowSize)
        {
            rhs.m_Allocations.clear();
            rhs.m_Head = nullptr;
            rhs.m_NextGrowSize = 0;
        }

        /**
         * @brief Deleted copy assignment operator.
         */
        FixedBlockAllocator & operator=(const FixedBlockAllocator &) = delete;

        /**
         * @brief Move assignment operator.
         * 
         * @param rhs The allocator to move from.
         * @return Reference to this allocator.
         */
        FixedBlockAllocator & operator=(FixedBlockAllocator && rhs) noexcept
        {
            for (const auto& allocation : m_Allocations)
            {
                delete[] allocation.blocks;
            }

            m_Allocations = std::move(rhs.m_Allocations);
            m_Head = rhs.m_Head;
            m_NextGrowSize = rhs.m_NextGrowSize;

            rhs.m_Allocations.clear();
            rhs.m_Head = nullptr;
            rhs.m_NextGrowSize = 0;
            return *this;
        }

        /**
         * @brief Destructor. Frees all allocated memory.
         */
        ~FixedBlockAllocator()
        {
            for (const auto& allocation : m_Allocations)
            {
                delete[] allocation.blocks;
            }
        }

        /**
         * @brief Allocates a block of memory.
         * 
         * @return A pointer to the allocated block, or nullptr if allocation failed
         *         (e.g., if max_size would be exceeded).
         */
        [[nodiscard]] void * Allocate() noexcept
        {
            if (m_Head == nullptr)
            {
                size_t grow_size = m_NextGrowSize;
                if (m_MaxSize != 0)
                {
                    std::size_t allowable_size = m_MaxSize - m_TotalSize;
                    if (allowable_size > 0)
                    {
                        grow_size = std::min(grow_size, m_MaxSize - m_TotalSize);
                    }
                    else
                    {
                        return nullptr;
                    }
                }

                Grow(grow_size);
                m_NextGrowSize = std::min(m_NextGrowSize * 2, static_cast<std::size_t>(1024 * 1024)); // Cap at 1M blocks
            }

            Block * block = m_Head;
            m_Head = m_Head->m_Next;

            m_FreeCount--;

            return block->m_Data;
        }

        /**
         * @brief Frees a previously allocated block.
         * 
         * @param data The pointer to the block to free.
         * @return true if the block was successfully freed, false otherwise.
         */
        bool Free(void * data) noexcept
        {
            if (data == nullptr)
            {
                return false;
            }

            if (m_FreeCount >= m_MaxSize)
            {
                return false;
            }

            Block * block = static_cast<Block *>(data);
            block->m_Next = m_Head;
            m_Head = block;

            m_FreeCount++;
            return true;
        }

        /**
         * @brief Checks if a pointer was allocated by this allocator.
         * 
         * @param ptr The pointer to check.
         * @return true if the pointer was allocated by this allocator, false otherwise.
         */
        [[nodiscard]] bool Owns(void * ptr) const noexcept
        {
            for (const Allocation & allocation : m_Allocations)
            {
                // Calculate the start and end of the allocation
                void * start = allocation.blocks;
                void * end = static_cast<std::byte *>(start) + (sizeof(Block) * allocation.size);

                if (ptr >= start && ptr < end)
                {
                    // Check if the pointer is properly aligned to a block boundary
                    std::size_t offset = static_cast<std::byte *>(ptr) - static_cast<std::byte *>(start);
                    if (offset % sizeof(Block) == 0)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /**
         * @brief Gets the current number of free blocks.
         * 
         * @return The number of free blocks.
         */
        [[nodiscard]] std::size_t GetFreeCount() const noexcept
        {
            return m_FreeCount;
        }

    private:
        /**
         * @brief Grows the allocator by allocating more blocks.
         * 
         * @param grow_size The number of blocks to allocate.
         */
        void Grow(std::size_t grow_size) noexcept
        {
            try
            {
                std::unique_ptr<Block[]> new_blocks = std::make_unique<Block[]>(grow_size);
                m_Allocations.push_back({new_blocks.get(), grow_size});

                new_blocks[0].m_Next = m_Head;
                for (std::size_t index = 1; index < grow_size; ++index)
                {
                    new_blocks[index].m_Next = &new_blocks[index - 1];
                }

                m_Head = &new_blocks[grow_size - 1];
                m_TotalSize += grow_size;
                m_FreeCount += grow_size;
                new_blocks.release();
            }
            catch (const std::bad_alloc &)
            {
                // If allocation fails, try with a smaller size
                if (grow_size > 8)
                {
                    Grow(grow_size / 2);
                }
                else
                {
                    FatalPrint("Failed to allocate memory");
                    std::terminate();
                }
            }
            catch (...)
            {
                FatalPrint("Failed to allocate memory");
                std::terminate();
            }
        }

    private:
        /**
         * @brief A block of memory that can be allocated.
         */
        struct alignas(T) Block
        {
            union
            {
                Block * m_Next;              ///< Pointer to the next block in the free list
                alignas(T) std::byte m_Data[sizeof(T)];  ///< The actual data storage
            };
        };
        
        /**
         * @brief Represents an allocation of multiple blocks.
         */
        struct Allocation
        {
            Block * blocks;                  ///< Pointer to the array of blocks
            std::size_t size;                ///< Number of blocks in the allocation
        };

        Vector<Allocation> m_Allocations;    ///< List of all allocations
        Block * m_Head = nullptr;            ///< Head of the free list
        std::size_t m_MaxSize = 0;           ///< Maximum number of blocks that can be allocated
        std::size_t m_NextGrowSize = 8;      ///< Size of the next growth
        std::size_t m_FreeCount = 0;         ///< Number of free blocks
        std::size_t m_TotalSize = 0;         ///< Total number of blocks allocated
    };

    /**
     * @brief A thread-safe version of FixedBlockAllocator.
     * 
     * This allocator provides thread-safe allocation and deallocation operations
     * using atomic operations and mutexes. It maintains a free list of blocks
     * that can be safely accessed by multiple threads.
     * 
     * @tparam T The type of objects to allocate. Must be at least as large as std::size_t.
     */
    export template <typename T>
    class ThreadSafeFixedBlockAllocator
    {
    public:
        static_assert(sizeof(T) >= sizeof(std::size_t));

        /**
         * @brief Constructs a ThreadSafeFixedBlockAllocator with the specified initial allocation size.
         * 
         * @param initial_allocation The number of blocks to allocate initially.
         *                          If 0, no initial allocation is performed.
         */
        explicit ThreadSafeFixedBlockAllocator(std::size_t initial_allocation = 128) noexcept
        {
            if (initial_allocation > 0)
            {
                m_NextGrowSize = initial_allocation;
                Grow();
            }
        }

        /**
         * @brief Deleted copy constructor.
         */
        ThreadSafeFixedBlockAllocator(const ThreadSafeFixedBlockAllocator &) = delete;

        /**
         * @brief Deleted move constructor.
         */
        ThreadSafeFixedBlockAllocator(ThreadSafeFixedBlockAllocator && rhs) noexcept = delete;

        /**
         * @brief Deleted copy assignment operator.
         */
        ThreadSafeFixedBlockAllocator& operator=(const ThreadSafeFixedBlockAllocator &) = delete;

        /**
         * @brief Deleted move assignment operator.
         */
        ThreadSafeFixedBlockAllocator& operator=(ThreadSafeFixedBlockAllocator && rhs) noexcept = delete;

        /**
         * @brief Destructor. Frees all allocated memory.
         */
        ~ThreadSafeFixedBlockAllocator() noexcept
        {
            for (const auto& allocation : m_Allocations)
            {
                delete[] allocation.blocks;
            }
        }

        /**
         * @brief Allocates a block of memory.
         * 
         * @return A pointer to the allocated block, or nullptr if allocation failed.
         */
        [[nodiscard]] void * Allocate() noexcept
        {
            while (true)
            {
                Block * block = m_Head.load(std::memory_order_acquire);
                if (block == nullptr)
                {
                    Grow();
                    continue;
                }

                Block * next = block->m_Next.load(std::memory_order_acquire);

                if (m_Head.compare_exchange_weak(block, next, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return &block->m_Data;
                }
            }
        }

        /**
         * @brief Frees a previously allocated block.
         * 
         * @param data The pointer to the block to free.
         */
        void Free(void * data) noexcept
        {
            if (data == nullptr)
            {
                return;
            }

            Block * block = static_cast<Block *>(data);
            while (true)
            {
                Block * head = m_Head.load(std::memory_order_acquire);
                block->m_Next.store(head, std::memory_order_release);
                if (m_Head.compare_exchange_weak(head, block, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return;
                }
            }
        }

        /**
         * @brief Checks if a pointer was allocated by this allocator.
         * 
         * @param ptr The pointer to check.
         * @return true if the pointer was allocated by this allocator, false otherwise.
         */
        [[nodiscard]] bool Owns(void * ptr) noexcept
        {
            std::lock_guard lock(m_AllocationMutex);

            // Validate that the pointer is within our allocation range
            for (const Allocation & allocation : m_Allocations)
            {
                // Calculate the start and end of the allocation
                void * start = allocation.blocks;
                void * end = static_cast<std::byte *>(start) + (sizeof(Block) * allocation.size);

                if (ptr >= start && ptr < end)
                {
                    // Check if the pointer is properly aligned to a block boundary
                    std::size_t offset = static_cast<std::byte *>(ptr) - static_cast<std::byte *>(start);
                    if (offset % sizeof(Block) == 0)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

    private:
        /**
         * @brief Grows the allocator by allocating more blocks.
         */
        void Grow() noexcept
        {
            std::lock_guard lock(m_AllocationMutex);

            if (m_Head.load(std::memory_order_acquire) != nullptr)
            {
                return;
            }

            try
            {
                std::unique_ptr<Block[]> new_blocks = std::make_unique<Block[]>(m_NextGrowSize);
                m_Allocations.push_back({new_blocks.get(), m_NextGrowSize});

                for (std::size_t index = 0; index < m_NextGrowSize - 1; ++index)
                {
                    new_blocks[index].m_Next.store(&new_blocks[index + 1], std::memory_order_release);
                }

                Block * first_block = &new_blocks[0];
                Block * last_block = &new_blocks[m_NextGrowSize - 1];
                new_blocks.release();

                m_NextGrowSize = std::min(m_NextGrowSize * 2, static_cast<std::size_t>(1024 * 1024)); // Cap at 1M blocks

                while (true)
                {
                    Block * head = m_Head.load(std::memory_order_acquire);
                    last_block->m_Next.store(head, std::memory_order_release);
                    if (m_Head.compare_exchange_weak(head, first_block, std::memory_order_acq_rel, std::memory_order_acquire))
                    {
                        return;
                    }
                }
            }
            catch (std::bad_alloc &)
            {
                // If allocation fails, try with a smaller size
                if (m_NextGrowSize > 8)
                {
                    m_NextGrowSize /= 2;
                    Grow();
                }
                else
                {
                    FatalPrint("Failed to allocate memory");
                    std::terminate();
                }
            }
            catch (...)
            {
                FatalPrint("Failed to allocate memory");
                std::terminate();
            }
        }

    private:
        /**
         * @brief A block of memory that can be allocated.
         */
        struct alignas(T) Block
        {
            union
            {
                std::atomic<Block *> m_Next = nullptr;  ///< Atomic pointer to the next block in the free list
                alignas(T) std::byte m_Data[sizeof(T)];  ///< The actual data storage
            };
        };

        /**
         * @brief Represents an allocation of multiple blocks.
         */
        struct Allocation
        {
            Block * blocks;                  ///< Pointer to the array of blocks
            std::size_t size;                ///< Number of blocks in the allocation
        };

        static_assert(std::atomic<Block *>::is_always_lock_free, "Block pointer must be lock free");

        std::mutex m_AllocationMutex;        ///< Mutex for protecting allocations
        Vector<Allocation> m_Allocations;    ///< List of all allocations
        std::atomic<Block *> m_Head = nullptr; ///< Head of the free list
        std::size_t m_NextGrowSize = 8;      ///< Size of the next growth
    };

    /**
     * @brief A thread-cached version of FixedBlockAllocator.
     * 
     * This allocator maintains a thread-local cache of blocks and a global pool.
     * It provides fast allocation and deallocation for each thread while maintaining
     * memory efficiency through periodic rebalancing.
     * 
     * @tparam T The type of objects to allocate.
     * @tparam InitialAllocationPerThread The initial number of blocks to allocate per thread.
     * @tparam MaxLocalCachePerThread The maximum number of blocks to cache per thread.
     */
    export template <typename T, size_t InitialAllocationPerThread = 128, size_t MaxLocalCachePerThread = 512>
    class ThreadCachedFixedBlockAllocator
    {
    public:
        /**
         * @brief Constructs a ThreadCachedFixedBlockAllocator.
         */
        ThreadCachedFixedBlockAllocator()
        {
            m_FixedAllocators.emplace_back(MakeUnique<FixedBlockAllocator<T>>(
                InitialAllocationPerThread, MaxLocalCachePerThread));
            m_LocalAllocator = m_FixedAllocators.back().get();
        }

        /**
         * @brief Deleted copy constructor.
         */
        ThreadCachedFixedBlockAllocator(const ThreadCachedFixedBlockAllocator &) = delete;

        /**
         * @brief Deleted move constructor.
         */
        ThreadCachedFixedBlockAllocator(ThreadCachedFixedBlockAllocator && rhs) noexcept = delete;

        /**
         * @brief Deleted copy assignment operator.
         */
        ThreadCachedFixedBlockAllocator& operator=(const ThreadCachedFixedBlockAllocator &) = delete;

        /**
         * @brief Deleted move assignment operator.
         */
        ThreadCachedFixedBlockAllocator& operator=(ThreadCachedFixedBlockAllocator && rhs) noexcept = delete;

        /**
         * @brief Destructor. Frees all allocated memory.
         */
        ~ThreadCachedFixedBlockAllocator() noexcept = default;

        /**
         * @brief Allocates a block of memory.
         * 
         * @return A pointer to the allocated block, or nullptr if allocation failed.
         */
        [[nodiscard]] void * Allocate() noexcept
        {
            if (void * local_val = m_LocalAllocator->Allocate())
            {
                return local_val;
            }

            return m_ThreadAllocator.Allocate();
        }

        /**
         * @brief Frees a previously allocated block.
         * 
         * @param data The pointer to the block to free.
         */
        void Free(void * data) noexcept
        {
            if (m_LocalAllocator->Free(data))
            {
                return;
            }

            m_ThreadAllocator.Free(data);
        }

        /**
         * @brief Rebalances memory between thread-local caches.
         * 
         * This function should be called periodically to ensure efficient memory usage
         * across all threads. It redistributes free blocks between thread-local caches
         * to maintain a roughly equal number of free blocks per thread.
         */
        void RebalanceMemory() noexcept
        {
            std::lock_guard lock(m_FixedAllocatorMutex);

            size_t total_free_size = 0;
            for (UniquePtr<FixedBlockAllocator<T>> & allocator : m_FixedAllocators)
            {
                total_free_size += allocator->GetFreeCount();
            }

            Vector<void *> free_list;

            if (total_free_size == 0)
            {
                return;
            }

            size_t avg_free_size = total_free_size / m_FixedAllocators.size();

            for (UniquePtr<FixedBlockAllocator<T>> & allocator : m_FixedAllocators)
            {
                while (allocator->GetFreeCount() > avg_free_size + 1)
                {
                    if (void * ptr = allocator->Allocate())
                    {
                        free_list.push_back(ptr);
                    }
                }
            }

            for (UniquePtr<FixedBlockAllocator<T>> & allocator : m_FixedAllocators)
            {
                while (allocator->GetFreeCount() < avg_free_size && !free_list.empty())
                {
                    void * ptr = free_list.back();
                    if (allocator->Free(ptr))
                    {
                        free_list.pop_back();
                    }
                }
            }

            while (!free_list.empty())
            {
                if (!m_FixedAllocators.back()->Free(free_list.back()))
                {
                    m_ThreadAllocator.Free(free_list.back());
                }

                free_list.pop_back();
            }
        }

        /**
         * @brief Creates a local allocator for the current thread if one doesn't exist.
         */
        void MakeLocalAllocator() noexcept
        {
            if (!m_LocalAllocator)
            {
                std::lock_guard lock(m_FixedAllocatorMutex);
                m_FixedAllocators.emplace_back(MakeUnique<FixedBlockAllocator<T>>(
                    InitialAllocationPerThread, MaxLocalCachePerThread));
                m_LocalAllocator = m_FixedAllocators.back().get();
            }
        }

    private:
        static thread_local FixedBlockAllocator<T> * m_LocalAllocator;  ///< Thread-local allocator

        ThreadSafeFixedBlockAllocator<T> m_ThreadAllocator;  ///< Global thread-safe allocator
        Vector<UniquePtr<FixedBlockAllocator<T>>> m_FixedAllocators;  ///< List of all thread-local allocators

        std::mutex m_FixedAllocatorMutex;  ///< Mutex for protecting the list of allocators
        std::atomic_size_t m_NumAllocations = 0;  ///< Total number of allocations
    };

    template <typename T, size_t InitialAllocationPerThread, size_t MaxLocalCachePerThread>
    thread_local FixedBlockAllocator<T> * ThreadCachedFixedBlockAllocator<T, InitialAllocationPerThread, MaxLocalCachePerThread>::m_LocalAllocator = nullptr;
}