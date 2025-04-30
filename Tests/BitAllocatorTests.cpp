module;

#include <gtest/gtest.h>
#include <vector>
#include <random>
#include <algorithm>

export module YT:BitAllocatorTests;

import :BitAllocator;

using namespace YT;

class BitAllocatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset the allocator before each test
        allocator = BitAllocator(256);
    }

    BitAllocator allocator;
};

// Basic functionality tests
TEST_F(BitAllocatorTest, BasicAllocation)
{
    // Allocate a single bit
    std::size_t bit1 = allocator.AllocateBit();
    EXPECT_EQ(bit1, 0); // First allocation should be bit 0

    // Allocate another bit
    std::size_t bit2 = allocator.AllocateBit();
    EXPECT_EQ(bit2, 1); // Second allocation should be bit 1
}

TEST_F(BitAllocatorTest, ReleaseAndReallocate)
{
    // Allocate and release a bit
    std::size_t bit = allocator.AllocateBit();
    allocator.ReleaseBit(bit);

    // Reallocate the same bit
    std::size_t reallocated = allocator.AllocateBit();
    EXPECT_EQ(reallocated, bit); // Should get the same bit back
}

TEST_F(BitAllocatorTest, SequentialAllocation)
{
    // Allocate all bits in a word (64 bits)
    std::vector<std::size_t> bits;
    for (int i = 0; i < 64; ++i)
    {
        bits.push_back(allocator.AllocateBit());
    }

    // Verify all bits are unique and sequential
    std::sort(bits.begin(), bits.end());
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_EQ(bits[i], i);
    }
}

// Edge case tests
TEST_F(BitAllocatorTest, Growth)
{
    // Allocate more bits than initial capacity
    const int total_bits = 300; // More than initial 256
    std::vector<std::size_t> bits;
    for (int i = 0; i < total_bits; ++i)
    {
        bits.push_back(allocator.AllocateBit());
    }

    // Verify all bits are unique
    std::sort(bits.begin(), bits.end());
    for (int i = 0; i < total_bits; ++i)
    {
        EXPECT_EQ(bits[i], i);
    }
}

TEST_F(BitAllocatorTest, ReleaseAll)
{
    // Allocate and release all bits in a word
    std::vector<std::size_t> bits;
    for (int i = 0; i < 64; ++i)
    {
        bits.push_back(allocator.AllocateBit());
    }

    // Release all bits
    for (std::size_t bit : bits)
    {
        allocator.ReleaseBit(bit);
    }

    // Reallocate and verify we get the same bits back
    std::vector<std::size_t> reallocated;
    for (int i = 0; i < 64; ++i)
    {
        reallocated.push_back(allocator.AllocateBit());
    }

    std::sort(reallocated.begin(), reallocated.end());
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_EQ(reallocated[i], i);
    }
}

// Performance tests
TEST_F(BitAllocatorTest, LargeAllocation)
{
    const int total_bits = 10000;
    std::vector<std::size_t> bits;
    bits.reserve(total_bits);

    // Allocate a large number of bits
    for (int i = 0; i < total_bits; ++i)
    {
        bits.push_back(allocator.AllocateBit());
    }

    // Verify all bits are unique
    std::sort(bits.begin(), bits.end());
    for (int i = 0; i < total_bits; ++i)
    {
        EXPECT_EQ(bits[i], i);
    }
}

TEST_F(BitAllocatorTest, RandomAllocationRelease)
{
    const int total_bits = 1000;
    std::vector<std::size_t> bits;
    bits.reserve(total_bits);

    // Allocate initial bits
    for (int i = 0; i < total_bits; ++i)
    {
        bits.push_back(allocator.AllocateBit());
    }

    // Randomly release and reallocate bits
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, total_bits - 1);

    for (int i = 0; i < 10000; ++i)
    {
        int index = dis(gen);
        if (bits[index] != static_cast<std::size_t>(-1))
        {
            allocator.ReleaseBit(bits[index]);
            bits[index] = static_cast<std::size_t>(-1);
        }
        else
        {
            bits[index] = allocator.AllocateBit();
        }
    }

    // Verify no duplicates in final state
    std::vector<std::size_t> active_bits;
    for (std::size_t bit : bits)
    {
        if (bit != static_cast<std::size_t>(-1))
        {
            active_bits.push_back(bit);
        }
    }
    std::sort(active_bits.begin(), active_bits.end());
    EXPECT_EQ(std::unique(active_bits.begin(), active_bits.end()), active_bits.end());
}

// Death tests for invalid operations
TEST_F(BitAllocatorTest, DeathTest_ReleaseInvalidBit)
{
    // Attempt to release a bit that was never allocated
    EXPECT_DEATH(allocator.ReleaseBit(1000), "");
}

TEST_F(BitAllocatorTest, DeathTest_DoubleRelease)
{
    // Allocate and release a bit
    std::size_t bit = allocator.AllocateBit();
    allocator.ReleaseBit(bit);

    // Attempt to release it again
    EXPECT_DEATH(allocator.ReleaseBit(bit), "");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
