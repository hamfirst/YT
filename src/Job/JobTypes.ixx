
module;

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <array>

export module YT:JobTypes;

import :Types;

namespace YT
{
    export enum class ThreadContextType
    {
        Unknown,
        Main,
        Job,
        Background,
        FileMapper,
        FreeType,
    };

    export class CoroBase;

    export class JobManager;

    export template <typename ReturnType>
    class CoroBundle;


    static constexpr std::size_t CacheLineSize = 64;

    export struct JobCompletionTrackingInfo
    {
        std::size_t m_LocalCount = 0;
        std::atomic_size_t m_RemoteCount = 0;
        std::byte m_Pad[CacheLineSize - sizeof(m_LocalCount) - sizeof(m_RemoteCount)] = {};
    };

}
