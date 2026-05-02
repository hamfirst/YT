
module;

#include <mutex>

export module YT:WorkerThreadQueue;

import :Types;
import :JobTypes;
import :MultiProducerSingleConsumer;

namespace YT
{
    export class WorkerThreadQueue
    {
    public:
        explicit WorkerThreadQueue(ThreadContextType thread_context_type);

        void PushWork(Function<void()> && work) noexcept;
        void TryExecuteWork() noexcept;

    private:

        ThreadContextType m_ThreadContextType;
        std::mutex m_Mutex;
        MultiProducerSingleConsumer<Function<void()>, 2048> m_Queue;
    };
}
