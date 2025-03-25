module;

#include <vector>
#include <cstdint>

module YT:RenderQuad;

import :RenderManager;
import :RenderTypes;
import :RenderReflect;

namespace YT
{
    constexpr char g_QuadHeader[] =
    {
#embed "../Shaders/Quad/Header.qfrag"
    };

    constexpr char g_QuadMain[] =
    {
#embed "../Shaders/Quad/Main.qfrag"
    };

    constexpr char g_QuadFooter[] =
    {
#embed "../Shaders/Quad/Footer.qfrag"
    };

    class RenderQuad
    {
    public:
        RenderQuad() noexcept
        {
            m_QuadBufferTypeId = RegisterShaderStruct<QuadData>(64 * 1024);
        }

        QuadRenderTypeId RegisterQuadShaderType(const StringView & function_name, const StringView & shader_code) noexcept
        {
            QuadRenderTypeId new_type_id
            {
                .m_QuadRenderTypeIndex = static_cast<int>(m_ShaderData.size()),
            };

            m_ShaderData.emplace_back(ShaderData
            {
                .m_FunctionName = String(function_name),
                .m_ShaderCode = String(shader_code),
            });

            m_NeedsRecompile = true;
            return new_type_id;
        }

    private:

        void Recompile() noexcept
        {
            constexpr StringView header(g_QuadHeader, sizeof(g_QuadHeader));
            constexpr StringView main(g_QuadMain, sizeof(g_QuadMain));
            constexpr StringView footer(g_QuadFooter, sizeof(g_QuadFooter));

            String shader(header);
            for (const ShaderData & shader_data : m_ShaderData)
            {
                shader += shader_data.m_ShaderCode;
                shader += "\n\n";
            }

            shader += main;

            uint32_t quad_shader_id = 0;
            for (const ShaderData & shader_data : m_ShaderData)
            {
                shader += Format("case {}: o_color = {}(global_data, quad_data); return;\n",
                    quad_shader_id, shader_data.m_FunctionName);
                ++quad_shader_id;
            }

            shader += footer;

            if (!g_RenderManager->CompileShader(shader, "QuadShader", m_ShaderBinary))
            {
                FatalPrint("Failed to compile quad shader code");
            }
        }

    private:

        struct ShaderData
        {
            String m_FunctionName;
            String m_ShaderCode;
        };

        BufferTypeId m_QuadBufferTypeId;

        Vector<ShaderData> m_ShaderData;
        Vector<uint32_t> m_ShaderBinary;
        bool m_NeedsRecompile = false;
    };
}
