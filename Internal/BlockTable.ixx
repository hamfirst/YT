
module;

#include <atomic>
#include <cstdint>
#include <random>

import YT.Types;

export module YT.BlockTable;

namespace YT
{
    export class BlockTableGenerationRefCount
    {
    public:

        BlockTableGenerationRefCount() noexcept = default;
        BlockTableGenerationRefCount(const BlockTableGenerationRefCount &) noexcept = delete;
        BlockTableGenerationRefCount(BlockTableGenerationRefCount &&) noexcept = delete;

        BlockTableGenerationRefCount &operator=(const BlockTableGenerationRefCount &) noexcept = delete;
        BlockTableGenerationRefCount &operator=(BlockTableGenerationRefCount &&) noexcept = delete;
        ~BlockTableGenerationRefCount() = default;

        void SetGenerationAndRefCount(uint32_t generation, uint32_t ref_count = 0) noexcept
        {
            m_Data = std::atomic<uint64_t>(ref_count) << 32 | generation;
        }

        [[nodiscard]] bool CheckGeneration(uint32_t generation) const noexcept
        {
            return (m_Data.load() & UINT32_MAX) == generation;
        }

        [[nodiscard]] uint32_t GetGeneration() const noexcept
        {
            return m_Data.load() & UINT32_MAX;
        }

        bool IncRef(uint32_t generation) noexcept
        {
            while (true)
            {
                uint64_t data = m_Data.load();
                uint64_t cur_gen = (data & UINT32_MAX);
                uint64_t cur_ref = (data >> 32) & UINT32_MAX;

                if (cur_gen != generation)
                {
                    return false;
                }

                uint64_t new_data = (cur_ref + 1) << 32 | cur_gen;

                if (m_Data.compare_exchange_strong(data, new_data))
                {
                    return true;
                }
            }
        }

        void IncRefNoValidation() noexcept
        {
            m_Data.fetch_add(UINT32_MAX + 1);
        }

        bool DecRef(uint32_t generation, uint32_t & out_ref_count) noexcept
        {
            while (true)
            {
                uint64_t data = m_Data.load();
                uint64_t cur_gen = (data & UINT32_MAX);
                uint64_t cur_ref = (data >> 32) & UINT32_MAX;

                if (cur_gen != generation)
                {
                    return false;
                }

                if (cur_ref == 0)
                {
                    return false;
                }

                cur_ref--;
                uint64_t new_data = (cur_ref << 32) | cur_gen;

                if (m_Data.compare_exchange_strong(data, new_data))
                {
                    out_ref_count = cur_ref;
                    return true;
                }
            }
        }

        bool DecRefNoValidation() noexcept
        {
            uint64_t result = m_Data.fetch_sub(UINT32_MAX + 1);

            uint64_t cur_ref = (result >> 32) & UINT32_MAX;
            return cur_ref != 0;
        }

    private:
        std::atomic_uint64_t m_Data = 0;
    };

    export struct BlockTableHandle
    {
        explicit operator bool () const noexcept { return *this != BlockTableHandle{}; }

        bool operator==(const BlockTableHandle &) const noexcept = default;
        bool operator!=(const BlockTableHandle &) const noexcept = default;

        uint16_t m_BlockIndex = 0;
        uint16_t m_ElemIndex = 0;
        uint32_t m_Generation = 0;
    };

    export constexpr BlockTableHandle InvalidBlockTableHandle = {};
    static_assert(sizeof(BlockTableHandle) == sizeof(uint64_t));

    export template <typename HandleType>
    constexpr HandleType MakeCustomBlockTableHandle(const BlockTableHandle & handle)
    {
        static_assert(sizeof(HandleType) == sizeof(BlockTableHandle));
        static_assert(std::is_base_of_v<BlockTableHandle, HandleType>);

        return HandleType{ handle };
    }

    export template <typename T, int BlockSize = 65536, int BlockCount = 2048>
    class BlockTable final
    {
    public:

        BlockTable() = default;
        BlockTable(const BlockTable &) = delete;
        BlockTable(BlockTable &&) = delete;

        BlockTable &operator=(const BlockTable &) = delete;
        BlockTable &operator=(BlockTable &&) = delete;

        ~BlockTable()
        {
            for (int block_index = 0; block_index < BlockCount; ++block_index)
            {
                if (m_Blocks[block_index])
                {
                    for (int element_index = 0; element_index < BlockSize; ++element_index)
                    {
                        int word_index = element_index / 64;
                        int bit_index = element_index % 64;

                        if (m_Blocks[block_index]->m_BlockAlloc[word_index].load() & (1 << bit_index))
                        {
                            ElementStorage & element = m_Blocks[block_index]->m_BlockData[element_index];

                            T * t = reinterpret_cast<T *>(&element.m_Buffer);
                            t->~T();
                        }
                    }

                    delete m_Blocks[block_index];
                }
            }
        }

        template <typename... Args>
        BlockTableHandle AllocateHandle(Args &&... args) noexcept
        {
            // Try to find a block with free space
            for (int block_index = 0; block_index < BlockCount; ++block_index)
            {
                // Make a new block
                if (!m_Blocks[block_index])
                {
                    std::lock_guard guard(m_BlockAllocMutex);
                    if (!m_Blocks[block_index])
                    {
                        m_Blocks[block_index] = new Block;
                    }
                }

                if (Optional<int> slot_index = ReserveSlotInBlock(block_index))
                {
                    return AssignSlot(block_index, slot_index.value(), std::forward<Args>(args)...);
                }
            }

            return InvalidBlockTableHandle;
        }

        bool ReleaseHandle(BlockTableHandle handle) noexcept
        {
            ElementStorage & element = m_Blocks[handle.m_BlockIndex]->m_BlockData[handle.m_ElemIndex];

            if (element.m_GenerationRefCount.CheckGeneration(handle.m_Generation))
            {
                element.m_GenerationRefCount.SetGenerationAndRefCount(0);

                T * t = reinterpret_cast<T *>(&element.m_Buffer);
                t->~T();

                ReleaseSlot(handle.m_BlockIndex, handle.m_ElemIndex);

                m_Blocks[handle.m_BlockIndex]->m_LowestFreeIndexHeuristic = handle.m_ElemIndex;
                return true;
            }

            return false;
        }

        OptionalPtr<T> ResolveHandle(BlockTableHandle handle) noexcept
        {
            if (handle.m_Generation == 0)
            {
                return nullptr;
            }

            ElementStorage & element = m_Blocks[handle.m_BlockIndex]->m_BlockData[handle.m_ElemIndex];
            if (element.m_GenerationRefCount.CheckGeneration(handle.m_Generation))
            {
                return reinterpret_cast<T *>(&element.m_Buffer);
            }

            return nullptr;
        }

        OptionalPtr<BlockTableGenerationRefCount> GetGenerationPointer(BlockTableHandle handle) noexcept
        {
            if (handle.m_Generation == 0)
            {
                return nullptr;
            }

            ElementStorage & element = m_Blocks[handle.m_BlockIndex]->m_BlockData[handle.m_ElemIndex];
            if (element.m_GenerationRefCount.CheckGeneration(handle.m_Generation))
            {
                return &element.m_GenerationRefCount;
            }

            return nullptr;
        }

    private:

        bool AllocateSlot(int block_index, int element_index) noexcept
        {
            std::atomic_uint64_t & alloc_bits = m_Blocks[block_index]->m_BlockAlloc[element_index / 64];
            int bit_index = element_index % 64;

            while (true)
            {
                uint64_t current_alloc_bits = alloc_bits.load();

                if (current_alloc_bits & (1 << bit_index))
                {
                    return false;
                }

                uint64_t new_alloc_bits = current_alloc_bits | (1 << bit_index);
                if (alloc_bits.compare_exchange_strong(current_alloc_bits, new_alloc_bits))
                {
                    return true;
                }
            }
        }

        void ReleaseSlot(int block_index, int element_index) noexcept
        {
            std::atomic_uint64_t & alloc_bits = m_Blocks[block_index]->m_BlockAlloc[element_index / 64];
            int bit_index = element_index % 64;

            while (true)
            {
                uint64_t current_alloc_bits = alloc_bits.load();
                uint64_t new_alloc_bits = current_alloc_bits & ~(1 << bit_index);

                if (alloc_bits.compare_exchange_strong(current_alloc_bits, new_alloc_bits))
                {
                    return;
                }
            }
        }

        template <typename... Args>
        BlockTableHandle AssignSlot(int block_index, int element_index, Args &&... args) noexcept
        {
            uint32_t generation = m_NextGeneration.fetch_add(1);

            ElementStorage & storage = m_Blocks[block_index]->m_BlockData[element_index];

            storage.m_GenerationRefCount.SetGenerationAndRefCount(generation, 0);

            void * storage_address = &storage.m_Buffer;
            new (storage_address) T(std::forward<Args>(args)...);

            return BlockTableHandle
            {
                .m_BlockIndex = static_cast<uint16_t>(block_index),
                .m_ElemIndex = static_cast<uint16_t>(element_index),
                .m_Generation = generation,
            };
        }

        Optional<int> FindSlotInBlock(int block_index) noexcept
        {
            auto CheckWord = [&](int word) -> Optional<int>
            {
                uint64_t alloc_bits = m_Blocks[block_index]->m_BlockAlloc[word];

                if (alloc_bits != UINT64_MAX)
                {
                    int bit_index = std::countr_one(alloc_bits);

                    int element_index = bit_index + word * 64;
                    if (AllocateSlot(block_index, element_index))
                    {
                        return element_index;
                    }
                }

                return {};
            };

            int search_start = m_Blocks[block_index]->m_LowestFreeIndexHeuristic.load();
            int word_start = search_start / 64;

            for (int word = word_start; word < BlockSize / 64; ++word)
            {
                if (auto slot = CheckWord(word))
                {
                    return slot;
                }
            }

            for (int word = 0; word < word_start; ++word)
            {
                if (auto slot = CheckWord(word))
                {
                    return slot;
                }
            }

            return {};
        }

        Optional<int> ReserveSlotInBlock(int block_index) noexcept
        {
            if (m_Blocks[block_index]->m_FreeCount.fetch_sub(1) > 0)
            {
                // Search for a free element
                if (Optional<int> element_index = FindSlotInBlock(block_index))
                {
                    m_Blocks[block_index]->m_LowestFreeIndexHeuristic = element_index.value() + 1;
                    return element_index;
                }

                // Somehow we had a free slot according to the free count, but it got snatched up?
                m_Blocks[block_index]->m_FreeCount.fetch_add(1);
            }
            else
            {
                m_Blocks[block_index]->m_FreeCount.fetch_add(1);
            }

            return {};
        }

    private:

        struct ElementStorage
        {
            BlockTableGenerationRefCount m_GenerationRefCount;
            alignas(T) std::byte m_Buffer[sizeof(T)];
        };

        struct Block
        {
            std::atomic_int m_LowestFreeIndexHeuristic = 0;
            std::atomic_int m_FreeCount = BlockSize;
            std::atomic_uint64_t m_BlockAlloc[(BlockSize + 63) / 64];
            ElementStorage m_BlockData[BlockSize];
        };

        std::mutex m_BlockAllocMutex;
        Block* m_Blocks[BlockCount] = { };
        std::atomic_uint32_t m_NextGeneration = 1;

    };
}

