module;

#include <experimental/meta>

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
        uint8_t m_Type : 8;
        uint64_t m_Index : 56;
    };

    export struct QuadData
    {
        glm::vec2 m_Start;
        glm::vec2 m_End;

        glm::vec2 m_StartTX;
        glm::vec2 m_EndTX;

        glm::vec4 m_Color;

        uint32_t m_Mode = 0;
        uint64_t m_ExtraData = 0;
    };

    // start 'expand' definition
    namespace __impl {
        template<auto... vals>
        struct replicator_type {
            template<typename F>
              constexpr void operator>>(F body) const {
                (body.template operator()<vals>(), ...);
            }
        };

        template<auto... vals>
        replicator_type<vals...> replicator = {};
    }

    template<typename R>
    consteval auto expand(R range) {
        std::vector<std::meta::info> args;
        for (auto r : range) {
            args.push_back(std::meta::reflect_value(r));
        }
        return substitute(^^__impl::replicator, args);
    }
    // end 'expand' definition

    export template <typename T>
    constexpr String GetStructShaderDef()
    {
        constexpr auto class_info = ^^T;
        constexpr auto class_name = std::meta::identifier_of(class_info);

        String str("struct ");
        str.append(class_name);
        str.append("\n{");

        [: expand(nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())) :] >> [&]<auto dm>
        {
            str.append("\n\t");

            if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^bool))
            {
                str.append("bool");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^uint32_t))
            {
                str.append("uint");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^int32_t))
            {
                str.append("int");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^uint64_t))
            {
                str.append("uint64_t");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^int64_t))
            {
                str.append("int64_t");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^float))
            {
                str.append("float");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^double))
            {
                str.append("double");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::vec1))
            {
                str.append("vec1");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::vec2))
            {
                str.append("vec2");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::vec3))
            {
                str.append("vec3");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::vec4))
            {
                str.append("vec4");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::bvec1))
            {
                str.append("bvec1");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::bvec2))
            {
                str.append("bvec2");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::bvec3))
            {
                str.append("bvec3");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::bvec4))
            {
                str.append("bvec4");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::ivec1))
            {
                str.append("ivec1");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::ivec2))
            {
                str.append("ivec2");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::ivec3))
            {
                str.append("ivec3");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::ivec4))
            {
                str.append("ivec4");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::uvec1))
            {
                str.append("uvec1");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::uvec2))
            {
                str.append("uvec2");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::uvec3))
            {
                str.append("uvec3");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::uvec4))
            {
                str.append("uvec4");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::dvec1))
            {
                str.append("dvec1");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::dvec2))
            {
                str.append("dvec2");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::dvec3))
            {
                str.append("dvec3");
            }
            else if constexpr (std::meta::dealias(std::meta::type_of(dm)) == std::meta::dealias(^^glm::dvec4))
            {
                str.append("dvec4");
            }
            else
            {
                str.append(std::meta::display_string_of(std::meta::type_of(dm)));
            }
            str.append(" ");
            str.append(std::meta::identifier_of(dm));
            str.append(";");
        };

        str.append("\n};\n");

        return str;
    }

}