
#include "WaitImpl.h"

#include <thread>

#include <immintrin.h>
#include <cpuid.h>

// Need to put this in a standard header/cpp file since it has special compile flags

namespace YT
{
    bool SupportsIntelWait()
    {
        unsigned int eax, ebx, ecx, edx;
        // Query leaf 7, sub-leaf 0
        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
        {
            return (ecx & (1 << 5)); // Check bit 5 of ECX
        }

        return false;
    }

    bool SupportsAmdWait()
    {
        unsigned int eax, ebx, ecx, edx;
        // Query extended leaf 0x80000001
        if (__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx))
        {
            return (ecx & (1 << 29)); // Check bit 29 of ECX
        }
        return false;
    }

    __attribute__((target("waitpkg")))
    void MonitorIntel(const void * addr)
    {
        _umonitor(const_cast<void *>(addr));
    }

    __attribute__((target("mwaitx")))
    void MonitorAmd(const void * addr)
    {
        _mm_monitorx(addr, 0, 0);
    }

    void MonitorFallback(const void *)
    {

    }

    __attribute__((target("waitpkg")))
    void WaitIntel(std::uint32_t timeout)
    {
        _umwait(0, timeout);
    }

    __attribute__((target("mwaitx")))
    void WaitAmd(std::uint32_t timeout)
    {
        _mm_mwaitx(2, 0, timeout);
    }

    void WaitFallback(std::uint32_t)
    {
        _mm_pause();
    }
}

