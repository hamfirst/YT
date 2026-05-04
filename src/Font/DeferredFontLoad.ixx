module;

#include <cstddef>
#include <cstdint>
#include <utility>

export module YT:DeferredFontLoad;

import :Types;
import :FontReference;
import :FontLoad;
import :Coroutine;
import :FileMapper;

namespace YT
{
    export class DeferredFontLoad;

    export DeferredFontLoad * g_DeferredFontLoadHead = nullptr;

    class DeferredFontLoad
    {
    public:
        explicit DeferredFontLoad(const Span<const std::byte> & font_data) noexcept;
        explicit DeferredFontLoad(const StringView & file_name) noexcept;

        DeferredFontLoad(const DeferredFontLoad &) = delete;
        DeferredFontLoad(DeferredFontLoad &&) = delete;
        DeferredFontLoad & operator=(const DeferredFontLoad &) = delete;
        DeferredFontLoad & operator=(DeferredFontLoad &&) = delete;

        operator const FontReference & () const noexcept;
        const FontReference & operator -> () const noexcept;
        const FontReference & operator * () const noexcept;

        static void Start() noexcept;
        static void Finalize() noexcept;

    protected:
        Coro<void, ThreadContextType::AnyThread> AsyncLoad();

    private:
        Span<const std::byte> m_FontData{};
        String m_FileName{};
        FontReference m_FontRef{};
        DeferredFontLoad * m_Next = nullptr;
    };

    export template <std::size_t ArraySize>
    DeferredFontLoad LoadDeferredFontEmbedded(const std::byte (&arr)[ArraySize]) noexcept
    {
        return DeferredFontLoad(Span<const std::byte>(&arr[0], ArraySize));
    }

    export template <std::size_t ArraySize>
    DeferredFontLoad LoadDeferredFontEmbedded(unsigned char (&arr)[ArraySize]) noexcept
    {
        return DeferredFontLoad(CreateByteSpan(arr));
    }
}
