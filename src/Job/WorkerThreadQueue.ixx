
module;

#include <mutex>

export module YT:WorkerThread;

import :Types;
import :MultiProducerSingleConsumer;

namespace YT
{
    export class WorkerThreadQueue
    {
    public:
        WorkerThreadQueue();

        void PushWork(Function<void()> && work) noexcept;
        void TryExecuteWork() noexcept;

    private:

        std::mutex m_Mutex;
        MultiProducerSingleConsumer<Function<void()>, 2048> m_Queue;
    };
}
