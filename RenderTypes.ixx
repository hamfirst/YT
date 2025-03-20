module;

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

    export enum class QuadDataMode : int
    {

    };

    export struct QuadData
    {
        float m_StartX = 0.0f;
        float m_StartY = 0.0f;
        float m_EndX = 0.0f;
        float m_EndY = 0.0f;

        float m_StartTX = 0.0f;
        float m_StartTY = 0.0f;
        float m_EndTX = 0.0f;
        float m_EndTY = 0.0f;

        float m_TintR = 0.0f;
        float m_TintG = 0.0f;
        float m_TintB = 0.0f;
        float m_TintA = 0.0f;

        int m_Mode = 0;
        int m_ImageIndex = 0;
        int m_GradientIndex = 0;
        int m_PolygonIndex = 0;

    };
}