module;

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <type_traits>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

import glm;

export module YT:RenderTypes;

import :Types;
import :BlockTable;

namespace YT
{
    export enum class ImageFormat
    {
        R8G8B8A8Unorm,
        R8G8B8Unorm,
        R8G8Unorm,
        R8Unorm,
    };

    export int GetBytesPerPixel(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::R8G8B8A8Unorm:
                return 4;
            case ImageFormat::R8G8B8Unorm:
                return 3;
            case ImageFormat::R8G8Unorm:
                return 2;
            case ImageFormat::R8Unorm:
                return 1;
        }

        return 4;
    }

    export enum class ShaderType
    {
        Vertex,
        Fragment,
        Mesh,
    };

    export struct PSOCreateInfo final
    {
        // You must set either vertex or mesh shader
        Span<const std::uint8_t> m_VertexShader = {};
        StringView m_VertexShaderEntryPoint = "main";

        Span<const std::uint8_t> m_MeshShader = {};
        StringView m_MeshShaderEntryPoint = "main";

        Span<const std::uint8_t> m_FragmentShader = {};
        StringView m_FragmentShaderEntryPoint = "main";

        int m_PushConstantsSize = 0;
        Map<String, float> m_FeatureConstants;
    };

    export struct PSODeferredSettings
    {
        vk::Format m_SurfaceFormat = vk::Format::eUndefined;
        std::size_t m_BufferDescriptorSetId = 0;

        bool operator==(const PSODeferredSettings &) const = default;
        bool operator!=(const PSODeferredSettings &) const = default;
    };

    export struct PSOVariant
    {
        PSODeferredSettings m_DeferredSettings;
        vk::UniquePipelineLayout m_Layout;
        vk::UniquePipeline m_Pipeline;

        PSOVariant() = default;
        PSOVariant(const PSOVariant &) = delete;
        PSOVariant(PSOVariant &&) = default;

        PSOVariant &operator=(const PSOVariant &) = delete;
        PSOVariant &operator=(PSOVariant &&) = default;

        ~PSOVariant()
        {
            m_Pipeline.reset();
            m_Layout.reset();
        }
    };

    export struct PSOHandle : public BlockTableHandle { };
    export constexpr PSOHandle InvalidPSOHandle = MakeCustomBlockTableHandle<PSOHandle>(InvalidBlockTableHandle);

    export struct ImageHandle : public BlockTableHandle { };

    export constexpr ImageHandle InvalidImageHandle = MakeCustomBlockTableHandle<ImageHandle>(InvalidBlockTableHandle);

    export struct BufferType
    {
        std::uint32_t m_ElementSize = 0;
        std::uint32_t m_AlignedSize = 0;
        std::size_t m_BufferSize = 0;
    };

    export struct BufferTypeId
    {
        std::uint32_t m_BufferTypeIndex = 0;
    };

    export struct BufferDataHandle
    {
        std::uint64_t m_Index : 56;
        std::uint8_t m_Type : 8;

        operator std::uint64_t() const noexcept
        {
            static_assert(sizeof(*this) == sizeof(std::uint64_t));
            std::uint64_t * p_alias = (std::uint64_t *)(this);
            return *p_alias;
        }
    };

    export struct QuadRenderTypeId
    {
        int m_QuadRenderTypeIndex = 0;
    };
}

export namespace YT
{
    #define vec2 glm::vec2
    #define vec3 glm::vec3
    #define vec4 glm::vec4

    #define uint std::uint32_t
    #define int64_t std::int64_t
    #define uint64_t std::uint64_t

#include "../shaders/Structs/GlobalData.h"
#include "../shaders/Structs/IndexData.h"
#include "../shaders/Structs/QuadData.h"
#include "../shaders/Structs/QuadRenderData.h"
#include "../shaders/Structs/DrawerData.h"

    #undef vec2
    #undef vec3
    #undef vec4

    #undef uint
    #undef int64_t
    #undef uint64_t
}

