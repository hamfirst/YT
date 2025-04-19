module;

#include <coroutine>
#include <new>
#include <functional>
#include <semaphore>
#include <atomic>
#include <mutex>
#include <thread>
#include <cassert>

extern thread_local int g_ThreadID;
extern thread_local int g_NextJobID;

export module YT:JobManager;

import :Types;

namespace YT
{
    /**
     * @brief Base data structure for job promises.
     * 
     * Contains common data shared between all job promise types.
     * Manages coroutine resumption and completion tracking.
     */
    export struct JobPromiseBaseData
    {
        std::coroutine_handle<> m_ResumeHandle = std::coroutine_handle<>();  ///< Handle to resume after this job completes
        std::atomic_size_t* m_IncrementCounter = nullptr;                    ///< Counter to increment when job completes
        bool m_IsMainThread = false;                                        ///< Whether this job must run on the main thread
    };

    // Thread-safe job system using C++20 coroutines for task-based parallelism
    export class JobManager final
    {
    public:
        /// Number of worker threads (including main thread)
        static constexpr int NumThreads = 4;

        /**
         * @brief Constructs a JobManager and starts worker threads.
         * 
         * Initializes thread-local state and spawns NumThreads-1 worker threads.
         * The main thread (thread 0) is used for root job execution and coordination.
         */
        JobManager()
            : m_RunningSemaphore{0}
        {
            g_ThreadID = 0;
            g_NextJobID = 1;
            for (int i = 1; i < NumThreads; ++i)
            {
                m_Threads.emplace_back([this, i]{ JobMain(i); });
            }
        }

        /**
         * @brief Destroys the JobManager and joins all worker threads.
         * 
         * Signals all worker threads to stop and waits for them to complete.
         * Any pending jobs are abandoned.
         */
        ~JobManager() noexcept
        {
            m_Quit.store(true, std::memory_order_relaxed);
            m_RunningSemaphore.release(NumThreads - 1);
            for (std::thread & thread : m_Threads)
            {
                thread.join();
            }
        }

        /**
         * @brief Prepares the job system for execution by starting worker threads.
         * 
         * Sets the running flag and releases worker threads to begin processing jobs.
         * Must be called before any jobs are pushed.
         */
        void PrepareToRunJobs()
        {
            m_Running.store(true, std::memory_order_release);
            m_RunningSemaphore.release(NumThreads - 1);
        }

        /**
         * @brief Stops job execution by signaling worker threads to stop processing.
         * 
         * Sets the running flag to false, causing worker threads to stop processing jobs.
         * Does not wait for in-progress jobs to complete.
         */
        void StopRunningJobs()
        {
            m_Running.store(false, std::memory_order_release);
        }

        /**
         * @brief Returns the ID of the current thread.
         * 
         * @return The thread ID (0 for main thread, 1-N for worker threads)
         */
        static int GetThreadId() noexcept
        {
            return g_ThreadID;
        }

        /**
         * @brief Pushes a job to be executed on any available thread.
         * 
         * @param handle The coroutine handle representing the job to execute
         * @pre JobManager must be running (PrepareToRunJobs() has been called)
         * @note Jobs are distributed round-robin across threads
         * @note If the target thread is busy, the job is executed locally
         */
        void PushJob(std::coroutine_handle<> handle) noexcept
        {
            assert(m_Running.load(std::memory_order_acquire));

            JobBlock & block = m_Blocks[g_NextJobID][g_ThreadID];
            std::coroutine_handle<> coro = block.m_Handle.exchange(handle, std::memory_order_acq_rel);

            if (coro)
            {
                ResumeCoroutine(coro);
            }

            g_NextJobID = (g_NextJobID + 1) % NumThreads;
        }

        /**
         * @brief Pushes a job that must execute on the main thread.
         * 
         * @param handle The coroutine handle representing the main thread job
         * @pre JobManager must be running
         */
        void PushMainThreadJob(std::coroutine_handle<> handle) noexcept
        {
            assert(m_Running.load(std::memory_order_relaxed));

            std::lock_guard lock(m_MainThreadJobMutex);
            m_MainThreadJobs.emplace_back(handle);
        }

        /**
         * @brief Executes jobs until the completion counter reaches the target value.
         * 
         * Processes jobs on the current thread and main thread jobs if running on thread 0.
         * Continues until the counter reaches the specified target value.
         * 
         * @param counter Atomic counter tracking job completions
         * @param target Target value for the counter
         * @pre JobManager must be running (PrepareToRunJobs() has been called)
         * @note Main thread jobs are only processed when called from thread 0
         */
        void RunJobs(std::atomic_size_t & counter, int target) noexcept
        {
            while (true)
            {
                ProcessJobList(g_ThreadID);

                if (g_ThreadID == 0)
                {
                    std::lock_guard lock(m_MainThreadJobMutex);
                    for (std::coroutine_handle<> & handle : m_MainThreadJobs)
                    {
                        ResumeCoroutine(handle);
                    }

                    m_MainThreadJobs.clear();
                }

                if (counter.load(std::memory_order_relaxed) == target)
                {
                    break;
                }
            }
        }

    private:
        /**
         * @brief Resumes a coroutine and handles its completion.
         * 
         * Resumes the coroutine and checks if it has completed.
         * If the coroutine has a resume handle, schedules the next job.
         * If the coroutine has completed and has a counter, increments it.
         * 
         * @param coro The coroutine handle to resume
         * @note This function is called from both PushJob and ProcessJobList
         * @note The coroutine's promise must inherit from JobPromiseBaseData
         */
        void ResumeCoroutine(std::coroutine_handle<> coro)
        {
            coro.resume();

            std::coroutine_handle<JobPromiseBaseData> handle =
                std::coroutine_handle<JobPromiseBaseData>::from_address(coro.address());

            JobPromiseBaseData & promise = handle.promise();

            if (promise.m_ResumeHandle)
            {
                std::coroutine_handle<JobPromiseBaseData> resume_handle =
                    std::coroutine_handle<JobPromiseBaseData>::from_address(promise.m_ResumeHandle.address());

                JobPromiseBaseData & resume_promise = resume_handle.promise();
                if (resume_promise.m_IsMainThread)
                {
                    PushMainThreadJob(promise.m_ResumeHandle);
                }
                else
                {
                    PushJob(promise.m_ResumeHandle);
                }
            }

            if (coro.done() && promise.m_IncrementCounter)
            {
                promise.m_IncrementCounter->fetch_add(1, std::memory_order_relaxed);
            }
        }

        /**
         * @brief Processes pending jobs for a specific thread.
         * 
         * @param thread_id The thread ID to process jobs for
         * @return true if a job was processed, false otherwise
         */
        bool ProcessJobList(int thread_id) noexcept
        {
            for (int i = 0; i < NumThreads; ++i)
            {
                JobBlock & block = m_Blocks[thread_id][i];

                // First do a load to check if the handle exists
                if (std::coroutine_handle<> handle_test = block.m_Handle.load(std::memory_order_acquire))
                {
                    if (std::coroutine_handle<> handle = block.m_Handle.exchange(nullptr, std::memory_order_acq_rel))
                    {
                        ResumeCoroutine(handle);
                        return true;
                    }
                }
            }
            return false;
        }

        /**
         * @brief Main loop for worker threads.
         * 
         * @param thread_id The ID of this worker thread
         */
        void JobMain(int thread_id) noexcept
        {
            g_ThreadID = thread_id;
            g_NextJobID = (thread_id + 1) % NumThreads;
            while (!m_Quit.load(std::memory_order_relaxed))
            {
                m_RunningSemaphore.acquire();

                int idle_count = 0;

                while (m_Running.load(std::memory_order_acquire))
                {
                    if (!ProcessJobList(thread_id))
                    {
                        idle_count++;
                        std::this_thread::yield();
                    }
                }
            }
        }

    private:
        /// Structure for storing a single job in a thread's queue
        struct JobBlock
        {
            std::atomic<std::coroutine_handle<>> m_Handle = std::coroutine_handle<>(nullptr);
            std::byte m_Pad[std::hardware_destructive_interference_size - sizeof(m_Handle)] = {};
        };

        std::atomic_bool m_Quit = false;                    ///< Flag to signal thread termination
        std::atomic_bool m_Running = false;                 ///< Flag indicating if jobs are being processed

        std::counting_semaphore<NumThreads> m_RunningSemaphore;  ///< Semaphore for thread synchronization

        std::vector<std::thread> m_Threads;                 ///< Worker thread handles

        std::mutex m_MainThreadJobMutex;                    ///< Mutex for main thread job queue
        std::vector<std::coroutine_handle<>> m_MainThreadJobs;  ///< Jobs that must run on the main thread

        JobBlock m_Blocks[NumThreads][NumThreads];          ///< Job queues for each thread
    };

    /// Global JobManager instance
    export JobManager g_JobManager;

    export template <typename ReturnType>
    class JobList;

    /**
     * @brief Promise type for jobs that return a value.
     * 
     * @tparam ReturnType The type returned by the job
     */
    export template <typename ReturnType>
    struct JobPromiseType : public JobPromiseBaseData
    {
        auto get_return_object() { return this; }

        void unhandled_exception() noexcept 
        {
            FatalPrint("Unhandled exception in job coroutine");
        }

        template <typename ReturnValueType>
        void return_value(ReturnValueType && value) noexcept
        {
            m_ReturnValue = std::forward<ReturnValueType>(value);
        }

        ReturnType & get_return_value() noexcept { return m_ReturnValue; }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        ReturnType m_ReturnValue;
    };

    /**
     * @brief Promise type specialization for void-returning jobs.
     */
    export template <>
    struct JobPromiseType<void> : public JobPromiseBaseData
    {
        auto get_return_object() { return this; }

        void unhandled_exception() noexcept {}
        void return_void() noexcept {}

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
    };

    /**
     * @brief Coroutine type for executing jobs on any thread.
     * 
     * Represents a coroutine that can be executed as a job on any available thread.
     * Supports both void and value-returning jobs.
     * 
     * @tparam ReturnType The type returned by the job (void or any other type)
     */
    export template<class ReturnType>
    class JobCoro
    {
    public:
        using promise_type = JobPromiseType<ReturnType>;

        /**
         * @brief Constructs a JobCoro from a promise.
         * 
         * @param promise The promise object associated with the coroutine
         */
        JobCoro(promise_type * promise) : m_Promise(promise) {}
        JobCoro(const JobCoro &) = delete;
        JobCoro(JobCoro && other) noexcept : m_Promise(other.m_Promise)
        {
            other.m_Promise = nullptr;
        }

        JobCoro & operator=(const JobCoro &) = delete;
        JobCoro & operator=(JobCoro && other) noexcept
        {
            m_Promise = other.m_Promise;
            other.m_Promise = nullptr;
            return *this;
        }

        /**
         * @brief Checks if the JobCoro is valid.
         * 
         * @return true if the JobCoro contains a valid promise, false otherwise
         */
        operator bool() const noexcept { return m_Promise; }

        /**
         * @brief Destroys the JobCoro and its associated coroutine.
         * 
         * Asserts that the coroutine has completed execution.
         */
        ~JobCoro() noexcept
        {
            if (m_Promise)
            {
                auto coro = std::coroutine_handle<promise_type>::from_promise(*m_Promise);
                assert(coro.done());
                coro.destroy();
            }
        }

        /**
         * @brief Runs the job from a job list.
         * 
         * @param counter Pointer to the atomic counter that tracks job completion
         * @note The job will be executed on the main thread if m_IsMainThread is true
         */
        void RunFromList(std::atomic_size_t * counter)
        {
            m_Promise->m_IncrementCounter = counter;

            if (m_Promise->m_IsMainThread)
            {
                g_JobManager.PushMainThreadJob(std::coroutine_handle<promise_type>::from_promise(*m_Promise));
            }
            else
            {
                g_JobManager.PushJob(std::coroutine_handle<promise_type>::from_promise(*m_Promise));
            }
        }

        /**
         * @brief Returns the result of the job.
         * 
         * @return The job's return value (only valid for non-void ReturnType)
         */
        std::add_lvalue_reference_t<ReturnType> GetResult() noexcept
        {
            if constexpr (!std::is_same_v<ReturnType, void>)
            {
                return m_Promise->get_return_value();
            }
        }

        bool await_ready() const noexcept { return false; }

        template <typename SuspendReturnType>
        void await_suspend(std::coroutine_handle<JobPromiseType<SuspendReturnType>> coro) noexcept
        {
            m_Promise->m_ResumeHandle = coro;
            g_JobManager.PushJob(std::coroutine_handle<promise_type>::from_promise(*m_Promise));
        }

        std::add_lvalue_reference_t<ReturnType> await_resume() noexcept
        {
            return GetResult();
        }
    protected:
        template <typename JobListReturnType>
        friend class JobList;

        promise_type * m_Promise = nullptr;
    };

    /**
     * @brief Coroutine type for executing jobs on the main thread.
     * 
     * Specialization of JobCoro that ensures jobs are executed on the main thread.
     * Can be converted to a regular JobCoro if needed.
     * 
     * @tparam ReturnType The type returned by the job (void or any other type)
     */
    export template<class ReturnType>
    class JobCoroMainThread : public JobCoro<ReturnType>
    {
    public:
        using promise_type = typename JobCoro<ReturnType>::promise_type;

        JobCoroMainThread(promise_type * promise) : JobCoro<ReturnType>(promise)
        {
            this->m_Promise->m_IsMainThread = true;
        }

        /**
         * @brief Constructs a JobCoroMainThread from a JobCoro.
         * 
         * @param other The JobCoro to convert to a main thread job
         */

        JobCoroMainThread(JobCoro<ReturnType> && other) : JobCoro<ReturnType>(std::move(other))
        {
            this->m_Promise->m_IsMainThread = true;
        }

        /**
         * @brief Converts the JobCoroMainThread back to a regular JobCoro.
         * 
         * @return A JobCoro that can run on any thread
         */
        operator JobCoro<ReturnType>() noexcept
        {
            JobCoro<ReturnType> job(this->m_Promise);
            this->m_Promise = nullptr;
            return job;
        }
    };

    /**
     * @brief Container for managing a list of jobs.
     * 
     * Provides a way to group and track the completion of multiple jobs.
     * Supports waiting for all jobs to complete and accessing their results.
     * 
     * @tparam ReturnType The type returned by the jobs in the list
     */
    template <typename ReturnType>
    class JobList
    {
    public:
        /**
         * @brief Adds a job to the list.
         * 
         * @param coro The job to add to the list
         * @note The job will be executed immediately
         */
        void PushJob(JobCoro<ReturnType> && coro) noexcept
        {
            coro.RunFromList(&m_Counter);
            m_Target++;

            m_OwnedJobs.emplace_back(std::move(coro));
        }

        /**
         * @brief Waits for all jobs in the list to complete.
         * 
         * Blocks until the completion counter reaches the target value.
         */
        void WaitForCompletion() noexcept
        {
            g_JobManager.RunJobs(m_Counter, m_Target);
        }

        /**
         * @brief Returns the result of a job at the specified index.
         * 
         * @param index The index of the job in the list
         * @return The job's return value
         * @pre The job at the specified index must have completed
         */
        std::add_lvalue_reference_t<ReturnType> operator[](std::size_t index)
        {
            return m_OwnedJobs[index].GetResult();
        }

    private:
        std::vector<JobCoro<ReturnType>> m_OwnedJobs;
        std::atomic_size_t m_Counter = 0;
        int m_Target = 0;
    };
}
