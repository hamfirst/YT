
#pragma once

#include <cstdint>

// Need to put this in a standard header/cpp file since it has special compile flags

namespace YT
{
    bool SupportsIntelWait();
    bool SupportsAmdWait();

    void MonitorIntel(const void * addr);
    void MonitorAmd(const void * addr);
    void MonitorFallback(const void * addr);

    void WaitIntel(std::uint32_t timeout);
    void WaitAmd(std::uint32_t timeout);
    void WaitFallback(std::uint32_t timeout);
}


