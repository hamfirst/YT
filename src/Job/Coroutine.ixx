module;

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <atomic>
#include <thread>
#include <array>
#include <coroutine>
#include <concepts>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>

export module YT:Coroutine;

import :Types;
import :JobTypes;
import :Wait;

namespace YT
{
    export ThreadContextType GetCurrentThreadContext() noexcept;
    export void SetCurrentThreadContext(ThreadContextType context) noexcept;

    export void MakeThreadLocalCoroutineAllocator() noexcept;

    void * AllocateCoroutine(std::size_t size, std::align_val_t alignment) noexcept;
    void FreeCoroutine(void * ptr, std::size_t size) noexcept;

    void RunMainThreadJobsIfNeeded();
    void ExecuteSynchronousCoroutineIfNeeded() noexcept;

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
        CoroBase() = default;
        CoroBase(const CoroBase &) = delete;
        CoroBase(CoroBase && rhs) noexcept;

        CoroBase & operator= (const CoroBase &) = delete;
        CoroBase & operator= (CoroBase && rhs) noexcept;

        ~CoroBase();

        void Resume() const noexcept;
        void RunFromList(JobCompletionTrackingInfo * tracking_block) noexcept;

        void RunSynchronous();
        void EnqueueCoroutineResume() noexcept;

    protected:

        void ScheduleForward() noexcept;
        void ScheduleBackward() noexcept;

        void * GetResultPtr() noexcept;
        const void * GetResultPtr() const noexcept;

    private:

        void Schedule(ThreadContextType thread_context, std::coroutine_handle<> coroutine, bool allow_sync_resume) noexcept;

    protected:

        template <typename ReturnType>
        friend class CoroBundle;

        std::coroutine_handle<> m_CoroutineHandle = {};
        ThreadContextType m_CoroutineType = {};
        std::coroutine_handle<> m_ReturnHandle = {};
        ThreadContextType m_ReturnType = {};
        JobCompletionTrackingInfo * m_IncrementCounters = nullptr;
        std::thread::id m_OwningThread = {};
        Mutex * m_Mutex = nullptr;
        void * m_Promise = nullptr;
        void * m_ResultPtr = nullptr;
        CoroBase ** m_PromiseCoroPtr = nullptr;
        bool m_Started : 1 = false;
        bool m_Complete : 1 = false;
        bool m_HasReturnValue : 1 = false;
    };

    /**
     * ``co_return`` lowering requires the promise to expose either ``return_void`` or ``return_value``, not both;
     * constrained overloads do not help. Use ``Coro<void, T>`` (``return_void`` only) vs ``Coro<R, T>`` with ``R``
     * non-void (``return_value`` only). You cannot partially specialize a nested ``promise_type`` alone; partial
     * specialization of ``Coro`` is the usual split.
     */
    export template <typename ReturnType, ThreadContextType ThreadType>
    class Coro : public CoroBase
    {
    public:

        static_assert(ThreadType != ThreadContextType::Unknown, "ThreadType must be ThreadContextType::Unknown");

        struct promise_type
        {
            CoroBase * m_Coro = nullptr;

            bool m_HasReturnValue = false;
            alignas(ReturnType) ReturnStorage<ReturnType> m_ReturnStorage;

            promise_type()
            {
            }

            ~promise_type()
            {
                if (m_HasReturnValue)
                {
                    ReturnType * result = reinterpret_cast<ReturnType *>(&m_ReturnStorage.m_Array[0]);
                    result->~ReturnType();
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

            template <typename ReturnValueType>
            void return_value(ReturnValueType && value) noexcept
                requires (std::constructible_from<ReturnType, ReturnValueType>)
            {
                std::construct_at(
                    reinterpret_cast<ReturnType *>(static_cast<void *>(m_ReturnStorage.m_Array)),
                    std::forward<ReturnValueType>(value));
                m_HasReturnValue = true;

                Coro * coro = static_cast<Coro *>(m_Coro);
                coro->m_HasReturnValue = true;
                coro->ScheduleBackward();
            }

            void * operator new(std::size_t size) noexcept { return AllocateCoroutine(size, {}); }
            void * operator new(std::size_t size, std::align_val_t alignment) noexcept { return AllocateCoroutine(size, alignment); }
            void operator delete (void * ptr, std::size_t size) noexcept { FreeCoroutine(ptr, size); }
        };

        Coro()
        {
            FatalPrint("Failed to allocate coroutine");
        }

        explicit Coro(promise_type * promise) noexcept
        {
            promise->m_Coro = this;

            m_Promise = promise;
            m_ResultPtr = &promise->m_ReturnStorage.m_Array;
            m_PromiseCoroPtr = &promise->m_Coro;
            m_CoroutineHandle = std::coroutine_handle<promise_type>::from_promise(*promise);
            m_CoroutineType = ThreadType;
        }

        Coro(const Coro & rhs) = delete;
        Coro(Coro && rhs) = default;

        Coro & operator=(const Coro & rhs) = delete;
        Coro & operator=(Coro && rhs) = default;

        ReturnType GetResult() noexcept
        {
            promise_type * promise = static_cast<promise_type *>(m_Promise);

            ReturnType * result = reinterpret_cast<ReturnType *>(&promise->m_ReturnStorage.m_Array);
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
            requires(std::move_constructible<ReturnType>)
        {
            promise_type * promise = static_cast<promise_type *>(m_Promise);

            ReturnType * result = reinterpret_cast<ReturnType *>(&promise->m_ReturnStorage.m_Array);
            return std::move(*result);
        }
    };

    template <ThreadContextType ThreadType>
    class Coro<void, ThreadType> : public CoroBase
    {
    public:

        static_assert(ThreadType != ThreadContextType::Unknown, "ThreadType must be ThreadContextType::Unknown");

        struct promise_type
        {
            CoroBase * m_Coro = nullptr;

            static void unhandled_exception() noexcept
            {
                FatalPrint("Unhandled exception in coroutine");
            }

            auto get_return_object() noexcept { return Coro{this}; }
            static auto get_return_object_on_allocation_failure() noexcept { return Coro(); }

            static std::suspend_always initial_suspend() noexcept { return {}; }
            static std::suspend_always final_suspend() noexcept { return {}; }

            void return_void() noexcept
            {
                Coro * const coro = static_cast<Coro *>(m_Coro);
                coro->m_HasReturnValue = true;
                coro->ScheduleBackward();
            }

            void * operator new(std::size_t size) noexcept { return AllocateCoroutine(size, {}); }
            void * operator new(std::size_t size, std::align_val_t alignment) noexcept { return AllocateCoroutine(size, alignment); }
            void operator delete (void * ptr, std::size_t size) noexcept { FreeCoroutine(ptr, size); }
        };

        Coro()
        {
            FatalPrint("Failed to allocate coroutine");
        }

        explicit Coro(promise_type * promise) noexcept
        {
            promise->m_Coro = this;

            m_Promise = promise;
            m_ResultPtr = nullptr;
            m_PromiseCoroPtr = &promise->m_Coro;
            m_CoroutineHandle = std::coroutine_handle<promise_type>::from_promise(*promise);
            m_CoroutineType = ThreadType;
        }

        Coro(const Coro & rhs) = delete;
        Coro(Coro && rhs) = default;

        Coro & operator=(const Coro & rhs) = delete;
        Coro & operator=(Coro && rhs) = default;

        static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> coro) noexcept
        {
            m_ReturnHandle = coro;
            m_ReturnType = GetCurrentThreadContext();
            ScheduleForward();
        }

        static void await_resume() noexcept {}
    };

    template <typename ReturnType>
    class CoroBundle final
    {
    public:

        CoroBundle() = default;
        CoroBundle(const CoroBundle &) = delete;
        CoroBundle(CoroBundle &&) = delete;
        CoroBundle & operator=(const CoroBundle &) = delete;
        CoroBundle & operator=(CoroBundle &&) = delete;

        void Reserve(std::size_t count)
        {
            m_OwnedJobs.reserve(count);
        }

        /**
         * @brief Adds a job to the list.
         *
         * @param coro The job to add to the list
         * @note The job will be executed immediately
         */
        template <ThreadContextType ThreadType>
        void PushJob(Coro<ReturnType, ThreadType> && coro) noexcept
        {
            // We have to pre-reserve the entire array so we have stable pointers
            assert(m_OwnedJobs.size() < m_OwnedJobs.capacity());

            m_Target++;
            CoroBase & list_coro = m_OwnedJobs.emplace_back(std::forward<CoroBase>(coro));
            list_coro.RunFromList(&m_Counters);
        }

        /**
         * @brief Waits for all jobs in the list to complete.
         *
         * Blocks until the completion counter reaches the target value.
         */
        void WaitForCompletion() const noexcept
        {
            if (m_OwnedJobs.empty())
            {
                return;
            }

            while (true)
            {
                RunMainThreadJobsIfNeeded();

                MonitorAddr(&m_Counters.m_RemoteCount);

                if (m_Counters.m_LocalCount + m_Counters.m_RemoteCount >= m_Target)
                {
                    break;
                }

                WaitForAddr(150000);
            }
        }

        /**
         * @brief Returns the result of a job at the specified index.
         *
         * @param index The index of the job in the list
         * @return The job's return value
         * @pre The job at the specified index must have completed
         */
        std::add_lvalue_reference_t<ReturnType> operator[](std::size_t index)
            requires (!std::same_as<ReturnType, void>)
        {
            return *static_cast<ReturnType *>(m_OwnedJobs[index].GetResultPtr());
        }

        std::add_lvalue_reference_t<const ReturnType> operator[](std::size_t index) const
            requires (!std::same_as<ReturnType, void>)
        {
            return *static_cast<const ReturnType *>(m_OwnedJobs[index].GetResultPtr());
        }

    private:
        Vector<CoroBase> m_OwnedJobs;
        JobCompletionTrackingInfo m_Counters;
        int m_Target = 0;
    };

    export template <typename ReturnType = void>
    using JobCoro = Coro<ReturnType, ThreadContextType::Job>;

    export template <typename ReturnType = void>
    using BackgroundTask = Coro<ReturnType, ThreadContextType::Background>;

    export template <typename ReturnType = void>
    using MainThreadTask = Coro<ReturnType, ThreadContextType::Main>;
}

