
module;

#include <glm/glm.hpp>


module YT:ImageReferenceImpl;

import :Types;
import :RenderTypes;
import :RenderManager;
import :ImageReference;

namespace YT
{
    ImageReference::ImageReference(ImageHandle handle, uint32_t width, uint32_t height, uint32_t image_index) noexcept
        : m_Handle(handle)
        , m_Width(width)
        , m_Height(height)
        , m_ImageIndex(image_index)
    {

    }

    ImageReference::ImageReference(ImageReference && rhs) noexcept
    {
        m_Handle = rhs.m_Handle;
        m_Width = rhs.m_Width;
        m_Height = rhs.m_Height;
        m_ImageIndex = rhs.m_ImageIndex;

        rhs.m_Handle = {};
        rhs.m_Width = 0;
        rhs.m_Height = 0;
        rhs.m_ImageIndex = UINT32_MAX;
    }

    ImageReference & ImageReference::operator=(ImageReference && rhs) noexcept
    {
        m_Handle = rhs.m_Handle;
        m_Width = rhs.m_Width;
        m_Height = rhs.m_Height;
        m_ImageIndex = rhs.m_ImageIndex;

        rhs.m_Handle = {};
        rhs.m_Width = 0;
        rhs.m_Height = 0;
        rhs.m_ImageIndex = UINT32_MAX;
        return *this;
    }

    ImageReference::~ImageReference() noexcept
    {
        if (m_ImageIndex != UINT32_MAX)
        {
            g_RenderManager->DestroyImage(m_Handle);
        }
    }

    ImageHandle ImageReference::GetHandle() const noexcept
    {
        return m_Handle;
    }

    uint32_t ImageReference::GetWidth() const noexcept
    {
        return m_Width;
    }

    uint32_t ImageReference::GetHeight() const noexcept
    {
        return m_Height;
    }

    uint32_t ImageReference::GetImageIndex() const noexcept
    {
        return m_ImageIndex;
    }

    ImageReference::operator bool() const noexcept
    {
        return m_ImageIndex != UINT32_MAX;
    }

    void ImageReference::Release()
    {
        if (m_ImageIndex != UINT32_MAX)
        {
            g_RenderManager->DestroyImage(m_Handle);
            m_Handle = {};
            m_Width = 0;
            m_Height = 0;
            m_ImageIndex = UINT32_MAX;
        }
    }
}
