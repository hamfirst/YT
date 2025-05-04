module;

#include <format>
#include <experimental/meta>

#include <glm/glm.hpp>

export module YT:RenderReflect;

import :Types;
import :RenderTypes;

namespace YT
{
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
    constexpr String GetShaderStructDef() noexcept
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

    export template <typename T>
    String GetShaderBufferDef(int set, int binding) noexcept
    {
        StringView class_name = std::meta::identifier_of(^^T);

        return std::format(
            "layout(std430, set = {}, binding = {}) readonly buffer {}BufferType\n"
            "{{\n"
            "    {} elems[];\n"
            "}} {}_buffer;\n"
            "\n"
            "{} Get{}(uint64_t handle)\n"
            "{{\n"
            "    if(uint(((handle & 0xFF00000000000000UL) >> 56)) == {}U)\n"
            "    {{\n"
            "        return {}_buffer.elems[uint(handle & 0x00FFFFFFFFFFFFFFUL)];\n"
            "    }}\n"
            "    else\n"
            "    {{\n"
            "        {} default_value;\n"
            "        return default_value;\n"
            "    }}\n"
            "}}\n"
            "\n",
            set, binding, class_name, class_name, class_name, class_name, class_name, binding, class_name, class_name);
    }

    BufferTypeId RegisterShaderStruct(size_t struct_size, size_t struct_aligned_size, size_t max_elements_per_frame) noexcept;
    void RegisterShaderInclude(const StringView & struct_name, const StringView & shader_code) noexcept;

    export template <typename T>
    void RegisterShaderType() noexcept
    {
        StringView class_name = std::meta::identifier_of(^^T);
        RegisterShaderInclude(class_name, GetShaderStructDef<T>());
    }

    export template <typename T>
    BufferTypeId RegisterShaderBufferStruct(size_t max_elements_per_frame, int descriptor_set = 0) noexcept
    {
        StringView class_name = std::meta::identifier_of(^^T);
        BufferTypeId type_id = RegisterShaderStruct(sizeof(T), sizeof(T), max_elements_per_frame);

        String shader_code = GetShaderStructDef<T>() +
            GetShaderBufferDef<T>(descriptor_set, type_id.m_BufferTypeIndex);

        RegisterShaderInclude(class_name, shader_code);

        return type_id;
    }


}
