
module;

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <array>

export module YT:JobTypes;

import :Types;

namespace YT
{
    export class CoroBase;

    export class JobManager;

    export enum class ThreadContextType
    {
        Unknown,
        Main,
        Job,
        Background,
        FileMapper,
        FreeType,
    };

    static constexpr std::size_t CacheLineSize = 64;

    export struct JobCompletionTrackingElement
    {
        std::size_t m_LocalCount = 0;
        std::atomic_size_t m_RemoteCount = 0;
        std::byte m_Pad[CacheLineSize - sizeof(m_LocalCount) - sizeof(m_RemoteCount)] = {};
    };

    export using JobCompletionTrackingBlock = std::array<JobCompletionTrackingElement, Threading::NumJobThreads>;

}
