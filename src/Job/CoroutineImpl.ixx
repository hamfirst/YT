module;

#include <memory>

module YT:CoroutineImpl;

import :Coroutine;
import :JobManager;

namespace YT
{
    std::suspend_always JobCoroBase::initial_suspend() noexcept
    {
        return {};
    }

    std::suspend_always JobCoroBase::final_suspend() noexcept
    {
        return {};
    }

    void JobCoroBase::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        m_ReturnHandle = coro;
    }

    void * JobCoroBase::operator new(std::size_t size) noexcept
    {
        if (size > MaxJobAllocSize)
        {
            return std::malloc(size);
        }

        return g_JobManagerAllocator.Allocate();
    }

    void JobCoroBase::operator delete (void * ptr, std::size_t size) noexcept
    {
        if (size > MaxJobAllocSize)
        {
            std::free(ptr);
            return;
        }

        g_JobManagerAllocator.Free(ptr);
    }
}