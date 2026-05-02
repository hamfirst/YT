
module;

//import_std

#include <cassert>
#include <memory>
#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <semaphore>
#include <coroutine>

module YT:JobManagerImpl;

import :Types;
import :JobManager;
import :Coroutine;
import :WorkerThread;

namespace YT
{
    thread_local int g_NextJobID = 0;

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
    {
        SetThreadId(0);

        g_NextJobID = 0;
        for (int i = 1; i < NumJobThreads; ++i)
        {
            m_Threads.emplace_back([this, i]{ JobMain(i); });
        }
    }

    JobManager::~JobManager() noexcept
    {
        m_Quit.store(true, std::memory_order_relaxed);
        m_RunningSemaphore.release(NumJobThreads - 1);
        for (std::thread & thread : m_Threads)
        {
            thread.join();
        }
    }

    void JobManager::PrepareToRunJobs()
    {
        m_Running.store(true, std::memory_order_release);
        m_RunningSemaphore.release(NumJobThreads - 1);
    }

    void JobManager::StopRunningJobs()
    {
        m_Running.store(false, std::memory_order_release);
    }

    void JobManager::PushJob(CoroBase & coro) noexcept
    {
        if constexpr (NumJobThreads == 1)
        {
            coro.Resume();
        }
        else if (GetCurrentThreadContext() == ThreadContextType::Job || GetCurrentThreadContext() == ThreadContextType::Main)
        {
            assert(m_Running.load(std::memory_order_acquire));

            JobBlock & block = m_Blocks[g_NextJobID][GetThreadId()];

            if (CoroBase * existing_coro = block.m_Coroutine.exchange(&coro, std::memory_order_relaxed))
            {
                existing_coro->Resume();
            }

            g_NextJobID = (g_NextJobID + 1) % NumJobThreads;
        }
        else
        {
            while (!m_ExternalJobs.TryEnqueue(&coro))
            {
                std::this_thread::yield();
            }
        }
    }

    void JobManager::RunJobs(const JobCompletionTrackingBlock & tracking_block, int target) noexcept
    {
        while (true)
        {
            ProcessJobList(GetThreadId());

            // Run main thread jobs
            std::size_t completion_count = 0;
            for (int i = 0; i < NumJobThreads; ++i)
            {
                if (i == GetThreadId())
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


    bool JobManager::ProcessJobList(int thread_id) noexcept
    {
        for (int i = 0; i < NumJobThreads; ++i)
        {
            JobBlock & block = m_Blocks[thread_id][i];

            // First do a load to check if the handle exists
            if (block.m_Coroutine.load(std::memory_order_relaxed))
            {
                if (CoroBase * handle = block.m_Coroutine.exchange(nullptr, std::memory_order_acq_rel))
                {
                    handle->Resume();
                    return true;
                }
            }
        }

        return false;
    }

    void JobManager::ProcessExternalJobs() noexcept
    {
        CoroBase * coro = nullptr;
        while (m_ExternalJobs.TryDequeue(coro))
        {
            coro->Resume();
        }

        g_MainThreadQueue.TryExecuteWork();
    }

    void JobManager::JobMain(int thread_id) noexcept
    {
        SetThreadId(thread_id);
        g_NextJobID = (thread_id + 1) % NumJobThreads;

        MakeThreadLocalCoroutineAllocator();

        while (!m_Quit.load(std::memory_order_relaxed))
        {
            m_RunningSemaphore.acquire();

            int idle_count = 0;

            while (m_Running.load(std::memory_order_acquire))
            {
                if (!ProcessJobList(thread_id))
                {
                    idle_count++;

                    if (idle_count  > 5)
                    {
                        ProcessExternalJobs();
                        idle_count = 0;
                    }
                }
            }
        }
    }
}