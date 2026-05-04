module;

#include <cassert>
#include <coroutine>
#include <mutex>
#include <new>

export module YT:CoroEvent;

import :Types;
import :Coroutine;

namespace YT
{
    export class CoroEvent final
    {
    public:
        CoroEvent() noexcept = default;

        CoroEvent(const CoroEvent &) = delete;
        CoroEvent(CoroEvent &&) = delete;
        CoroEvent & operator=(const CoroEvent &) = delete;
        CoroEvent & operator=(CoroEvent &&) = delete;

        ~CoroEvent()
        {
            const std::scoped_lock lock(m_Mutex);
            assert(m_Waiters.empty() && "CoroEvent destroyed with pending waiters");
        }

        [[nodiscard]] bool IsTriggered() const noexcept
        {
            const std::scoped_lock lock(m_Mutex);
            return m_Triggered;
        }

        void Trigger() noexcept
        {
            Vector<CoroBase **> wake{};
            {
                const std::scoped_lock lock(m_Mutex);
                if (m_Triggered)
                {
                    return;
                }
                m_Triggered = true;
                wake.swap(m_Waiters);
            }

            for (CoroBase ** waiter : wake)
            {
                if (waiter && *waiter)
                {
                    (*waiter)->EnqueueCoroutineResume();
                }
            }
        }

        void Reset() noexcept
        {
            const std::scoped_lock lock(m_Mutex);
            m_Triggered = false;
        }

        bool TryEnqueueWaiter(CoroBase ** coro) noexcept
        {
            const std::scoped_lock lock(m_Mutex);
            if (m_Triggered)
            {
                return false;
            }
            m_Waiters.push_back(coro);
            return true;
        }

    private:
        mutable Mutex m_Mutex{};
        bool m_Triggered = false;
        Vector<CoroBase **> m_Waiters = {};
    };

    export class CoroEventWait final
    {
    public:
        explicit CoroEventWait(CoroEvent & event) noexcept
            : m_Event(&event)
        {}

        [[nodiscard]] bool await_ready() const noexcept
        {
            return m_Event->IsTriggered();
        }

        template <typename PromiseType>
        [[nodiscard]] bool await_suspend(std::coroutine_handle<PromiseType> continuation) noexcept
        {
            assert(continuation.promise().m_Coro != nullptr);
            return m_Event->TryEnqueueWaiter(&continuation.promise().m_Coro);
        }

        static void await_resume() noexcept {}

    private:
        CoroEvent * m_Event = nullptr;
    };
}
