module;

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <array>
#include <coroutine>
#include <concepts>

export module YT:Coroutine;

import :Types;

namespace YT
{
    export enum class ThreadContextType
    {
        Unknown,
        Main,
        Job,
        Background,
    };

    export ThreadContextType GetCurrentThreadContext() noexcept;

    export struct JobCompletionTrackingElement
    {
        std::size_t m_LocalCount = 0;
        std::atomic_size_t m_RemoteCount = 0;
        std::byte m_Pad[std::hardware_destructive_interference_size - sizeof(m_LocalCount) - sizeof(m_RemoteCount)] = {};
    };

    using JobCompletionTrackingBlock = std::array<JobCompletionTrackingElement, Threading::NumThreads>;

    export class JobCoroBase
    {
    public:
        JobCoroBase() = default;

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

    private:
        std::coroutine_handle<> m_ReturnHandle = {};
    };

    export template <typename ReturnType>
    class JobCoro : public JobCoroBase
    {
    public:

        template <typename ReturnValueType>
        void return_value(ReturnValueType && value) noexcept requires (std::constructible_from<ReturnType, ReturnValueType>)
        {
            new (&m_ReturnStorage) ReturnType(std::forward<ReturnValueType>(value));
        }

        std::add_lvalue_reference_t<ReturnType> await_resume() noexcept
        {
            return *reinterpret_cast<ReturnType*>(&m_ReturnStorage);
        }

    private:
        alignas(ReturnType) std::byte m_ReturnStorage[sizeof(ReturnType)] = {};
    };

    export template <>
    class JobCoro<void>
    {
    public:
        static void return_void() noexcept {}
    };
}

