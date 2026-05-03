module;

//import_std
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <coroutine>
#include <new>
#include <memory>
#include <type_traits>
#include <vector>
#include <array>
#include <functional>
#include <semaphore>
#include <atomic>
#include <mutex>
#include <thread>
#include <format>

export module YT:JobManager;

import :Types;
import :Coroutine;
import :FixedBlockAllocator;
import :MultiProducerMultiConsumer;
import :JobTypes;

namespace YT
{
    // Thread-safe job system using C++20 coroutines for task-based parallelism
    class JobManager
    {
    public:

        static constexpr int NumJobThreads = Threading::NumJobThreads;  ///< Number of worker threads (including main thread)

        static bool CreateJobManager() noexcept;

        /**
         * @brief Constructs a JobManager and starts worker threads.
         * 
         * Initializes thread-local state and spawns NumThreads-1 worker threads.
         * The main thread (thread 0) is used for root job execution and coordination.
         */
        JobManager();

        /**
         * @brief Destroys the JobManager and joins all worker threads.
         * 
         * Signals all worker threads to stop and waits for them to complete.
         * Any pending jobs are abandoned.
         */
        ~JobManager() noexcept;

        /**
         * @brief Prepares the job system for execution by starting worker threads.
         * 
         * Sets the running flag and releases worker threads to begin processing jobs.
         * Must be called before any jobs are pushed.
         */
        void PrepareToRunJobs();

        /**
         * @brief Stops job execution by signaling worker threads to stop processing.
         * 
         * Sets the running flag to false, causing worker threads to stop processing jobs.
         * Does not wait for in-progress jobs to complete.
         */
        void StopRunningJobs();

        void PushJob(CoroBase & coro) noexcept;

    private:

        /**
         * @brief Processes pending jobs for a specific thread.
         * 
         * @param thread_id The thread ID to process jobs for
         * @return true if a job was processed, false otherwise
         */
        bool ProcessJobList(int thread_id) noexcept;

        void ProcessExternalJobs() noexcept;

        /**
         * @brief Main loop for worker threads.
         * 
         * @param thread_id The ID of this worker thread
         */
        void JobMain(int thread_id) noexcept;

    private:
        static constexpr std::size_t CacheLineSize = 64;//std::hardware_destructive_interference_size;

        /// Structure for storing a single job in a thread's queue padded to avoid false sharing
        struct JobBlock
        {
            std::atomic<CoroBase *> m_Coroutine = nullptr;
            std::byte m_Pad[CacheLineSize - sizeof(m_Coroutine)];
        };

        std::atomic_bool m_Quit = false;                    ///< Flag to signal thread termination
        std::atomic_bool m_Running = false;                 ///< Flag indicating if jobs are being processed

        std::counting_semaphore<NumJobThreads> m_RunningSemaphore{0};  ///< Semaphore for thread synchronization

        std::vector<std::thread> m_Threads;                 ///< Worker thread handles

        MultiProducerMultiConsumer<CoroBase*, 2048> m_ExternalJobs;

        JobBlock m_Blocks[NumJobThreads][NumJobThreads];          ///< Job queues for each thread
    };

    /// Global JobManager instance
    export UniquePtr<JobManager> g_JobManager;

}
