
module;

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>

module YT:FontManagerImpl;

import :Types;
import :FontTypes;
import :FontReference;
import :FontManager;
import :OwnedBuffer;
import :BlockTable;
import :FileMapper;
import :DeferredFontLoad;

namespace YT
{
    FontManager::FontTableEntry::FontTableEntry(FT_Library library, OwnedBuffer && buffer) noexcept
    {
        if (library == nullptr || buffer.empty())
        {
            return;
        }

        m_OwnedBuffer.emplace(std::move(buffer));
        Span<const std::byte> span = m_OwnedBuffer->AsSpan();

        const FT_Error err = FT_New_Memory_Face(library,
            reinterpret_cast<FT_Byte const *>(span.data()),
            static_cast<FT_Long>(span.size()),
            0,
            &m_Face);

        if (err != 0)
        {
            m_OwnedBuffer.reset();
            return;
        }

        m_HbFont = hb_ft_font_create_referenced(m_Face);
        if (m_HbFont == nullptr)
        {
            FT_Done_Face(m_Face);
            m_Face = nullptr;
            m_OwnedBuffer.reset();
            return;
        }
    }

    FontManager::FontTableEntry::FontTableEntry(FT_Library library, const Span<const std::byte> & span) noexcept
    {
        if (library == nullptr || span.empty())
        {
            return;
        }

        const FT_Error err = FT_New_Memory_Face(library,
            reinterpret_cast<FT_Byte const *>(span.data()),
            static_cast<FT_Long>(span.size()),
            0,
            &m_Face);

        if (err != 0)
        {
            return;
        }

        m_HbFont = hb_ft_font_create_referenced(m_Face);
        if (m_HbFont == nullptr)
        {
            FT_Done_Face(m_Face);
            m_Face = nullptr;
            return;
        }
    }

    FontManager::FontTableEntry::FontTableEntry(FT_Library library, MappedFile && mf) noexcept
    {
        if (library == nullptr)
        {
            return;
        }

        m_Mapped.emplace(std::move(mf));

        Span<const std::byte> span = m_Mapped->GetData();
        if (span.empty())
        {
            m_Mapped.reset();
            return;
        }

        const FT_Error err = FT_New_Memory_Face(library,
            reinterpret_cast<FT_Byte const *>(span.data()),
            static_cast<FT_Long>(span.size()),
            0,
            &m_Face);

        if (err != 0)
        {
            m_Mapped.reset();
            return;
        }

        m_HbFont = hb_ft_font_create_referenced(m_Face);
        if (m_HbFont == nullptr)
        {
            FT_Done_Face(m_Face);
            m_Face = nullptr;
            m_Mapped.reset();
            return;
        }
    }

    FontManager::FontTableEntry::~FontTableEntry() noexcept
    {
        if (m_HbFont != nullptr)
        {
            hb_font_destroy(m_HbFont);
            m_HbFont = nullptr;
        }

        if (m_Face != nullptr)
        {
            FT_Done_Face(m_Face);
            m_Face = nullptr;
        }

        m_OwnedBuffer.reset();
        m_Mapped.reset();
    }

    bool FontManager::FontTableEntry::IsValid() const noexcept
    {
        return m_Face != nullptr && m_HbFont != nullptr;
    }

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
            (void)error;
            throw std::runtime_error("FT_Init_FreeType");
        }
    }

    FontManager::~FontManager()
    {
        m_FontTable.Clear();
        if (m_Library != nullptr)
        {
            FT_Done_FreeType(m_Library);
            m_Library = nullptr;
        }
    }

    FontReference FontManager::CreateFontFromBytes(OwnedBuffer && buffer) noexcept
    {
        if (m_Library == nullptr || buffer.empty())
        {
            return {};
        }

        const BlockTableHandle raw = m_FontTable.AllocateHandle(m_Library, std::move(buffer));

        if (raw == InvalidBlockTableHandle)
        {
            return {};
        }

        FontTableEntry * entry = m_FontTable.ResolveHandle(raw);
        if (entry == nullptr || !entry->IsValid())
        {
            m_FontTable.ReleaseHandle(raw);
            return {};
        }

        const FontHandle font_handle = MakeCustomBlockTableHandle<FontHandle>(raw);
        const std::uint32_t font_index = FontTable::GetHandleIndex(font_handle);
        return FontReference(font_handle, font_index);
    }

    FontReference FontManager::CreateFontFromBorrowedBytes(const Span<const std::byte> & data) noexcept
    {
        if (m_Library == nullptr || data.empty())
        {
            return {};
        }

        const BlockTableHandle raw = m_FontTable.AllocateHandle(m_Library, data);

        if (raw == InvalidBlockTableHandle)
        {
            return {};
        }

        FontTableEntry * entry = m_FontTable.ResolveHandle(raw);
        if (entry == nullptr || !entry->IsValid())
        {
            m_FontTable.ReleaseHandle(raw);
            return {};
        }

        const FontHandle font_handle = MakeCustomBlockTableHandle<FontHandle>(raw);
        const std::uint32_t font_index = FontTable::GetHandleIndex(font_handle);
        return FontReference(font_handle, font_index);
    }

    FontReference FontManager::CreateFontFromMappedFile(MappedFile && mapped_file) noexcept
    {
        if (m_Library == nullptr || mapped_file.GetData().empty())
        {
            return {};
        }

        const BlockTableHandle raw =
            m_FontTable.AllocateHandle(m_Library, std::move(mapped_file));

        if (raw == InvalidBlockTableHandle)
        {
            return {};
        }

        FontTableEntry * entry = m_FontTable.ResolveHandle(raw);
        if (entry == nullptr || !entry->IsValid())
        {
            m_FontTable.ReleaseHandle(raw);
            return {};
        }

        const FontHandle font_handle = MakeCustomBlockTableHandle<FontHandle>(raw);
        const std::uint32_t font_index = FontTable::GetHandleIndex(font_handle);
        return FontReference(font_handle, font_index);
    }

    void FontManager::DestroyFont(FontHandle handle) noexcept
    {
        if (static_cast<BlockTableHandle>(handle) != InvalidBlockTableHandle)
        {
            m_FontTable.ReleaseHandle(handle);
        }
    }

    void FontManager::FinalizeDeferredFontLoads() noexcept
    {
        DeferredFontLoad::Finalize();
    }
}
