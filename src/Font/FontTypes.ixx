module;

#include <cstddef>
#include <cstdint>

export module YT:FontTypes;

import :Types;
import :BlockTable;

namespace YT
{
    export struct FontHandle : public BlockTableHandle
    {
    };

    export constexpr FontHandle InvalidFontHandle = MakeCustomBlockTableHandle<FontHandle>(InvalidBlockTableHandle);
}
