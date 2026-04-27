module;

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>

export module YT:SingleProducerMultiConsumerTests;

import :SingleProducerMultiConsumer;

namespace YT
{
    class SingleProducerMultiConsumerTest : public ::testing::Test
    {
    };

    struct LifetimeTracked
    {
        static std::atomic<int> destructor_count;
        int value = 0;

        LifetimeTracked() = default;
        explicit LifetimeTracked(int v) : value(v) {}

        LifetimeTracked(const LifetimeTracked&) = default;
        LifetimeTracked(LifetimeTracked&&) noexcept = default;
        LifetimeTracked& operator=(const LifetimeTracked&) = default;
        LifetimeTracked& operator=(LifetimeTracked&&) noexcept = default;

        ~LifetimeTracked()
        {
            destructor_count.fetch_add(1, std::memory_order_relaxed);
        }

        static void Reset()
        {
            destructor_count.store(0, std::memory_order_relaxed);
        }
    };

    std::atomic<int> LifetimeTracked::destructor_count = 0;

    TEST_F(SingleProducerMultiConsumerTest, EnqueueDequeueOrder)
    {
        SingleProducerMultiConsumer<int, 8> queue;

        EXPECT_TRUE(queue.Empty());
        EXPECT_FALSE(queue.Full());
        EXPECT_EQ(queue.Size(), 0u);

        EXPECT_TRUE(queue.TryEnqueue(1));
        EXPECT_TRUE(queue.TryEnqueue(2));
        EXPECT_TRUE(queue.TryEnqueue(3));
        EXPECT_EQ(queue.Size(), 3u);

        int value = 0;
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 1);
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 2);
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 3);

        EXPECT_FALSE(queue.TryDequeue(value));
        EXPECT_TRUE(queue.Empty());
    }

    TEST_F(SingleProducerMultiConsumerTest, FullAndWraparound)
    {
        SingleProducerMultiConsumer<int, 4> queue;

        EXPECT_TRUE(queue.TryEnqueue(10));
        EXPECT_TRUE(queue.TryEnqueue(11));
        EXPECT_TRUE(queue.TryEnqueue(12));
        EXPECT_TRUE(queue.TryEnqueue(13));
        EXPECT_TRUE(queue.Full());
        EXPECT_FALSE(queue.TryEnqueue(99));

        int value = 0;
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 10);
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 11);

        EXPECT_TRUE(queue.TryEnqueue(20));
        EXPECT_TRUE(queue.TryEnqueue(21));
        EXPECT_TRUE(queue.Full());

        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 12);
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 13);
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 20);
        EXPECT_TRUE(queue.TryDequeue(value));
        EXPECT_EQ(value, 21);
        EXPECT_FALSE(queue.TryDequeue(value));
    }

    TEST_F(SingleProducerMultiConsumerTest, ClearDestroysQueuedObjects)
    {
        LifetimeTracked::Reset();
        SingleProducerMultiConsumer<LifetimeTracked, 8> queue;

        EXPECT_TRUE(queue.Emplace(1));
        EXPECT_TRUE(queue.Emplace(2));
        EXPECT_TRUE(queue.Emplace(3));
        EXPECT_EQ(queue.Size(), 3u);

        queue.Clear();
        EXPECT_TRUE(queue.Empty());
        EXPECT_EQ(LifetimeTracked::destructor_count.load(std::memory_order_relaxed), 3);
    }

    TEST_F(SingleProducerMultiConsumerTest, ProducerMultiConsumerConcurrency)
    {
        constexpr int count = 30000;
        constexpr int num_consumers = 4;

        SingleProducerMultiConsumer<int, 2048> queue;
        std::vector<std::atomic<int>> seen(static_cast<std::size_t>(count));
        for (auto & value : seen)
        {
            value.store(0, std::memory_order_relaxed);
        }

        std::atomic<int> consumed_count = 0;

        std::thread producer([&queue]()
        {
            for (int i = 0; i < count; ++i)
            {
                while (!queue.TryEnqueue(i))
                {
                    std::this_thread::yield();
                }
            }
        });

        std::vector<std::thread> consumers;
        consumers.reserve(num_consumers);

        for (int i = 0; i < num_consumers; ++i)
        {
            consumers.emplace_back([&queue, &seen, &consumed_count]()
            {
                while (consumed_count.load(std::memory_order_acquire) < count)
                {
                    int value = 0;
                    if (!queue.TryDequeue(value))
                    {
                        std::this_thread::yield();
                        continue;
                    }

                    ASSERT_GE(value, 0);
                    ASSERT_LT(value, count);
                    seen[static_cast<std::size_t>(value)].fetch_add(1, std::memory_order_relaxed);
                    consumed_count.fetch_add(1, std::memory_order_release);
                }
            });
        }

        producer.join();
        for (auto & consumer : consumers)
        {
            consumer.join();
        }

        EXPECT_EQ(consumed_count.load(std::memory_order_acquire), count);
        EXPECT_TRUE(queue.Empty());

        for (int i = 0; i < count; ++i)
        {
            EXPECT_EQ(seen[static_cast<std::size_t>(i)].load(std::memory_order_relaxed), 1);
        }
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
