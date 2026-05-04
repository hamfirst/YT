
module;

#include <array>
#include <atomic>
#include <thread>
#include <semaphore>
#include <new>

module YT:BackgroundTaskManagerImpl;

import :BackgroundTaskManager;
import :Coroutine;
import :WorkerThread;
import :WorkerThreadQueue;

namespace YT
{
    bool BackgroundTaskManager::CreateBackgroundTaskManager() noexcept
    {
        if (g_BackgroundTaskManager)
        {
            return true;
        }

        try
        {
            g_BackgroundTaskManager = MakeUnique<BackgroundTaskManager>();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    BackgroundTaskManager::BackgroundTaskManager()
        : m_Running { true }, m_Semaphore { 0 }
    {
        for (std::thread & thread : m_Threads)
        {
            thread = std::thread([this]{ ThreadMain(); });
        }
    }

    BackgroundTaskManager::~BackgroundTaskManager()
    {
        m_Running = false;
        m_Semaphore.release(NumThreads);

        for (std::thread & thread : m_Threads)
        {
            thread.join();
        }
    }

    void BackgroundTaskManager::PushWork(Function<void()> && work)
    {
        while (!m_Queue.TryEnqueue(std::move(work)))
        {
            std::this_thread::yield();
        }

        SignalWork();
    }

    void BackgroundTaskManager::SignalWork()
    {
        m_Requests.fetch_add(1, std::memory_order_release);
        m_Semaphore.release();
    }

    void BackgroundTaskManager::SyncAll() const noexcept
    {
        while (true)
        {
            MonitorAddr(&m_Responses);
            if (m_Requests.load() == m_Responses.load())
            {
                break;
            }

            WaitForAddr(150000);
        }
    }


    void BackgroundTaskManager::ThreadMain()
    {
        MakeThreadLocalCoroutineAllocator();
        SetCurrentThreadContext(ThreadContextType::Background);

        while (m_Running.load(std::memory_order_relaxed))
        {
            m_Semaphore.acquire();

            Function<void()> work;
            while (m_Queue.TryDequeue(work))
            {
                work();
                m_Responses.fetch_add(1, std::memory_order_acquire);
            }

            auto ProcessWorkQueue = [&](WorkerThreadQueue & queue) -> void
            {
                SetCurrentThreadContext(queue.GetThreadContextType());
                int work_completed = queue.TryExecuteWork();

                m_Responses.fetch_add(work_completed, std::memory_order_release);
            };

            ProcessWorkQueue(g_FreeTypeThreadQueue);

            SetCurrentThreadContext(ThreadContextType::Background);
        }
    }
}
