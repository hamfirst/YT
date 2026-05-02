module;

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <array>
#include <coroutine>
#include <concepts>
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

    export class CoroBase
    {
    public:
        CoroBase() = default;
        ~CoroBase();

        void RunFromList(JobCompletionTrackingBlock * tracking_block);

        void Resume() const;

        auto get_return_object() { return this; }

        static void unhandled_exception() noexcept
        {
            FatalPrint("Unhandled exception in job coroutine");
        }

        std::suspend_always initial_suspend() noexcept;
        std::suspend_always final_suspend() noexcept;

        static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> coro) noexcept;

        void * operator new(std::size_t size) noexcept;
        void operator delete (void * ptr, std::size_t size) noexcept;

    protected:
        friend class JobManager;

        std::coroutine_handle<> m_CoroutineHandle = {};
        ThreadContextType m_CoroutineType = {};
        std::coroutine_handle<> m_ReturnHandle = {};
        ThreadContextType m_ReturnType = {};
        JobCompletionTrackingBlock * m_IncrementCounters = nullptr;
        int m_OwningThread = -1;
        bool m_Complete = false;
    };

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

    export template <typename ReturnType>
    class JobCoro : public CoroBase
    {
    public:

        JobCoro()
        {
            m_CoroutineHandle = std::coroutine_handle<JobCoro>::from_address(this);
        }

        static void return_void() noexcept
            requires (std::same_as<ReturnType, void>)
        {

        }

        template <typename ReturnValueType>
        void return_value(ReturnValueType && value) noexcept
            requires (std::constructible_from<ReturnType, ReturnValueType> && !std::same_as<ReturnType, void>)
        {
            new (&m_ReturnStorage.m_Array) ReturnType(std::forward<ReturnValueType>(value));
        }

        std::add_lvalue_reference_t<ReturnType> await_resume() noexcept
            requires (!std::same_as<ReturnType, void>)
        {
            return *reinterpret_cast<ReturnType*>(&m_ReturnStorage.m_Array);
        }

        static void await_resume() noexcept requires (std::same_as<ReturnType, void>)
        {

        }

    private:
        ReturnStorage<ReturnType> m_ReturnStorage;
    };
}

