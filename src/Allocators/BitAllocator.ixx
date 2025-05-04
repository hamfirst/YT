module;

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <algorithm>

#ifdef _MSC_VER
#include <intrin.h>
#endif

export module YT:BitAllocator;

import :Types;

namespace YT
{
    /**
     * @brief Efficient bit-based resource allocator.
     * 
     * Manages a pool of resources using a bit array, where each bit represents
     * the allocation status of a single resource. Optimized for fast allocation
     * and release operations.
     * 
     * The allocator grows automatically as needed, but does not shrink.
     * Allocation is O(n) in the worst case but typically O(1) due to the
     * lowest free word optimization.
     * 
     * @note Not thread-safe. External synchronization required for concurrent access.
     */
    export class BitAllocator final
    {
    public:
        /**
         * @brief Constructs a BitAllocator with the specified initial size.
         * 
         * @param initial_size Initial number of bits to allocate (rounded up to multiple of 64)
         * @note The actual capacity will be rounded up to the next multiple of 64
         */
        explicit BitAllocator(int initial_size = 256) noexcept
        {
            m_Bits.resize((initial_size + 63) / 64, 0);
        }

        /**
         * @brief Allocates a single bit and returns its index.
         * 
         * Searches for the first free bit, starting from the last known free word.
         * If no free bits are found, the bit array is grown by one word (64 bits).
         * 
         * @return Index of the allocated bit
         * @note The returned index is guaranteed to be unique until released
         */
        std::size_t AllocateBit() noexcept
        {
            for (std::size_t index = m_LowestFreeWordGuess, num = m_Bits.size(); index < num; ++index)
            {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
                if (int bit = __builtin_ffsll(~m_Bits[index]); bit != 0)
                {
                    --bit;
                    std::size_t result = index * 64 + bit;
                    m_LowestFreeWordGuess = index;
                    m_Bits[index] |= (1LL << bit);
                    return result;
                }
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
                unsigned long bit;
                if (_BitScanForward64(&bit, ~m_Bits[index]))
                {
                    std::size_t result = index * 64 + bit;
                    m_LowestFreeWordGuess = index;
                    m_Bits[index] |= (1LL << bit);
                    return result;
                }
#else
                uint64_t word = ~m_Bits[index];
                if (word != 0)
                {
                    // Find the first set bit using De Bruijn sequence multiplication
                    constexpr uint64_t debruijn64 = 0x03f79d71b4cb0a89;
                    constexpr int index64[64] = {
                        0,  1, 48,  2, 57, 49, 28,  3,
                        61, 58, 50, 42, 38, 29, 17,  4,
                        62, 55, 59, 36, 53, 51, 43, 22,
                        45, 39, 33, 30, 24, 18, 12,  5,
                        63, 47, 56, 27, 60, 41, 37, 16,
                        54, 35, 52, 21, 44, 32, 23, 11,
                        46, 26, 40, 15, 34, 20, 31, 10,
                        25, 14, 19,  9, 13,  8,  7,  6
                    };

                    int bit = index64[((word & -word) * debruijn64) >> 58];
                    std::size_t result = index * 64 + bit;
                    m_LowestFreeWordGuess = index;
                    m_Bits[index] |= (1LL << bit);
                    return result;
                }
#endif
            }

            std::size_t word = m_Bits.size();
            m_Bits.emplace_back(1);

            return word * 64;
        }

        /**
         * @brief Releases a previously allocated bit.
         * 
         * Marks the specified bit as free and updates the lowest free word guess
         * if necessary. The bit can be reallocated by subsequent calls to AllocateBit.
         * 
         * @param bit Index of the bit to release
         * @pre bit must be a valid index returned by a previous call to AllocateBit
         * @pre bit must not have been released already
         */
        void ReleaseBit(std::size_t bit) noexcept
        {
            assert(bit < m_Bits.size() * 64);
            
            int word = bit / 64;
            bit %= 64;

            assert(m_Bits[word] & (1LL << bit));
            m_Bits[word] &= ~(1LL << bit);

            m_LowestFreeWordGuess = std::min(m_LowestFreeWordGuess, bit);
        }

    private:
        std::size_t m_LowestFreeWordGuess = 0;  ///< Optimizes allocation by tracking the lowest word with free bits
        Vector<int64_t> m_Bits;                 ///< Bit array where each bit represents an allocatable resource
    };
}
