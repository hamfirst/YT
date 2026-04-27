module;

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>

export module YT:SingleProducerSingleConsumerTests;

import :SingleProducerSingleConsumer;

namespace YT
{
    class SingleProducerSingleConsumerTest : public ::testing::Test
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

    TEST_F(SingleProducerSingleConsumerTest, EnqueueDequeueOrder)
    {
        SingleProducerSingleConsumer<int, 8> queue;

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

    TEST_F(SingleProducerSingleConsumerTest, FullAndWraparound)
    {
        SingleProducerSingleConsumer<int, 4> queue;

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

    TEST_F(SingleProducerSingleConsumerTest, ClearDestroysQueuedObjects)
    {
        LifetimeTracked::Reset();
        SingleProducerSingleConsumer<LifetimeTracked, 8> queue;

        EXPECT_TRUE(queue.Emplace(1));
        EXPECT_TRUE(queue.Emplace(2));
        EXPECT_TRUE(queue.Emplace(3));
        EXPECT_EQ(queue.Size(), 3u);

        queue.Clear();
        EXPECT_TRUE(queue.Empty());
        EXPECT_EQ(LifetimeTracked::destructor_count.load(std::memory_order_relaxed), 3);
    }

    TEST_F(SingleProducerSingleConsumerTest, ProducerConsumerConcurrency)
    {
        constexpr int count = 20000;
        SingleProducerSingleConsumer<int, 1024> queue;

        std::vector<int> consumed;
        consumed.reserve(count);

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

        std::thread consumer([&queue, &consumed]()
        {
            int value = 0;
            for (int i = 0; i < count; ++i)
            {
                while (!queue.TryDequeue(value))
                {
                    std::this_thread::yield();
                }
                consumed.push_back(value);
            }
        });

        producer.join();
        consumer.join();

        ASSERT_EQ(consumed.size(), static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i)
        {
            EXPECT_EQ(consumed[static_cast<std::size_t>(i)], i);
        }
        EXPECT_TRUE(queue.Empty());
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
