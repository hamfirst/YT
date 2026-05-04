module;

#include <memory>
#include <coroutine>
#include <new>
#include <cassert>
#include <thread>
#include <utility>

module YT:CoroutineImpl;

import :Types;
import :JobTypes;
import :Coroutine;
import :JobManager;
import :BackgroundTaskManager;
import :FileMapper;
import :WorkerThread;

namespace YT
{
    thread_local ThreadContextType g_ThreadContextType = ThreadContextType::Unknown;
    thread_local std::coroutine_handle<> g_SynchronousCoroutineHandle = {};

    static constexpr std::size_t MaxCoroAllocSize = 128;
    ThreadCachedFixedBlockAllocator<std::uint64_t[MaxCoroAllocSize / 8], 4096, 65536> g_CoroAllocator;


    ThreadContextType GetCurrentThreadContext() noexcept
    {
        return g_ThreadContextType;
    }

    void SetCurrentThreadContext(ThreadContextType context) noexcept
    {
        g_ThreadContextType = context;
    }

    void * AllocateCoroutine(std::size_t size, std::align_val_t alignment) noexcept
    {
        void * p;
        if (size <= MaxCoroAllocSize)
        {
            p = g_CoroAllocator.Allocate();
        }
        else
        {
            p = std::malloc(size);
        }
        return p;
    }

    void FreeCoroutine(void * ptr, std::size_t size) noexcept
    {
        if (size <= MaxCoroAllocSize)
        {
            g_CoroAllocator.Free(ptr);
            return;
        }

        std::free(ptr);
    }

    void RunMainThreadJobsIfNeeded()
    {
        if (GetCurrentThreadContext() == ThreadContextType::Main)
        {
            g_MainThreadQueue.TryExecuteWork();
        }
    }

    void MakeThreadLocalCoroutineAllocator() noexcept
    {
        g_CoroAllocator.MakeLocalAllocator();
    }

    void SetSynchronousCoroutineHandle(std::coroutine_handle<> handle) noexcept
    {
        assert(!g_SynchronousCoroutineHandle);
        g_SynchronousCoroutineHandle = handle;
    }

    void ExecuteSynchronousCoroutineIfNeeded() noexcept
    {
        while (std::coroutine_handle<> handle = std::exchange(g_SynchronousCoroutineHandle, {}))
        {
            handle.resume();
        }
    }

    CoroBase::CoroBase(CoroBase && rhs) noexcept
        : m_CoroutineHandle(std::exchange(rhs.m_CoroutineHandle, {}))
        , m_CoroutineType(std::exchange(rhs.m_CoroutineType, ThreadContextType::Unknown))
        , m_ReturnHandle(std::exchange(rhs.m_ReturnHandle, {}))
        , m_ReturnType(std::exchange(rhs.m_ReturnType, ThreadContextType::Unknown))
        , m_IncrementCounters(std::exchange(rhs.m_IncrementCounters, nullptr))
        , m_OwningThread(std::exchange(rhs.m_OwningThread, std::thread::id{}))
        , m_Mutex(std::exchange(rhs.m_Mutex, nullptr))
        , m_Promise(std::exchange(rhs.m_Promise, nullptr))
        , m_ResultPtr(std::exchange(rhs.m_ResultPtr, nullptr))
        , m_PromiseCoroPtr(std::exchange(rhs.m_PromiseCoroPtr, nullptr))
    {
        assert(!rhs.m_Started);
        // Bit-fields cannot bind to `std::exchange` reference parameters.
        m_Started = rhs.m_Started;
        m_Complete = rhs.m_Complete;
        m_HasReturnValue = rhs.m_HasReturnValue;
        rhs.m_Started = false;
        rhs.m_Complete = false;
        rhs.m_HasReturnValue = false;

        if (m_PromiseCoroPtr)
        {
            *m_PromiseCoroPtr = this;
        }
    }

    CoroBase & CoroBase::operator= (CoroBase && rhs) noexcept
    {
        if (this != &rhs)
        {
            assert(!m_Started && !rhs.m_Started);

            if (m_CoroutineHandle)
            {
                m_CoroutineHandle.destroy();
            }

            m_CoroutineHandle = std::exchange(rhs.m_CoroutineHandle, {});
            m_CoroutineType = std::exchange(rhs.m_CoroutineType, ThreadContextType::Unknown);
            m_ReturnHandle = std::exchange(rhs.m_ReturnHandle, {});
            m_ReturnType = std::exchange(rhs.m_ReturnType, ThreadContextType::Unknown);
            m_IncrementCounters = std::exchange(rhs.m_IncrementCounters, nullptr);
            m_OwningThread = std::exchange(rhs.m_OwningThread, std::thread::id{});
            m_Mutex = std::exchange(rhs.m_Mutex, nullptr);
            m_Promise = std::exchange(rhs.m_Promise, nullptr);
            m_ResultPtr = std::exchange(rhs.m_ResultPtr, nullptr);
            m_PromiseCoroPtr = std::exchange(rhs.m_PromiseCoroPtr, nullptr);
            // Bit-fields cannot bind to `std::exchange` reference parameters.
            m_Started = rhs.m_Started;
            m_Complete = rhs.m_Complete;
            m_HasReturnValue = rhs.m_HasReturnValue;
            rhs.m_Started = false;
            rhs.m_Complete = false;
            rhs.m_HasReturnValue = false;

            if (m_PromiseCoroPtr)
            {
                *m_PromiseCoroPtr = this;
            }
        }

        return *this;
    }

    CoroBase::~CoroBase()
    {
        if (m_CoroutineHandle)
        {
            m_CoroutineHandle.destroy();
        }
    }

    void CoroBase::RunFromList(JobCompletionTrackingInfo * tracking_block) noexcept
    {
        m_IncrementCounters = tracking_block;
        m_OwningThread = std::this_thread::get_id();

        ScheduleForward();
    }

    void CoroBase::RunSynchronous()
    {
        Mutex mutex;
        m_Mutex = &mutex;
        m_Mutex->lock();

        ScheduleForward();
        m_Mutex->lock();

        m_Mutex = nullptr;
    }

    void CoroBase::ScheduleForward() noexcept
    {
        assert(!m_Started);
        m_Started = true;

        m_ReturnType = GetCurrentThreadContext();

        Schedule(m_CoroutineType, m_CoroutineHandle, true);
        ExecuteSynchronousCoroutineIfNeeded();
    }

    void CoroBase::EnqueueCoroutineResume() noexcept
    {
        Schedule(m_CoroutineType, m_CoroutineHandle, true);
    }

    void CoroBase::ScheduleBackward() noexcept
    {
        assert(!m_Complete);
        m_Complete = true;

        if (m_IncrementCounters)
        {
            if (m_OwningThread == std::this_thread::get_id())
            {
                m_IncrementCounters->m_LocalCount++;
            }
            else
            {
                m_IncrementCounters->m_RemoteCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        Schedule(m_ReturnType, m_ReturnHandle, false);

        if (m_Mutex)
        {
            m_Mutex->unlock();
        }
    }

    void * CoroBase::GetResultPtr() noexcept
    {
        return m_ResultPtr;
    }

    const void * CoroBase::GetResultPtr() const noexcept
    {
        return m_ResultPtr;
    }

    void CoroBase::Schedule(ThreadContextType thread_context, std::coroutine_handle<> coroutine, bool allow_sync_resume) noexcept
    {
        auto TrySyncResume = [&]()
        {
            if (allow_sync_resume)
            {
                Resume();
            }
            else
            {
                SetSynchronousCoroutineHandle(coroutine);
            }
        };

        if (coroutine)
        {
            if (thread_context == ThreadContextType::Main && GetCurrentThreadContext() == ThreadContextType::Main)
            {
                TrySyncResume();
            }
            else if (thread_context == ThreadContextType::Main || thread_context == ThreadContextType::Job)
            {
                g_JobManager->PushJob(*this);
            }
            else if (thread_context == ThreadContextType::Background)
            {
                g_BackgroundTaskManager->PushWork([this] { Resume(); });
            }
            else if (thread_context == ThreadContextType::FileMapper)
            {
                g_FileMapper->PushCoro(this);
            }
            else if (thread_context == ThreadContextType::FreeType)
            {
                g_FreeTypeThreadQueue.PushWork([this] { Resume(); });
            }
            else if (thread_context == ThreadContextType::AnyThread)
            {
                TrySyncResume();
            }
            else
            {
                // Not sure where to put this so just go on the main thread queue
                g_MainThreadQueue.PushWork([this] { Resume(); });
            }
        }
    }

    void CoroBase::Resume() const noexcept
    {
        if (!m_Complete)
        {
            if (m_CoroutineHandle)
            {
                assert(!m_HasReturnValue);

                m_CoroutineHandle.resume();
            }
        }
        else if (m_ReturnHandle)
        {
            m_ReturnHandle.resume();
        }

        ExecuteSynchronousCoroutineIfNeeded();
    }

}
