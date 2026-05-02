
module;

#include <array>
#include <atomic>
#include <thread>
#include <semaphore>

module YT:BackgroundTaskManagerImpl;

import :BackgroundTaskManager;
import :Coroutine;

namespace YT
{
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
    }

    void BackgroundTaskManager::ThreadMain()
    {
        MakeThreadLocalCoroutineAllocator();
        while (m_Running.load(std::memory_order_relaxed))
        {
            m_Semaphore.acquire();

            Function<void()> work;
            while (m_Queue.TryDequeue(work))
            {
                work();
            }
        }
    }
}
