module;

#include <vector>
#include <cstdint>

module YT:RenderQuad;

import :RenderManager;
import :RenderTypes;
import :RenderReflect;

namespace YT
{
    class RenderQuad
    {
    public:

        static bool CreateRenderQuad(const ApplicationInitInfo & init_info) noexcept;

        RenderQuad(const ApplicationInitInfo & init_info);

        QuadRenderTypeId RegisterQuadShader(const StringView & function_name, const StringView & shader_code) noexcept;
        void UpdateShader() noexcept;

    private:

        bool Recompile() noexcept;

    private:

        struct ShaderData
        {
            String m_FunctionName;
            String m_ShaderCode;
        };

        BufferTypeId m_QuadBufferTypeId;
        PSOHandle m_PSOHandle;

        Vector<ShaderData> m_ShaderData;
        Vector<uint32_t> m_VertexShaderBinary;
        Vector<uint32_t> m_FragmentShaderBinary;
        bool m_NeedsRecompile = false;
    };

    UniquePtr<RenderQuad> g_RenderQuad;
}
