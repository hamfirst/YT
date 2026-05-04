
module;

#include <cstddef>
#include <cstdint>
#include <utility>
#include <coroutine>
#include <new>
#include <type_traits>
#include <utility>

module YT:FontLoadImpl;

import :Types;
import :FontReference;
import :FontManager;
import :OwnedBuffer;
import :FontLoad;
import :Coroutine;
import :FileMapper;

namespace YT
{
    FreeTypeTask<FontReference> CreateFontReferenceFromOwnedBuffer(OwnedBuffer && buffer) noexcept
    {
        if (!g_FontManager)
        {
            co_return FontReference{};
        }

        co_return g_FontManager->CreateFontFromBytes(std::move(buffer));
    }

    FreeTypeTask<FontReference> CreateFontReferenceFromBorrowedBytes(const Span<const std::byte> & data) noexcept
    {
        if (!g_FontManager)
        {
            co_return FontReference{};
        }

        co_return g_FontManager->CreateFontFromBorrowedBytes(data);
    }

    FreeTypeTask<FontReference> CreateFontReferenceFromMappedFile(MappedFile && file) noexcept
    {
        if (!g_FontManager)
        {
            co_return FontReference{};
        }

        co_return g_FontManager->CreateFontFromMappedFile(std::move(file));
    }

    Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromMemory(const Span<const std::byte> & font_data) noexcept
    {
        OwnedBuffer buffer(font_data);
        if (!buffer)
        {
            co_return FontReference{};
        }

        FontReference ref = co_await CreateFontReferenceFromOwnedBuffer(std::move(buffer));
        co_return std::move(ref);
    }

    Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromOwnedBuffer(OwnedBuffer && buffer) noexcept
    {
        if (!buffer)
        {
            co_return FontReference{};
        }

        FontReference ref = co_await CreateFontReferenceFromOwnedBuffer(std::move(buffer));
        co_return std::move(ref);
    }

    Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromBorrowedMemory(const Span<const std::byte> & font_data) noexcept
    {
        FontReference ref = co_await CreateFontReferenceFromBorrowedBytes(font_data);
        co_return std::move(ref);
    }

    Coro<FontReference, ThreadContextType::AnyThread> LoadFontFromFile(const StringView & file_name) noexcept
    {
        MappedFile file = co_await MapFileAsync(file_name);
        FontReference ref = co_await CreateFontReferenceFromMappedFile(std::move(file));
        co_return std::move(ref);
    }
}
