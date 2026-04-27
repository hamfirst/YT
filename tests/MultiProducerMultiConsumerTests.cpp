module;

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>

export module YT:MultiProducerMultiConsumerTests;

import :MultiProducerMultiConsumer;

namespace YT
{
    class MultiProducerMultiConsumerTest : public ::testing::Test
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

    TEST_F(MultiProducerMultiConsumerTest, EnqueueDequeueOrder)
    {
        MultiProducerMultiConsumer<int, 8> queue;

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

    TEST_F(MultiProducerMultiConsumerTest, FullAndWraparound)
    {
        MultiProducerMultiConsumer<int, 4> queue;

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

    TEST_F(MultiProducerMultiConsumerTest, ClearDestroysQueuedObjects)
    {
        LifetimeTracked::Reset();
        MultiProducerMultiConsumer<LifetimeTracked, 8> queue;

        EXPECT_TRUE(queue.Emplace(1));
        EXPECT_TRUE(queue.Emplace(2));
        EXPECT_TRUE(queue.Emplace(3));
        EXPECT_EQ(queue.Size(), 3u);

        queue.Clear();
        EXPECT_TRUE(queue.Empty());
        EXPECT_EQ(LifetimeTracked::destructor_count.load(std::memory_order_relaxed), 3);
    }

    TEST_F(MultiProducerMultiConsumerTest, MultiProducerMultiConsumerConcurrency)
    {
        constexpr int producer_count = 4;
        constexpr int consumer_count = 4;
        constexpr int values_per_producer = 10000;
        constexpr int total_values = producer_count * values_per_producer;

        MultiProducerMultiConsumer<int, 4096> queue;
        std::vector<std::atomic<int>> seen(static_cast<std::size_t>(total_values));
        for (auto & value : seen)
        {
            value.store(0, std::memory_order_relaxed);
        }

        std::atomic<int> consumed_count = 0;
        std::atomic<int> producers_finished = 0;

        std::vector<std::thread> producers;
        producers.reserve(producer_count);

        for (int producer_index = 0; producer_index < producer_count; ++producer_index)
        {
            producers.emplace_back([&queue, producer_index, &producers_finished]()
            {
                const int start = producer_index * values_per_producer;
                const int end = start + values_per_producer;
                for (int value = start; value < end; ++value)
                {
                    while (!queue.TryEnqueue(value))
                    {
                        std::this_thread::yield();
                    }
                }

                producers_finished.fetch_add(1, std::memory_order_release);
            });
        }

        std::vector<std::thread> consumers;
        consumers.reserve(consumer_count);

        for (int consumer_index = 0; consumer_index < consumer_count; ++consumer_index)
        {
            consumers.emplace_back([&queue, &seen, &consumed_count, &producers_finished]()
            {
                while (true)
                {
                    int value = 0;
                    if (queue.TryDequeue(value))
                    {
                        ASSERT_GE(value, 0);
                        ASSERT_LT(value, total_values);
                        seen[static_cast<std::size_t>(value)].fetch_add(1, std::memory_order_relaxed);
                        consumed_count.fetch_add(1, std::memory_order_release);
                        continue;
                    }

                    if (producers_finished.load(std::memory_order_acquire) == producer_count &&
                        queue.Empty())
                    {
                        return;
                    }

                    std::this_thread::yield();
                }
            });
        }

        for (auto & producer : producers)
        {
            producer.join();
        }

        for (auto & consumer : consumers)
        {
            consumer.join();
        }

        EXPECT_EQ(consumed_count.load(std::memory_order_acquire), total_values);
        EXPECT_TRUE(queue.Empty());
        for (int i = 0; i < total_values; ++i)
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
