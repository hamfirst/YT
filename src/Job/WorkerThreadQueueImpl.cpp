
module;

//import_std

#include <thread>

module YT:WorkerThreadQueueImpl;

import :Types;
import :Coroutine;
import :WorkerThread;
import :MultiProducerSingleConsumer;
import :BackgroundTaskManager;

namespace YT
{
    WorkerThreadQueue::WorkerThreadQueue(ThreadContextType thread_context_type)
        : m_ThreadContextType(thread_context_type)
    {

    }

    void WorkerThreadQueue::PushWork(Function<void()> && work) noexcept
    {
        while (!m_Queue.TryEnqueue(std::move(work)))
        {
            std::this_thread::yield();
        }

        if (m_ThreadContextType != ThreadContextType::Main &&
            m_ThreadContextType != ThreadContextType::Job &&
            m_ThreadContextType != ThreadContextType::FileMapper)
        {
            g_BackgroundTaskManager->SignalWork();
        }
    }

    int WorkerThreadQueue::TryExecuteWork() noexcept
    {
        int work_completed = 0;
        if (m_Mutex.try_lock())
        {
            Function<void()> work;
            while (m_Queue.TryDequeue(work))
            {
                work_completed++;
                work();
            }

            m_Mutex.unlock();
        }

        return work_completed;
    }

    ThreadContextType WorkerThreadQueue::GetThreadContextType() const noexcept
    {
        return m_ThreadContextType;
    }
}
