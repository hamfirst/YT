module;

#include <cstddef>
#include <cstdint>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>

export module YT:FontManager;

import :Types;
import :FontTypes;
import :FontReference;
import :OwnedBuffer;
import :FileMapper;
import :BlockTable;

namespace YT
{
    export class FontManager final
    {
    public:
        static bool CreateFontManager() noexcept;

        FontManager();
        ~FontManager();

        /** Owns a copy of the font bytes via `buffer`; must run on ThreadContextType::FreeType. */
        [[nodiscard]] FontReference CreateFontFromBytes(OwnedBuffer && buffer) noexcept;

        /** No copy: `data` must stay valid until the font handle is destroyed. */
        [[nodiscard]] FontReference CreateFontFromBorrowedBytes(const Span<const std::byte> & data) noexcept;

        /** Moves a memory mapping in; retained until the font is destroyed. */
        [[nodiscard]] FontReference CreateFontFromMappedFile(MappedFile && mapped_file) noexcept;

        void DestroyFont(FontHandle handle) noexcept;

        void FinalizeDeferredFontLoads() noexcept;

    private:
        struct FontTableEntry
        {
            Optional<OwnedBuffer> m_OwnedBuffer{};
            Optional<MappedFile> m_Mapped{};
            FT_Face m_Face = nullptr;
            hb_font_t * m_HbFont = nullptr;

            FontTableEntry(FT_Library library, OwnedBuffer && buffer) noexcept;
            FontTableEntry(FT_Library library, const Span<const std::byte> & span) noexcept;
            FontTableEntry(FT_Library library, MappedFile && mf) noexcept;

            FontTableEntry(const FontTableEntry &) = delete;
            FontTableEntry & operator=(const FontTableEntry &) = delete;
            FontTableEntry(FontTableEntry &&) = delete;
            FontTableEntry & operator=(FontTableEntry &&) = delete;

            ~FontTableEntry() noexcept;

            [[nodiscard]] bool IsValid() const noexcept;
        };

        using FontTable = BlockTable<FontTableEntry>;

        FT_Library m_Library = nullptr;
        FontTable m_FontTable{};
    };

    export UniquePtr<FontManager> g_FontManager;
}
