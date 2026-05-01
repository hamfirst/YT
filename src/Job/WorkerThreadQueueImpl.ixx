
module;

//import_std

#include <thread>

module YT:WorkerThreadImpl;

import :Types;
import :WorkerThread;
import :MultiProducerSingleConsumer;

namespace YT
{
    WorkerThreadQueue::WorkerThreadQueue()
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
            Function<void()> work;
            while (m_Queue.TryDequeue(work))
            {
                work();
            }

            m_Mutex.unlock();
        }
    }
}
