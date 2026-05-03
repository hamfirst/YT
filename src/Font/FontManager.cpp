
module;

#include <stdexcept>

#include <ft2build.h>
#include FT_FREETYPE_H

module YT:FontManagerImpl;

import :FontManager;

namespace YT
{
    bool FontManager::CreateFontManager() noexcept
    {
        if (g_FontManager)
        {
            return true;
        }

        try
        {
            g_FontManager = MakeUnique<FontManager>();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    FontManager::FontManager()
    {
        if (FT_Error error = FT_Init_FreeType(&m_Library))
        {
            throw std::runtime_error("FT_Init_FreeType");
        }
    }

    FontManager::~FontManager()
    {
        FT_Done_FreeType(m_Library);
    }
}