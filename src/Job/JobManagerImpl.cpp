
module;

#include <cassert>
#include <thread>
#include <mutex>
#include <semaphore>
#include <coroutine>

thread_local int g_ThreadID = -1;
thread_local int g_NextJobID = 0;

module YT:JobManagerImpl;

import :Types;
import :JobManager;

namespace YT
{
    bool JobManager::CreateJobManager() noexcept
    {
        if (g_JobManager)
        {
            return true;
        }

        try
        {
            g_JobManager = MakeUnique<JobManager>();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    JobManager::JobManager()
                : m_RunningSemaphore{0}
    {
        g_ThreadID = 0;
        g_NextJobID = 0;
        for (int i = 1; i < NumThreads; ++i)
        {
            m_Threads.emplace_back([this, i]{ JobMain(i); });
        }
    }

    JobManager::~JobManager() noexcept
    {
        m_Quit.store(true, std::memory_order_relaxed);
        m_RunningSemaphore.release(NumThreads - 1);
        for (std::thread & thread : m_Threads)
        {
            thread.join();
        }
    }

    void JobManager::PrepareToRunJobs()
    {
        m_Running.store(true, std::memory_order_release);
        m_RunningSemaphore.release(NumThreads - 1);
    }

    void JobManager::StopRunningJobs()
    {
        m_Running.store(false, std::memory_order_release);
    }

    int JobManager::GetThreadId() noexcept
    {
        return g_ThreadID;
    }

    void JobManager::PushJob(std::coroutine_handle<> handle) noexcept
    {
        if constexpr (NumThreads == 1)
        {
            ResumeCoroutine(handle);
        }
        else
        {
            assert(m_Running.load(std::memory_order_acquire));

            JobBlock & block = m_Blocks[g_NextJobID][g_ThreadID];
            std::coroutine_handle<> coro = block.m_Handle.exchange(handle, std::memory_order_relaxed);

            if (coro)
            {
                ResumeCoroutine(coro);
            }

            g_NextJobID = (g_NextJobID + 1) % NumThreads;
        }
    }

    void JobManager::PushMainThreadJob(std::coroutine_handle<> handle) noexcept
    {
        assert(m_Running.load(std::memory_order_relaxed));

        std::lock_guard lock(m_MainThreadJobMutex);
        m_MainThreadJobs.emplace_back(handle);
    }

    void JobManager::RunJobs(const JobCompletionTrackingBlock & tracking_block, int target) noexcept
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

            std::size_t completion_count = 0;
            for (int i = 0; i < NumThreads; ++i)
            {
                if (i == g_ThreadID)
                {
                    completion_count += tracking_block[i].m_LocalCount;
                }
                else
                {
                    completion_count += tracking_block[i].m_RemoteCount.load(std::memory_order_relaxed);
                }
            }

            if (completion_count == target)
            {
                break;
            }
        }
    }

    void JobManager::ResumeCoroutine(std::coroutine_handle<> coro)
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

        if (coro.done())
        {
            if (promise.m_IncrementCounters)
            {
                if (promise.m_OwningThread == g_ThreadID)
                {
                    (*promise.m_IncrementCounters)[g_ThreadID].m_LocalCount++;
                }
                else
                {
                    (*promise.m_IncrementCounters)[g_ThreadID].m_RemoteCount.fetch_add(1, std::memory_order_relaxed);
                }
            }

            if (promise.m_ReturnVoid)
            {
                coro.destroy();
            }
        }
    }

    bool JobManager::ProcessJobList(int thread_id) noexcept
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

    void JobManager::JobMain(int thread_id) noexcept
    {
        g_ThreadID = thread_id;
        g_NextJobID = (thread_id + 1) % NumThreads;

        g_JobManagerAllocator.MakeLocalAllocator();

        while (!m_Quit.load(std::memory_order_relaxed))
        {
            m_RunningSemaphore.acquire();

            int idle_count = 0;

            while (m_Running.load(std::memory_order_acquire))
            {
                if (!ProcessJobList(thread_id))
                {
                    idle_count++;
                }
            }
        }
    }
}