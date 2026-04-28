
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
import :FileMapper;

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

    DeferredImageLoad::DeferredImageLoad(const StringView & file_name) noexcept
    {
        FileMapper::PushDeferred([this, file_name = String{file_name}]
        {
            g_FileMapper.MapFile(file_name, [this](MappedFile && file)
            {
                m_ImageData = file.GetData();
                m_MappedFile.emplace(std::move(file));
            });
        });

        if (g_RenderManager)
        {
            g_RenderManager->FinalizeDeferredImageLoad();
        }
        else
        {
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
        m_MappedFile.reset();
    }

}