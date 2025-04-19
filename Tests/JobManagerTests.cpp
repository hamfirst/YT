module;

#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <coroutine>

module YT:JobManagerTests;

import :JobManager;
import :Types;

using namespace YT;

// Test fixture for JobManager tests
class JobManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_JobManager.PrepareToRunJobs();
    }

    void TearDown() override
    {
        // Ensure all jobs are completed
        g_JobManager.StopRunningJobs();
    }
};

// Test coroutine that increments a counter
JobCoro<void> IncrementCounter(std::atomic<int>& counter, int increment = 1)
{
    counter.fetch_add(increment, std::memory_order_relaxed);
    co_return;
}

// Test coroutine that returns a value
JobCoro<int> ReturnValue(int value)
{
    co_return value;
}

// Test coroutine that runs on main thread
JobCoroMainThread<void> MainThreadJob(std::atomic<bool>& executed)
{
    EXPECT_EQ(g_JobManager.GetThreadId(), 0);  // Should run on main thread
    executed.store(true, std::memory_order_release);
    co_return;
}

// Basic functionality tests
TEST_F(JobManagerTest, BasicJobExecution)
{
    std::atomic<int> counter{0};
    JobList<void> jobs;
    
    // Create and run a simple job
    jobs.PushJob(IncrementCounter(counter));
    jobs.WaitForCompletion();
    
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(JobManagerTest, ReturnValue)
{
    JobList<int> jobs;
    
    // Create and run a job that returns a value
    jobs.PushJob(ReturnValue(42));
    jobs.WaitForCompletion();
    
    EXPECT_EQ(jobs[0], 42);
}

TEST_F(JobManagerTest, MultipleJobs)
{
    std::atomic<int> counter{0};
    JobList<void> jobs;
    
    // Create and run multiple jobs
    for (int i = 0; i < 10; ++i)
    {
        jobs.PushJob(IncrementCounter(counter));
    }
    jobs.WaitForCompletion();
    
    EXPECT_EQ(counter.load(), 10);
}

// Thread safety tests
TEST_F(JobManagerTest, ConcurrentJobExecution)
{
    std::atomic<int> counter{0};
    JobList<void> jobs;
    
    // Create multiple jobs that will run concurrently
    for (int i = 0; i < 1000; ++i)
    {
        jobs.PushJob(IncrementCounter(counter));
    }
    jobs.WaitForCompletion();
    
    EXPECT_EQ(counter.load(), 1000);
}

TEST_F(JobManagerTest, MainThreadJobs)
{
    std::atomic<bool> executed{false};
    JobList<void> jobs;
    
    // Create and run a main thread job
    jobs.PushJob(MainThreadJob(executed));
    jobs.WaitForCompletion();
    
    EXPECT_TRUE(executed.load());
}

// Edge case tests
TEST_F(JobManagerTest, EmptyJobList)
{
    JobList<void> jobs;
    jobs.WaitForCompletion();  // Should not hang
}

// Performance tests
TEST_F(JobManagerTest, JobThroughput)
{
    const int num_jobs = 100000;
    std::atomic<int> counter{0};
    JobList<void> jobs;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create and run many jobs
    for (int i = 0; i < num_jobs; ++i)
    {
        jobs.PushJob(IncrementCounter(counter));
    }
    
    jobs.WaitForCompletion();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_EQ(counter.load(), num_jobs);
    std::cout << "Processed " << num_jobs << " jobs in " << duration.count() << "ms" << std::endl;
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
