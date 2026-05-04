
module;

//import_std

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <memory>
#include <stdexcept>
#include <coroutine>
#include <new>

#include <stb_image.h>

module YT:DeferredImageLoadImpl;

import :Types;
import :ImageLoad;
import :ImageReference;
import :DeferredImageLoad;
import :RenderManager;
import :FileMapper;
import :Coroutine;

namespace YT
{
    Vector<UniquePtr<CoroBundle<void>>> g_DeferredImageLoadBundles;

    DeferredImageLoad::DeferredImageLoad(const Span<const std::byte> & image_data) noexcept
        : m_ImageData(image_data)
    {
        m_Next = g_DeferredImageLoadHead;
        g_DeferredImageLoadHead = this;
    }

    DeferredImageLoad::DeferredImageLoad(const StringView & file_name) noexcept
        : m_FileName(file_name)
    {
        m_Next = g_DeferredImageLoadHead;
        g_DeferredImageLoadHead = this;
    }

    DeferredImageLoad::operator const ImageReference & () const noexcept
    {
        return m_ImageRef;
    }

    const ImageReference & DeferredImageLoad::operator -> () const noexcept
    {
        return m_ImageRef;
    }

    const ImageReference & DeferredImageLoad::operator * () const noexcept
    {
        return m_ImageRef;
    }

    void DeferredImageLoad::Start() noexcept
    {
        if (!g_DeferredImageLoadHead)
        {
            return;
        }

        Vector<DeferredImageLoad*> deferred_images;

        DeferredImageLoad * ptr = g_DeferredImageLoadHead;
        while (ptr != nullptr)
        {
            deferred_images.emplace_back(ptr);
            ptr = ptr->m_Next;
        }

        UniquePtr<CoroBundle<void>> loads = MakeUnique<CoroBundle<void>>();
        loads->Reserve(deferred_images.size());

        for (DeferredImageLoad * ptr : deferred_images)
        {
            loads->PushJob(ptr->AsyncLoad());
        }

        g_DeferredImageLoadBundles.emplace_back(std::move(loads));

        g_DeferredImageLoadHead = nullptr;
    }

    void DeferredImageLoad::Finalize() noexcept
    {
        for (UniquePtr<CoroBundle<void>> & bundle : g_DeferredImageLoadBundles)
        {
            bundle->WaitForCompletion();
        }

        g_DeferredImageLoadBundles.clear();
    }

    Coro<void, ThreadContextType::AnyThread> DeferredImageLoad::AsyncLoad()
    {
        if (m_ImageData.data())
        {
            m_ImageRef = co_await LoadImageFromMemory(m_ImageData);
        }
        else
        {
            ImageReference reference = co_await LoadImageFromFile(m_FileName);
            m_ImageRef = std::move(reference);
        }

        co_return;
    }
}
