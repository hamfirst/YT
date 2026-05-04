
module;

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <coroutine>

module YT:DeferredFontLoadImpl;

import :Types;
import :FontLoad;
import :FontReference;
import :DeferredFontLoad;
import :Coroutine;

namespace YT
{
    Vector<UniquePtr<CoroBundle<void>>> g_DeferredFontLoadBundles;

    DeferredFontLoad::DeferredFontLoad(const Span<const std::byte> & font_data) noexcept
        : m_FontData(font_data)
    {
        m_Next = g_DeferredFontLoadHead;
        g_DeferredFontLoadHead = this;
    }

    DeferredFontLoad::DeferredFontLoad(const StringView & file_name) noexcept
        : m_FileName(file_name)
    {
        m_Next = g_DeferredFontLoadHead;
        g_DeferredFontLoadHead = this;
    }

    DeferredFontLoad::operator const FontReference & () const noexcept
    {
        return m_FontRef;
    }

    const FontReference & DeferredFontLoad::operator -> () const noexcept
    {
        return m_FontRef;
    }

    const FontReference & DeferredFontLoad::operator * () const noexcept
    {
        return m_FontRef;
    }

    void DeferredFontLoad::Start() noexcept
    {
        if (g_DeferredFontLoadHead == nullptr)
        {
            return;
        }

        Vector<DeferredFontLoad *> deferred_fonts;

        DeferredFontLoad * ptr = g_DeferredFontLoadHead;
        while (ptr != nullptr)
        {
            deferred_fonts.emplace_back(ptr);
            ptr = ptr->m_Next;
        }

        UniquePtr<CoroBundle<void>> loads = MakeUnique<CoroBundle<void>>();
        loads->Reserve(deferred_fonts.size());

        for (DeferredFontLoad * d : deferred_fonts)
        {
            loads->PushJob(d->AsyncLoad());
        }

        g_DeferredFontLoadBundles.emplace_back(std::move(loads));

        g_DeferredFontLoadHead = nullptr;
    }

    void DeferredFontLoad::Finalize() noexcept
    {
        for (UniquePtr<CoroBundle<void>> & bundle : g_DeferredFontLoadBundles)
        {
            bundle->WaitForCompletion();
        }

        g_DeferredFontLoadBundles.clear();
    }

    Coro<void, ThreadContextType::AnyThread> DeferredFontLoad::AsyncLoad()
    {
        if (m_FontData.data())
        {
            FontReference loaded = co_await LoadFontFromBorrowedMemory(m_FontData);
            m_FontRef = std::move(loaded);
        }
        else
        {
            FontReference loaded = co_await LoadFontFromFile(m_FileName);
            m_FontRef = std::move(loaded);
        }

        co_return;
    }
}
