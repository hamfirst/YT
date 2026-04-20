module;

//import_std
#include <exception>
#include <iostream>
#include <chrono>

export module YT:Init;

import :Types;

namespace YT
{
    export bool Init(const ApplicationInitInfo& init_info) noexcept;

    export void RunUntilAllWindowsClosed() noexcept;

    export void Cleanup() noexcept;

    export std::chrono::time_point<std::chrono::high_resolution_clock> g_InitTime;
}
