module;

#include <cstddef>
#include <cstdint>
#include <limits>

export module YT:FontReference;

import :Types;
import :FontTypes;

namespace YT
{
    export class FontReference
    {
    public:
        FontReference() noexcept = default;
        FontReference(FontHandle handle, std::uint32_t font_index) noexcept;

        FontReference(const FontReference &) = delete;
        FontReference(FontReference && rhs) noexcept;
        FontReference & operator=(const FontReference &) = delete;
        FontReference & operator=(FontReference && rhs) noexcept;

        ~FontReference() noexcept;

        [[nodiscard]] FontHandle GetHandle() const noexcept;
        [[nodiscard]] std::uint32_t GetFontIndex() const noexcept;

        explicit operator bool() const noexcept;

        void Release() noexcept;

    private:
        FontHandle m_Handle = InvalidFontHandle;
        std::uint32_t m_FontIndex = std::numeric_limits<std::uint32_t>::max();
    };
}
