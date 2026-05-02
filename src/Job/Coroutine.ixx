module;

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <array>
#include <coroutine>
#include <concepts>
#include <mutex>
#include <type_traits>

export module YT:Coroutine;

import :Types;
import :JobTypes;

namespace YT
{
    export int GetThreadId() noexcept;
    export void SetThreadId(int thread_id) noexcept;

    export ThreadContextType GetCurrentThreadContext() noexcept;
    export void SetCurrentThreadContext(ThreadContextType context) noexcept;

    export void MakeThreadLocalCoroutineAllocator() noexcept;

    void * AllocateCoroutine(std::size_t size) noexcept;
    void FreeCoroutine(void * ptr, std::size_t size) noexcept;

    export template <typename ReturnType>
    struct ReturnStorage
    {
        alignas(ReturnType) std::byte m_Array[sizeof(ReturnType)] = {};
    };

    template <>
    struct ReturnStorage<void>
    {
        std::byte m_Array[1] = {};
    };

    class CoroBase
    {
    public:

        void Resume() const noexcept;
        void RunFromList(JobCompletionTrackingBlock * tracking_block) noexcept;

        void RunSynchronous();

    protected:

        void ScheduleForward() noexcept;
        void ScheduleBackward() noexcept;

    private:

        void Schedule(ThreadContextType thread_context, std::coroutine_handle<> coroutine) noexcept;

    protected:
        friend class JobManager;

        std::coroutine_handle<> m_CoroutineHandle = {};
        ThreadContextType m_CoroutineType = {};
        std::coroutine_handle<> m_ReturnHandle = {};
        ThreadContextType m_ReturnType = {};
        JobCompletionTrackingBlock * m_IncrementCounters = nullptr;
        int m_OwningThread = -1;
        Mutex * m_Mutex = nullptr;
        bool m_Started : 1 = false;
        bool m_Complete : 1 = false;
    };

    export template <typename ReturnType, ThreadContextType ThreadType>
    class Coro : public CoroBase
    {
    public:

        static_assert(ThreadType != ThreadContextType::Unknown, "ThreadType must be ThreadContextType::Unknown");

        struct promise_type
        {
            Coro * m_Coro = nullptr;
            bool m_HasReturnValue = false;
            ReturnStorage<ReturnType> m_ReturnStorage;

            ~promise_type()
            {
                if constexpr (!std::is_same_v<ReturnType, void>)
                {
                    if (m_HasReturnValue)
                    {
                        ReturnType * result = reinterpret_cast<ReturnType *>(&m_ReturnStorage.m_Array[0]);
                        result->~ReturnType();
                    }
                }
            }

            static void unhandled_exception() noexcept
            {
                FatalPrint("Unhandled exception in coroutine");
            }

            auto get_return_object() noexcept { return Coro{this}; }
            static auto get_return_object_on_allocation_failure() noexcept { return Coro(); }

            static std::suspend_always initial_suspend() noexcept { return {}; }
            static std::suspend_always final_suspend() noexcept { return {}; }

            // void return_void() const noexcept
            //     requires (std::same_as<ReturnType, void>)
            // {
            //     m_Coro->ScheduleBackward();
            // }

            template <typename ReturnValueType>
            void return_value(ReturnValueType && value) noexcept
                requires (std::constructible_from<ReturnType, ReturnValueType> && !std::same_as<ReturnType, void>)
            {
                new (&m_ReturnStorage.m_Array) ReturnType(std::forward<ReturnValueType>(value));

                m_Coro->ScheduleBackward();
            }

            void * operator new(std::size_t size) noexcept { return AllocateCoroutine(size); }
            void operator delete (void * ptr, std::size_t size) noexcept { FreeCoroutine(ptr, size); }
        };

        Coro()
        {
            FatalPrint("Failed to allocate coroutine");
        }

        explicit Coro(promise_type * promise) noexcept
            : m_Promise(promise)
        {
            m_Promise->m_Coro = this;
            m_CoroutineHandle = std::coroutine_handle<promise_type>::from_promise(*promise);
            m_CoroutineType = ThreadType;
        }

        ~Coro()
        {
            if (m_CoroutineHandle)
            {
                m_CoroutineHandle.destroy();
            }
        }

        ReturnType GetResult() noexcept
        {
            ReturnType * result = reinterpret_cast<ReturnType *>(&m_Promise->m_ReturnStorage.m_Array);
            return std::move(*result);
        }

        static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> coro) noexcept
        {
            m_ReturnHandle = coro;
            m_ReturnType = GetCurrentThreadContext();
            ScheduleForward();
        }

        ReturnType await_resume() noexcept
            requires(std::move_constructible<ReturnType> && !std::same_as<ReturnType, void>)
        {
            ReturnType * result = reinterpret_cast<ReturnType *>(&m_Promise->m_ReturnStorage.m_Array);
            return std::move(*result);
        }

        static void await_resume() noexcept
            requires(std::same_as<ReturnType, void>)
        {

        }

    protected:

        promise_type * m_Promise = nullptr;
    };

    export template <typename ReturnType = void>
    using JobCoro = Coro<ReturnType, ThreadContextType::Job>;

    export template <typename ReturnType = void>
    using BackgroundTask = Coro<ReturnType, ThreadContextType::Background>;
}

