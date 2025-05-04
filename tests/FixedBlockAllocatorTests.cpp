module;

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <random>

export module YT:FixedBlockAllocatorTests;

import :FixedBlockAllocator;
import :Types;

namespace YT::Tests
{
    class FixedBlockAllocatorTest : public ::testing::Test
    {
	public:
		FixedBlockAllocatorTest() : allocator(128, 1024) { }
    protected:

        FixedBlockAllocator<uint64_t> allocator;
    };

    // Basic functionality tests
    TEST_F(FixedBlockAllocatorTest, BasicAllocation)
    {
        void* ptr = allocator.Allocate();
        ASSERT_NE(ptr, nullptr);
        ASSERT_TRUE(allocator.Owns(ptr));
    }

    TEST_F(FixedBlockAllocatorTest, FreeAndReallocate)
    {
        void* ptr = allocator.Allocate();
        ASSERT_TRUE(allocator.Free(ptr));
        void* new_ptr = allocator.Allocate();
        ASSERT_EQ(ptr, new_ptr);
    }

    TEST_F(FixedBlockAllocatorTest, MaxSizeAllocation)
    {
        std::vector<void*> ptrs;
        for (int i = 0; i < 1024; ++i)
        {
            void* ptr = allocator.Allocate();
            ASSERT_NE(ptr, nullptr);
            ptrs.push_back(ptr);
        }
		ASSERT_EQ(allocator.Allocate(), nullptr);

        for (void* ptr : ptrs)
        {
			ASSERT_TRUE(allocator.Owns(ptr));
            ASSERT_TRUE(allocator.Free(ptr));
        }
    }

    TEST_F(FixedBlockAllocatorTest, InvalidFree)
    {
        int value = 42;
        ASSERT_FALSE(allocator.Owns(&value));
    }

    TEST_F(FixedBlockAllocatorTest, NullFree)
    {
        ASSERT_FALSE(allocator.Free(nullptr));
    }

    // Growth tests
    TEST_F(FixedBlockAllocatorTest, Growth)
    {
        std::vector<void*> ptrs;
        for (int i = 0; i < 256; ++i)
        {
            void* ptr = allocator.Allocate();
            ASSERT_NE(ptr, nullptr);
            ptrs.push_back(ptr);
        }

        for (void* ptr : ptrs)
        {
            ASSERT_TRUE(allocator.Free(ptr));
        }
    }

    // Move tests
    TEST_F(FixedBlockAllocatorTest, MoveConstructor)
    {
        void* ptr = allocator.Allocate();
        FixedBlockAllocator<uint64_t> moved(std::move(allocator));
        ASSERT_TRUE(moved.Owns(ptr));
        ASSERT_FALSE(allocator.Owns(ptr));
    }

    TEST_F(FixedBlockAllocatorTest, MoveAssignment)
    {
        void* ptr = allocator.Allocate();
        FixedBlockAllocator<uint64_t> moved(64);
        moved = std::move(allocator);
        ASSERT_TRUE(moved.Owns(ptr));
        ASSERT_FALSE(allocator.Owns(ptr));
    }

    // ThreadSafeFixedBlockAllocator tests
    class ThreadSafeFixedBlockAllocatorTest : public ::testing::Test
    {
    protected:

        ThreadSafeFixedBlockAllocator<uint64_t> allocator;
    };

    TEST_F(ThreadSafeFixedBlockAllocatorTest, BasicAllocation)
    {
        void* ptr = allocator.Allocate();
        ASSERT_NE(ptr, nullptr);
        ASSERT_TRUE(allocator.Owns(ptr));
    }

    TEST_F(ThreadSafeFixedBlockAllocatorTest, ConcurrentAllocation)
    {
        const int num_threads = 4;
        const int allocations_per_thread = 1000;
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};

        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([&]()
            {
                for (int j = 0; j < allocations_per_thread; ++j)
                {
                    void* ptr = allocator.Allocate();
                    if (ptr != nullptr)
                    {
                        success_count++;
                        allocator.Free(ptr);
                    }
                }
            });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        ASSERT_GT(success_count, 0);
    }

    TEST_F(ThreadSafeFixedBlockAllocatorTest, InvalidFree)
    {
        int value = 42;
        ASSERT_FALSE(allocator.Owns(&value));
    }

    // ThreadCachedFixedBlockAllocator tests
    class ThreadCachedFixedBlockAllocatorTest : public ::testing::Test
    {
    protected:

        ThreadCachedFixedBlockAllocator<uint64_t, 128, 512> allocator;
    };

    TEST_F(ThreadCachedFixedBlockAllocatorTest, BasicAllocation)
    {
        void* ptr = allocator.Allocate();
        ASSERT_NE(ptr, nullptr);
        allocator.Free(ptr);
    }

    TEST_F(ThreadCachedFixedBlockAllocatorTest, ThreadLocalAllocation)
    {
        const int num_threads = 4;
        const int allocations_per_thread = 1000;
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};

        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([&]()
            {
                allocator.MakeLocalAllocator();
                std::vector<void*> ptrs;
                for (int j = 0; j < allocations_per_thread; ++j)
                {
                    void* ptr = allocator.Allocate();
                    if (ptr != nullptr)
                    {
                        success_count++;
                        ptrs.push_back(ptr);
                    }
                }

                for (void* ptr : ptrs)
                {
                    allocator.Free(ptr);
                }
            });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        ASSERT_GT(success_count, 0);
    }

    TEST_F(ThreadCachedFixedBlockAllocatorTest, MemoryRebalancing)
    {
        // Allocate from multiple threads to create imbalance
        const int num_threads = 4;
        const int allocations_per_thread = 1000;
        std::vector<std::thread> threads;

        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([&]()
            {
                allocator.MakeLocalAllocator();
                std::vector<void*> ptrs;
                for (int j = 0; j < allocations_per_thread; ++j)
                {
                    void* ptr = allocator.Allocate();
                    if (ptr != nullptr)
                    {
                        ptrs.push_back(ptr);
                    }
                }

                // Free some but not all allocations to create imbalance
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, ptrs.size() - 1);

                for (int j = 0; j < ptrs.size() / 2; ++j)
                {
                    int index = dis(gen);
                    allocator.Free(ptrs[index]);
                    ptrs[index] = nullptr;
                }
            });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        // Rebalance memory
        allocator.RebalanceMemory();

        // Verify we can still allocate
        void* ptr = allocator.Allocate();
        ASSERT_NE(ptr, nullptr);
        allocator.Free(ptr);
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
