module;

#include <memory>
#include <coroutine>

module YT:CoroutineImpl;

import :Coroutine;
import :JobManager;
import :JobTypes;
import :WorkerThread;

namespace YT
{
    thread_local int g_ThreadID = -1;
    thread_local ThreadContextType g_ThreadContextType = ThreadContextType::Unknown;

    static constexpr std::size_t MaxCoroAllocSize = 128;
    ThreadCachedFixedBlockAllocator<std::uint64_t[MaxCoroAllocSize / 8], 4096, 65536> g_CoroAllocator;

    int GetThreadId() noexcept
    {
        return g_ThreadID;
    }

    void SetThreadId(int thread_id) noexcept
    {
        g_ThreadID = thread_id;
    }

    ThreadContextType GetCurrentThreadContext() noexcept
    {
        return g_ThreadContextType;
    }

    void SetCurrentThreadContext(ThreadContextType context) noexcept
    {
        g_ThreadContextType = context;
    }

    void MakeThreadLocalCoroutineAllocator() noexcept
    {
        g_CoroAllocator.MakeLocalAllocator();
    }

    CoroBase::~CoroBase()
    {
        if (m_CoroutineHandle)
        {
            m_CoroutineHandle.destroy();
        }
    }

    void CoroBase::RunFromList(JobCompletionTrackingBlock * tracking_block)
    {
        m_IncrementCounters = tracking_block;
        m_OwningThread = GetThreadId();

        if (m_CoroutineType == ThreadContextType::Main)
        {
            g_MainThreadQueue.PushWork([this] { Resume(); });
        }
        else if (m_CoroutineType == ThreadContextType::Job)
        {
            g_JobManager->PushJob(*this);
        }
        else
        {
            FatalPrint("Attempting to run invalid coroutine in job list");
        }
    }

    void CoroBase::Resume() const
    {
        if (!m_Complete)
        {
            m_CoroutineHandle.resume();
        }
        else if (m_ReturnHandle)
        {
            m_ReturnHandle.resume();
        }
    }

    std::suspend_always CoroBase::initial_suspend() noexcept
    {
        return {};
    }

    std::suspend_always CoroBase::final_suspend() noexcept
    {
        m_Complete = true;
        if (m_IncrementCounters)
        {
            if (m_OwningThread == GetThreadId())
            {
                (*m_IncrementCounters)[GetThreadId()].m_LocalCount++;
            }
            else
            {
                (*m_IncrementCounters)[GetThreadId()].m_RemoteCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        if (m_ReturnHandle)
        {
            if (m_ReturnType == ThreadContextType::Main || m_ReturnType == ThreadContextType::Job)
            {
                g_JobManager->PushJob(*this);
            }
            else if (m_ReturnType == ThreadContextType::Background)
            {
                
            }
            else
            {
                g_MainThreadQueue.PushWork([this] { Resume(); });
            }
        }

        return {};
    }

    void CoroBase::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        m_ReturnHandle = coro;
    }

    void CoroBase::operator delete (void * ptr, std::size_t size) noexcept
    {
        if (size > MaxCoroAllocSize)
        {
            std::free(ptr);
            return;
        }

        g_CoroAllocator.Free(ptr);
    }
}
