module;

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <mutex>
#include <iostream>

module YT:BlockTableTest;

import :BlockTable;

using namespace YT;

// Test fixture for BlockTable tests
class BlockTableTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize any common resources
    }

    void TearDown() override
    {
        // Clean up any common resources
    }
};

// Test class for tracking construction/destruction
struct TestObject
{
    static std::atomic<int> constructor_count;
    static std::atomic<int> destructor_count;
    static std::atomic<int> copy_count;
    static std::atomic<int> move_count;

    int value;

    TestObject() : value(0)
    {
        constructor_count++;
    }

    explicit TestObject(int v) : value(v)
    {
        constructor_count++;
    }

    TestObject(const TestObject& other) : value(other.value)
    {
        copy_count++;
    }

    TestObject(TestObject&& other) noexcept : value(other.value)
    {
        move_count++;
    }

    ~TestObject()
    {
        destructor_count++;
    }

    static void ResetCounters()
    {
        constructor_count = 0;
        destructor_count = 0;
        copy_count = 0;
        move_count = 0;
    }
};

std::atomic<int> TestObject::constructor_count{0};
std::atomic<int> TestObject::destructor_count{0};
std::atomic<int> TestObject::copy_count{0};
std::atomic<int> TestObject::move_count{0};

// Basic functionality tests
TEST_F(BlockTableTest, BasicAllocation)
{
    BlockTable<TestObject> table;
    TestObject::ResetCounters();

    auto handle = table.AllocateHandle();
    EXPECT_TRUE(handle);
    EXPECT_EQ(TestObject::constructor_count, 1);

    auto ptr = table.ResolveHandle(handle);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(ptr->value, 0);

    EXPECT_TRUE(table.ReleaseHandle(handle));
    EXPECT_EQ(TestObject::destructor_count, 1);
}

TEST_F(BlockTableTest, ConstructorArguments)
{
    BlockTable<TestObject> table;
    TestObject::ResetCounters();

    auto handle = table.AllocateHandle(42);
    EXPECT_TRUE(handle);

    auto ptr = table.ResolveHandle(handle);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(ptr->value, 42);
}

TEST_F(BlockTableTest, InvalidHandle)
{
    BlockTable<TestObject> table;
    
    auto ptr = table.ResolveHandle(InvalidBlockTableHandle);
    EXPECT_FALSE(ptr);

    EXPECT_FALSE(table.ReleaseHandle(InvalidBlockTableHandle));
}

TEST_F(BlockTableTest, HandleReuse)
{
    BlockTable<TestObject> table;
    TestObject::ResetCounters();

    auto handle1 = table.AllocateHandle(1);
    EXPECT_TRUE(handle1);
    EXPECT_TRUE(table.ReleaseHandle(handle1));

    auto handle2 = table.AllocateHandle(2);
    EXPECT_TRUE(handle2);
    EXPECT_NE(handle1.m_Generation, handle2.m_Generation);
}

// Memory management tests
TEST_F(BlockTableTest, Clear)
{
    BlockTable<TestObject> table;
    TestObject::ResetCounters();

    std::vector<BlockTableHandle> handles;
    for (int i = 0; i < 100; ++i)
    {
        handles.push_back(table.AllocateHandle(i));
    }

    EXPECT_EQ(TestObject::constructor_count, 100);
    table.Clear();
    EXPECT_EQ(TestObject::destructor_count, 100);

    // Verify handles are invalid after clear
    for (const auto& handle : handles)
    {
        EXPECT_FALSE(table.ResolveHandle(handle));
    }
}

// Edge case tests
TEST_F(BlockTableTest, FullTable)
{
    constexpr int small_block_size = 64;
    constexpr int small_block_count = 2;
    BlockTable<TestObject, small_block_size, small_block_count> table;
    TestObject::ResetCounters();

    std::vector<BlockTableHandle> handles;
    for (int i = 0; i < small_block_size * small_block_count; ++i)
    {
        handles.push_back(table.AllocateHandle(i));
        EXPECT_TRUE(handles.back());
    }

    // Next allocation should fail
    auto handle = table.AllocateHandle();
    EXPECT_FALSE(handle);

    // Release some handles and try again
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(table.ReleaseHandle(handles[i]));
    }

    for (int i = 0; i < 10; ++i)
    {
        handle = table.AllocateHandle();
        EXPECT_TRUE(handle);
    }
}

// Thread safety tests
TEST_F(BlockTableTest, ConcurrentAllocation)
{
    BlockTable<TestObject, 256, 256> table;
    const int num_threads = 4;
    const int allocations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::vector<std::vector<BlockTableHandle>> thread_handles(num_threads);

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&table, &thread_handles, i, allocations_per_thread]()
        {
            for (int j = 0; j < allocations_per_thread; ++j)
            {
                thread_handles[i].push_back(table.AllocateHandle(i * 1000 + j));
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Verify all allocations were successful
    for (const auto& handles : thread_handles)
    {
        for (const auto& handle : handles)
        {
            EXPECT_TRUE(handle);
            auto ptr = table.ResolveHandle(handle);
            EXPECT_TRUE(ptr);
        }
    }
}

TEST_F(BlockTableTest, ConcurrentAccess)
{
    BlockTable<TestObject> table;
    const int num_threads = 4;
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};

    // Pre-allocate some handles
    std::vector<BlockTableHandle> handles;
    for (int i = 0; i < 100; ++i)
    {
        handles.push_back(table.AllocateHandle(i));
    }

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&table, &handles, &stop, operations_per_thread]()
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, handles.size() - 1);

            for (int j = 0; j < operations_per_thread && !stop; ++j)
            {
                auto handle = handles[dis(gen)];
                auto ptr = table.ResolveHandle(handle);
                if (ptr)
                {
                    EXPECT_GE(ptr->value, 0);
                    EXPECT_LT(ptr->value, 100);
                }
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

TEST_F(BlockTableTest, VisitAllHandles)
{
    BlockTable<TestObject> table;
    TestObject::ResetCounters();

    std::vector<BlockTableHandle> handles;
    for (int i = 0; i < 100; ++i)
    {
        handles.push_back(table.AllocateHandle(i));
    }

    std::atomic<int> visit_count{0};
    table.VisitAllHandles([&visit_count](BlockTableHandle handle)
    {
        visit_count++;
    });

    EXPECT_EQ(visit_count, 100);
}

// Custom handle type test
struct CustomHandle : public BlockTableHandle
{
};

TEST_F(BlockTableTest, CustomHandleType)
{
    BlockTable<TestObject> table;
    auto handle = table.AllocateHandle();
    
    CustomHandle custom_handle = MakeCustomBlockTableHandle<CustomHandle>(handle);
    EXPECT_EQ(custom_handle.m_BlockIndex, handle.m_BlockIndex);
    EXPECT_EQ(custom_handle.m_ElemIndex, handle.m_ElemIndex);
    EXPECT_EQ(custom_handle.m_Generation, handle.m_Generation);
}

TEST_F(BlockTableTest, ConcurrentAllocationAndRelease)
{
    BlockTable<TestObject, 256, 256> table;
    const int num_threads = 4;
    const int operations_per_thread = 1000;
    const int max_handles = 1000;  // Fixed size for the handle pool
    std::vector<std::thread> threads;
    std::atomic<int> allocation_failures{0};
    std::atomic<int> release_failures{0};
    std::atomic<int> total_operations{0};
    std::atomic<int> pool_size{0};

    // Create a fixed-size handle pool using a circular buffer
    std::vector<BlockTableHandle> handle_pool(max_handles);
    std::atomic<int> head{0};
    std::atomic<int> tail{0};

    // Pre-allocate some initial handles
    for (int i = 0; i < 100; ++i)
    {
        auto handle = table.AllocateHandle(i);
        if (handle)
        {
            int current_tail = tail.load(std::memory_order_acquire);
            int next_tail = (current_tail + 1) % max_handles;
            if (next_tail != head.load(std::memory_order_acquire))
            {
                handle_pool[current_tail] = handle;
                tail.store(next_tail, std::memory_order_release);
                pool_size.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&table, &handle_pool, &head, &tail, &pool_size, 
                            &allocation_failures, &release_failures, &total_operations, 
                            operations_per_thread, max_handles]()
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 1);

            for (int j = 0; j < operations_per_thread; ++j)
            {
                if (dis(gen) == 0)
                {
                    // Try to allocate
                    auto handle = table.AllocateHandle(j);
                    if (handle)
                    {
                        int current_tail = tail.load(std::memory_order_acquire);
                        int next_tail = (current_tail + 1) % max_handles;
                        if (next_tail != head.load(std::memory_order_acquire))
                        {
                            handle_pool[current_tail] = handle;
                            tail.store(next_tail, std::memory_order_release);
                            pool_size.fetch_add(1, std::memory_order_relaxed);
                        }
                        else
                        {
                            // Pool is full, release the handle
                            table.ReleaseHandle(handle);
                        }
                    }
                    else
                    {
                        allocation_failures++;
                    }
                }
                else
                {
                    // Try to release
                    while (true)
                    {
                        int current_head = head.load(std::memory_order_acquire);
                        int current_tail = tail.load(std::memory_order_acquire);
                        
                        if (current_head == current_tail)
                        {
                            break;  // Queue is empty
                        }

                        auto handle = handle_pool[current_head];
                        int next_head = (current_head + 1) % max_handles;
                        
                        // Try to claim this slot
                        if (head.compare_exchange_strong(current_head, next_head, 
                            std::memory_order_acq_rel))
                        {
                            pool_size.fetch_sub(1, std::memory_order_relaxed);
                            
                            if (!table.ReleaseHandle(handle))
                            {
                                release_failures++;
                            }
                            break;
                        }
                    }
                }
                total_operations++;
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Log statistics
    std::cout << "Total operations: " << total_operations << std::endl;
    std::cout << "Allocation failures: " << allocation_failures << std::endl;
    std::cout << "Release failures: " << release_failures << std::endl;
    std::cout << "Final pool size: " << pool_size << std::endl;

    // Clean up remaining handles in the pool
    while (true)
    {
        int current_head = head.load(std::memory_order_acquire);
        int current_tail = tail.load(std::memory_order_acquire);
        
        if (current_head == current_tail)
        {
            break;  // Queue is empty
        }

        auto handle = handle_pool[current_head];
        int next_head = (current_head + 1) % max_handles;
        
        if (head.compare_exchange_strong(current_head, next_head, 
            std::memory_order_acq_rel))
        {
            EXPECT_TRUE(table.ReleaseHandle(handle));
        }
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
