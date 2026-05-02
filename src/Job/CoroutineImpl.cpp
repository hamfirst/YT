module;

#include <memory>
#include <coroutine>
#include <cassert>

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

    void * AllocateCoroutine(std::size_t size) noexcept
    {
        if (size <= MaxCoroAllocSize)
        {
            return g_CoroAllocator.Allocate();
        }

        return std::malloc(size);
    }

    void FreeCoroutine(void * ptr, std::size_t size) noexcept
    {
        if (size <= MaxCoroAllocSize)
        {
            g_CoroAllocator.Free(ptr);
        }

        std::free(ptr);
    }

    void MakeThreadLocalCoroutineAllocator() noexcept
    {
        g_CoroAllocator.MakeLocalAllocator();
    }

    void CoroBase::RunFromList(JobCompletionTrackingBlock * tracking_block) noexcept
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

    void CoroBase::ScheduleForward() noexcept
    {
        assert(!m_Started);
        m_Started = true;

        Schedule(m_CoroutineType, m_CoroutineHandle);
    }

    void CoroBase::ScheduleBackward() noexcept
    {
        assert(!m_Complete);
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

        Schedule(m_ReturnType, m_ReturnHandle);
    }

    void CoroBase::Schedule(ThreadContextType thread_context, std::coroutine_handle<> coroutine) noexcept
    {
        if (coroutine)
        {
            if (thread_context == ThreadContextType::Main || thread_context == ThreadContextType::Job)
            {
                g_JobManager->PushJob(*this);
            }
            else if (thread_context == ThreadContextType::Background)
            {
                g_BackgroundTaskManager.PushWork([this] { Resume(); });
            }
            else if (thread_context == ThreadContextType::FileMapper)
            {
                g_FileMapper.PushCoro(this);
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
            m_CoroutineHandle.resume();
        }
        else if (m_ReturnHandle)
        {
            m_ReturnHandle.resume();
        }
    }

}
