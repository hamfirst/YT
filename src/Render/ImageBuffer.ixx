module;

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <utility>
#include <stdexcept>
#include <type_traits>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:ImageBuffer;

import :Types;
import :RenderTypes;
import :StagingBuffer;

namespace YT
{
    vk::Format GetVkFormat(ImageFormat format)
    {
        switch (format)
        {
        default:
        case ImageFormat::R8G8B8A8Unorm:
            return vk::Format::eR8G8B8A8Unorm;
        case ImageFormat::R8G8B8Unorm:
            return vk::Format::eR8G8B8Unorm;
        case ImageFormat::R8G8Unorm:
            return vk::Format::eR8G8Unorm;
        case ImageFormat::R8Unorm:
            return vk::Format::eR8Unorm;
        }
    }

    ImageLayout GetDefaultImageLayoutForOwner(ImageOwner owner)
    {
        switch (owner)
        {
            default:
            case ImageOwner::Unknown:
                return ImageLayout::Unknown;
            case ImageOwner::Graphics:
                return ImageLayout::ShaderRead;
            case ImageOwner::Transfer:
                return ImageLayout::TransferDest;
        }
    }

    vk::ImageLayout GetVkImageLayout(ImageLayout layout)
    {
        switch (layout)
        {
            default:
            case ImageLayout::Unknown:
                return vk::ImageLayout::eUndefined;
            case ImageLayout::ShaderRead:
                return vk::ImageLayout::eShaderReadOnlyOptimal;
            case ImageLayout::TransferDest:
                return vk::ImageLayout::eTransferDstOptimal;
        }
    }

    std::uint32_t GetVkQueueFamily(ImageOwner owner)
    {
        switch (owner)
        {
            default:
            case ImageOwner::Unknown:
                return vk::QueueFamilyIgnored;
            case ImageOwner::Graphics:
                return GGraphicsQueueIndex;
            case ImageOwner::Transfer:
                return GTransferQueueIndex;
        }
    }

    Pair<vk::PipelineStageFlags2, vk::AccessFlags2> GetSrcImageAccess(ImageLayout prev_layout, ImageLayout new_layout)
    {
        if (prev_layout == ImageLayout::ShaderRead && new_layout == ImageLayout::TransferDest)
        {
            return MakePair(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite);
        }
        if (prev_layout == ImageLayout::TransferDest && new_layout == ImageLayout::ShaderRead)
        {
            return MakePair(vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite);
        }

        return {};
    }

    Pair<vk::PipelineStageFlags2, vk::AccessFlags2> GetDstImageAccess(ImageLayout prev_layout, ImageLayout new_layout)
    {
        if (prev_layout == ImageLayout::ShaderRead && new_layout == ImageLayout::TransferDest)
        {
            return MakePair(vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite);
        }
        if (prev_layout == ImageLayout::TransferDest && new_layout == ImageLayout::ShaderRead)
        {
            return MakePair(vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead);
        }

        return {};
    }

    class ImageBuffer final
    {
    public:
        ImageBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator,
            std::uint32_t width, std::uint32_t height, ImageFormat format) :
            m_Device(device), m_Allocator(allocator), m_Width(width), m_Height(height), m_Format(format)
        {
            vk::ImageCreateInfo image_create_info;
            image_create_info.imageType = vk::ImageType::e2D;
            image_create_info.extent.width = width;
            image_create_info.extent.height = height;
            image_create_info.extent.depth = 1;
            image_create_info.mipLevels = 1;
            image_create_info.arrayLayers = 1;
            image_create_info.format = GetVkFormat(format);

            image_create_info.tiling = vk::ImageTiling::eOptimal;
            image_create_info.initialLayout = vk::ImageLayout::eUndefined;
            image_create_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            image_create_info.sharingMode = vk::SharingMode::eExclusive;
            image_create_info.samples = vk::SampleCountFlagBits::e1;
            image_create_info.flags = {};

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

            auto [allocation, image] =
                allocator->createImageUnique(image_create_info, allocation_create_info);

            if (!image || !allocation)
            {
                throw Exception("Could not create image");
            }

            m_Image = std::move(image);
            m_Allocation = std::move(allocation);

            vk::ImageViewCreateInfo view_create_info;
            view_create_info.image = m_Image.get();
            view_create_info.viewType = vk::ImageViewType::e2D;
            view_create_info.format = image_create_info.format;
            view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            m_ImageView = m_Device->createImageViewUnique(view_create_info, nullptr);
            if (!m_ImageView)
            {
                throw Exception("Could not create image view");
            }

            CreateSampler();
        }

        ImageBuffer(vk::UniqueDevice & device, vma::UniqueAllocator & allocator,
            vk::Image non_owning_image, std::uint32_t width, std::uint32_t height, ImageFormat format) :
            m_Device(device), m_Allocator(allocator), m_Width(width), m_Height(height), m_Format(format)
        {
            m_NonOwningImage = non_owning_image;

            vk::ImageViewCreateInfo view_create_info;
            view_create_info.image = m_Image.get();
            view_create_info.viewType = vk::ImageViewType::e2D;
            view_create_info.format = GetVkFormat(format);
            view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            m_ImageView = m_Device->createImageViewUnique(view_create_info, nullptr);
            if (!m_ImageView)
            {
                throw Exception("Could not create image view");
            }

            CreateSampler();
        }

        ImageBuffer(const ImageBuffer&) = delete;
        ImageBuffer(ImageBuffer&&) = delete;
        ImageBuffer& operator=(const ImageBuffer&) = delete;
        ImageBuffer& operator=(ImageBuffer&&) = delete;
        ~ImageBuffer() noexcept = default;

        [[nodiscard]] vk::ImageMemoryBarrier2 TransitionToLayout(ImageLayout new_layout) noexcept
        {
            assert(new_layout != ImageLayout::Unknown);
            vk::ImageMemoryBarrier2 memory_barrier;

            memory_barrier.oldLayout = GetVkImageLayout(m_Layout);
            memory_barrier.newLayout = GetVkImageLayout(new_layout);
            memory_barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            memory_barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            memory_barrier.image = m_Image.get();
            memory_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            memory_barrier.subresourceRange.baseMipLevel = 0;
            memory_barrier.subresourceRange.levelCount = 1;
            memory_barrier.subresourceRange.baseArrayLayer = 0;
            memory_barrier.subresourceRange.layerCount = 1;

            auto src_access = GetSrcImageAccess(m_Layout, new_layout);
            auto dst_access = GetDstImageAccess(m_Layout, new_layout);

            memory_barrier.srcStageMask = src_access.first;
            memory_barrier.srcAccessMask = src_access.second;
            memory_barrier.dstStageMask = dst_access.first;
            memory_barrier.dstAccessMask = dst_access.second;

            m_Layout = new_layout;
            return memory_barrier;
        }

        [[nodiscard]] vk::ImageMemoryBarrier2 TransitionToOwner(ImageOwner new_owner) noexcept
        {
            assert(new_owner != ImageOwner::Unknown);

            vk::ImageMemoryBarrier2 memory_barrier = TransitionToLayout(GetDefaultImageLayoutForOwner(new_owner));

            if (m_Owner != new_owner)
            {
                memory_barrier.srcQueueFamilyIndex = GetVkQueueFamily(m_Owner);
                memory_barrier.dstQueueFamilyIndex = GetVkQueueFamily(new_owner);
            }

            return memory_barrier;
        }

        [[nodiscard]] vk::Image GetImage() const noexcept
        {
            return m_Image ? m_Image.get() : m_NonOwningImage;
        }

        [[nodiscard]] vk::ImageView GetImageView() const noexcept
        {
            return m_ImageView.get();
        }

        [[nodiscard]] vk::Sampler GetSampler() const noexcept
        {
            return m_Sampler.get();
        }

        [[nodiscard]] std::uint32_t GetWidth() const noexcept
        {
            return m_Width;
        }

        [[nodiscard]] std::uint32_t GetHeight() const noexcept
        {
            return m_Height;
        }

        [[nodiscard]] ImageFormat GetFormat() const noexcept
        {
            return m_Format;
        }

        [[nodiscard]] ImageOwner GetOwner() const noexcept
        {
            return m_Owner;
        }

        void SetLastTimelineValue(std::uint64_t timeline_value) noexcept
        {
            m_LastTimelineValue = std::max(m_LastTimelineValue, timeline_value);
        }

        [[nodiscard]] std::uint64_t GetLastTimelineValue() const noexcept
        {
            return m_LastTimelineValue;
        }

        template <typename Visitor>
        void VisitRenderResources(Visitor && v)
        {
            if (m_Image)
            {
                v(m_Image);
            }

            if (m_ImageView)
            {
                v(m_ImageView);
            }

            if (m_Allocation)
            {
                v(m_Allocation);
            }

            if (m_Sampler)
            {
                v(m_Sampler);
            }
        }

    private:

        void CreateSampler()
        {
            vk::SamplerCreateInfo sampler_info;
            sampler_info.setMagFilter(vk::Filter::eLinear);
            sampler_info.setMinFilter(vk::Filter::eLinear);
            sampler_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
            sampler_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
            sampler_info.setAnisotropyEnable(false);
            sampler_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);

            m_Sampler = m_Device->createSamplerUnique(sampler_info);
        }

    private:
        vk::UniqueDevice & m_Device;
        vma::UniqueAllocator & m_Allocator;
        vma::UniqueImage m_Image;
        vk::Image m_NonOwningImage;
        vk::UniqueImageView m_ImageView;
        vma::UniqueAllocation m_Allocation;
        vk::UniqueSampler m_Sampler;

        std::uint32_t m_Width = 0;
        std::uint32_t m_Height = 0;
        ImageFormat m_Format = ImageFormat::R8G8B8A8Unorm;
        ImageOwner m_Owner = ImageOwner::Unknown;
        ImageLayout m_Layout = ImageLayout::Unknown;
        ImageUsage m_Usage = ImageUsage::Fragment;
        std::uint64_t m_LastTimelineValue = 0;
    };
}