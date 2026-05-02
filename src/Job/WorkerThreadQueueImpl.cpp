
module;

//import_std

#include <thread>

module YT:WorkerThreadQueueImpl;

import :Types;
import :Coroutine;
import :WorkerThread;
import :MultiProducerSingleConsumer;

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
    }

    void WorkerThreadQueue::TryExecuteWork() noexcept
    {
        if (m_Mutex.try_lock())
        {
            ThreadContextType old_thread_context = GetCurrentThreadContext();
            SetCurrentThreadContext(m_ThreadContextType);

            Function<void()> work;
            while (m_Queue.TryDequeue(work))
            {
                work();
            }

            m_Mutex.unlock();

            SetCurrentThreadContext(old_thread_context);
        }
    }
}
