module;

#include <cstddef>
#include <cstdint>

export module YT:FontLoad;

import :Types;
import :FontReference;
import :OwnedBuffer;
import :Coroutine;
import :FileMapper;

namespace YT
{
    /** Moves owned bytes into FontManager; must run on ThreadContextType::FreeType. */
    export FreeTypeTask<FontReference> CreateFontReferenceFromOwnedBuffer(OwnedBuffer && buffer) noexcept;

    /** No copy: `data` must outlive every FontReference created from this coroutine run. Use with `#embed`/static blobs. */
    export FreeTypeTask<FontReference> CreateFontReferenceFromBorrowedBytes(const Span<const std::byte> & data) noexcept;

    /** Moves the mapping into FontManager so no duplicate buffer is retained. */
    export FreeTypeTask<FontReference> CreateFontReferenceFromMappedFile(MappedFile && file) noexcept;

    /** Copies `font_data` into an OwnedBuffer before opening the font (safe for ephemeral spans across suspension). */
    export Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromMemory(const Span<const std::byte> & font_data) noexcept;

    /** Opens a font from an already-built buffer (moved into the FreeType job). */
    export Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromOwnedBuffer(OwnedBuffer && buffer) noexcept;

    /** No copy — see CreateFontReferenceFromBorrowedBytes. */
    export Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromBorrowedMemory(const Span<const std::byte> & font_data) noexcept;

    /** Maps the file once, then adopts the mapping (no malloc copy). */
    export Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromFile(const StringView & file_name) noexcept;
}
