
module;

#include <array>
#include <atomic>
#include <thread>
#include <semaphore>

export module YT:BackgroundTaskManager;

import :Types;
import :MultiProducerMultiConsumer;

namespace YT
{
    export class BackgroundTaskManager final
    {
    public:

        static constexpr std::size_t NumThreads = Threading::NumFileMapperThreads;

        BackgroundTaskManager();
        ~BackgroundTaskManager();

        void PushWork(Function<void()> && work);

    private:

        void ThreadMain();

    private:

        std::atomic_bool m_Running;
        MultiProducerMultiConsumer<Function<void()>, 8192> m_Queue;
        std::array<std::thread, NumThreads> m_Threads;
        std::counting_semaphore<> m_Semaphore;
    };

    BackgroundTaskManager g_BackgroundTaskManager;
}