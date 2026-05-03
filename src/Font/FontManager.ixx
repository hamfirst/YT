
module;

#include <ft2build.h>
#include FT_FREETYPE_H

export module YT:FontManager;

import :Types;

namespace YT
{
    class FontManager final
    {
    public:
        static bool CreateFontManager() noexcept;

        FontManager();
        ~FontManager();

    private:

        FT_Library m_Library = {};
    };

    UniquePtr<FontManager> g_FontManager;
}
