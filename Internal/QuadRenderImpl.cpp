module;

#include <vector>
#include <cstdint>

module YT:QuadRenderImpl;

import :RenderManager;
import :RenderTypes;
import :RenderReflect;
import :QuadRender;

namespace YT
{
    constexpr char g_Quad_VS[] =
    {
#embed "../Shaders/Quad/Quad.qvert"
    };

    constexpr char g_QuadHeader_FS[] =
    {
#embed "../Shaders/Quad/Header.qfrag"
    };

    constexpr char g_QuadMain_FS[] =
    {
#embed "../Shaders/Quad/Main.qfrag"
    };

    constexpr char g_QuadFooter_FS[] =
    {
#embed "../Shaders/Quad/Footer.qfrag"
    };

    bool QuadRender::CreateQuadRender(const ApplicationInitInfo & init_info) noexcept
    {
        try
        {
            g_QuadRender = MakeUnique<QuadRender>(init_info);
            return true;
        }
        catch (const Exception & e)
        {
            FatalPrint("Error creating render quad: {}", e.what());
        }
        catch (...)
        {
            FatalPrint("Unknown exception creating render quad");
        }

        return false;
    }

    QuadRender::QuadRender(const ApplicationInitInfo & init_info)
    {
        RegisterShaderType<QuadRenderData>();
        m_QuadBufferTypeId = RegisterShaderBufferStruct<QuadData>(64 * 1024);

        constexpr StringView vertex_shader(g_Quad_VS, sizeof(g_Quad_VS));
        if (!g_RenderManager->CompileShader(vertex_shader,
            ShaderType::Vertex, "QuadShader_VS", m_VertexShaderBinary))
        {
            throw Exception("Failed to compile quad vertex shader code");
        }

        g_RenderManager->RegisterShader(reinterpret_cast<uint8_t *>(m_VertexShaderBinary.data()),
            m_VertexShaderBinary.size() * sizeof(m_VertexShaderBinary[0]));

        g_RenderManager->GetPreRenderDelegate().BindNoValidityCheck([this]() { UpdateShader(); });
    }

    BufferTypeId QuadRender::GetQuadBufferTypeUd() const noexcept
    {
        return m_QuadBufferTypeId;
    }

    PSOHandle QuadRender::GetQuadPSOHandle() const noexcept
    {
        return m_PSOHandle;
    }

    QuadRenderTypeId QuadRender::RegisterQuadShader(const StringView & function_name, const StringView & shader_code) noexcept
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

    void QuadRender::UpdateShader() noexcept
    {
        if (m_NeedsRecompile)
        {
            if (Recompile())
            {
                PSOCreateInfo pso_info;
                pso_info.m_VertexShader = reinterpret_cast<uint8_t *>(m_VertexShaderBinary.data());
                pso_info.m_FragmentShader = reinterpret_cast<uint8_t *>(m_FragmentShaderBinary.data());
                pso_info.m_PushConstantsSize = sizeof(QuadRenderData);
                m_PSOHandle = g_RenderManager->RegisterPSO(pso_info);
            }
            m_NeedsRecompile = false;
        }
    }

    bool QuadRender::Recompile() noexcept
    {
        constexpr StringView header(g_QuadHeader_FS, sizeof(g_QuadHeader_FS));
        constexpr StringView main(g_QuadMain_FS, sizeof(g_QuadMain_FS));
        constexpr StringView footer(g_QuadFooter_FS, sizeof(g_QuadFooter_FS));

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

        if (!m_ShaderData.empty())
        {
            g_RenderManager->UnregisterShader(reinterpret_cast<uint8_t*>(m_FragmentShaderBinary.data()));
        }

        m_FragmentShaderBinary.clear();
        if (!g_RenderManager->CompileShader(shader,
            ShaderType::Fragment, "QuadShader_FS", m_FragmentShaderBinary))
        {
            FatalPrint("Failed to compile quad shader code");
            return false;
        }

        g_RenderManager->RegisterShader(reinterpret_cast<uint8_t *>(m_FragmentShaderBinary.data()),
            m_FragmentShaderBinary.size() * sizeof(m_FragmentShaderBinary[0]));
        return true;
    }
}
