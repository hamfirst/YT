module;

#include <glm/glm.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

export module YT:RenderTypes;

import :Types;
import :BlockTable;

namespace YT
{
    export struct PSOCreateInfo final
    {
        // You must set either vertex or mesh shader
        OptionalPtr<const uint8_t> m_VertexShader = nullptr;
        StringView m_VertexShaderEntryPoint = "main";

        OptionalPtr<const uint8_t> m_MeshShader = nullptr;
        StringView m_MeshShaderEntryPoint = "main";

        RequiredPtr<const uint8_t> m_FragmentShader = nullptr;
        StringView m_FragmentShaderEntryPoint = "main";

        Vector<Pair<String, String>> m_FeatureConstants;
    };

    export struct PSODeferredSettings
    {
        vk::Format m_SurfaceFormat = vk::Format::eUndefined;
        size_t m_BufferDescriptorSetId = 0;

        bool operator==(const PSODeferredSettings &) const = default;
        bool operator!=(const PSODeferredSettings &) const = default;
    };

    export struct PSO
    {
        PSOCreateInfo m_CreateInfo;
        Vector<Pair<PSODeferredSettings, vk::UniquePipeline>> m_Pipelines;
    };

    export using PSOTable = BlockTable<PSO>;
    export struct PSOHandle : public BlockTableHandle { };
    export constexpr PSOHandle InvalidPSOHandle = MakeCustomBlockTableHandle<PSOHandle>(InvalidBlockTableHandle);

    export struct BufferType
    {
        uint32_t m_ElementSize = 0;
        uint32_t m_AlignedSize = 0;
        size_t m_BufferSize = 0;
    };

    export struct BufferTypeId
    {
        uint32_t m_BufferTypeIndex = 0;
    };

    export struct BufferDataHandle
    {
        uint64_t m_Index : 56;
        uint8_t m_Type : 8;

        operator uint64_t() const noexcept
        {
            static_assert(sizeof(*this) == sizeof(uint64_t));
            uint64_t * p_alias = (uint64_t *)(this);
            return *p_alias;
        }
    };

    export struct GlobalData
    {
        float m_Time = 0.0f;
        float m_DeltaTime = 0.0f;

        float m_Random1 = 0.0f;
        float m_Random2 = 0.0f;
    };

    export struct QuadData
    {
        glm::vec2 m_Start;
        glm::vec2 m_End;

        glm::vec2 m_StartTX;
        glm::vec2 m_EndTX;

        glm::vec4 m_Color;

        uint32_t m_Mode = 0;
        uint32_t m_Flags = 0;
        uint64_t m_ExtraData = 0;
    };

}