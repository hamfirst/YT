
module;

#include <cstddef>
#include <cstdint>
#include <limits>

module YT:FontReferenceImpl;

import :Types;
import :FontTypes;
import :FontReference;
import :FontManager;

namespace YT
{
    FontReference::FontReference(FontHandle handle, std::uint32_t font_index) noexcept
        : m_Handle(handle)
        , m_FontIndex(font_index)
    {
    }

    FontReference::FontReference(FontReference && rhs) noexcept
        : m_Handle(rhs.m_Handle)
        , m_FontIndex(rhs.m_FontIndex)
    {
        rhs.m_Handle = InvalidFontHandle;
        rhs.m_FontIndex = std::numeric_limits<std::uint32_t>::max();
    }

    FontReference & FontReference::operator=(FontReference && rhs) noexcept
    {
        if (this != &rhs)
        {
            Release();

            m_Handle = rhs.m_Handle;
            m_FontIndex = rhs.m_FontIndex;

            rhs.m_Handle = InvalidFontHandle;
            rhs.m_FontIndex = std::numeric_limits<std::uint32_t>::max();
        }

        return *this;
    }

    FontReference::~FontReference() noexcept
    {
        Release();
    }

    FontHandle FontReference::GetHandle() const noexcept
    {
        return m_Handle;
    }

    std::uint32_t FontReference::GetFontIndex() const noexcept
    {
        return m_FontIndex;
    }

    FontReference::operator bool() const noexcept
    {
        return m_FontIndex != std::numeric_limits<std::uint32_t>::max();
    }

    void FontReference::Release() noexcept
    {
        if (g_FontManager && m_FontIndex != std::numeric_limits<std::uint32_t>::max())
        {
            g_FontManager->DestroyFont(m_Handle);
        }

        m_Handle = InvalidFontHandle;
        m_FontIndex = std::numeric_limits<std::uint32_t>::max();
    }
}
