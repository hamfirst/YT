module;

#include <cstddef>
#include <cstdint>

#include "WaitImpl.h"

export module YT:Wait;

namespace YT
{
    export void (*MonitorAddr)(const void *) = MonitorFallback;
    export void (*WaitForAddr)(std::uint32_t) = WaitFallback;

    export void InitWait()
    {
        if (SupportsIntelWait())
        {
            MonitorAddr = MonitorIntel;
            WaitForAddr = WaitIntel;
        }
        else if (SupportsAmdWait())
        {
            MonitorAddr = MonitorAmd;
            WaitForAddr = WaitAmd;
        }
    }

}
