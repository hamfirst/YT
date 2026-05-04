
module;

//import_std

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

import glm;

module YT:ImageReferenceImpl;

import :Types;
import :RenderTypes;
import :RenderManager;
import :ImageReference;

namespace YT
{
    ImageReference::ImageReference(ImageHandle handle, std::uint32_t width, std::uint32_t height, std::uint32_t image_index) noexcept
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
        rhs.m_ImageIndex = std::numeric_limits<std::uint32_t>::max();
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
        rhs.m_ImageIndex = std::numeric_limits<std::uint32_t>::max();
        return *this;
    }

    ImageReference::~ImageReference() noexcept
    {

        if (g_RenderManager && m_ImageIndex != std::numeric_limits<std::uint32_t>::max())
        {
            g_RenderManager->DestroyImage(m_Handle);
        }
    }

    ImageHandle ImageReference::GetHandle() const noexcept
    {
        return m_Handle;
    }

    std::uint32_t ImageReference::GetWidth() const noexcept
    {
        return m_Width;
    }

    std::uint32_t ImageReference::GetHeight() const noexcept
    {
        return m_Height;
    }

    std::uint32_t ImageReference::GetImageIndex() const noexcept
    {
        return m_ImageIndex;
    }

    ImageReference::operator bool() const noexcept
    {
        return m_ImageIndex != std::numeric_limits<std::uint32_t>::max();
    }

    void ImageReference::Release()
    {
        if (m_ImageIndex != std::numeric_limits<std::uint32_t>::max())
        {
            g_RenderManager->DestroyImage(m_Handle);
            m_Handle = {};
            m_Width = 0;
            m_Height = 0;
            m_ImageIndex = std::numeric_limits<std::uint32_t>::max();
        }
    }
}
