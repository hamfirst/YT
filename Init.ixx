module;

#include <exception>
#include <iostream>

export module YT:Init;

import :Types;

namespace YT
{
    export bool Init(const ApplicationInitInfo& init_info) noexcept;

    export void RunUntilAllWindowsClosed() noexcept;

    export void Cleanup() noexcept;

}
