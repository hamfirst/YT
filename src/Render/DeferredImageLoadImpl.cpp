
module;

//import_std

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>

module YT:DeferredImageLoadImpl;

import :Types;
import :ImageReference;
import :DeferredImageLoad;
import :RenderManager;

namespace YT
{
    DeferredImageLoad::DeferredImageLoad(const Span<const std::byte> & image_data) noexcept
    {
        if (g_RenderManager)
        {
            m_ImageRef.emplace(g_RenderManager->CreateImage(image_data));
        }
        else
        {
            m_ImageData = image_data;

            m_Next = g_DeferredImageLoadHead;
            g_DeferredImageLoadHead = this;
        }
    }

    DeferredImageLoad::operator const ImageReference & () const noexcept
    {
        return m_ImageRef.value();
    }

    const ImageReference & DeferredImageLoad::operator -> () const noexcept
    {
        return *m_ImageRef;
    }

    const ImageReference & DeferredImageLoad::operator * () const noexcept
    {
        return *m_ImageRef;
    }

    void DeferredImageLoad::Finalize() noexcept
    {
        m_ImageRef = g_RenderManager->CreateImage(m_ImageData);
    }
}