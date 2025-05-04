module;

#include <vector>
#include <cstdint>

module YT:QuadRender;

import :RenderManager;
import :RenderTypes;
import :RenderReflect;

namespace YT
{
    class QuadRender
    {
    public:

        static bool CreateQuadRender(const ApplicationInitInfo & init_info) noexcept;

        explicit QuadRender(const ApplicationInitInfo & init_info);

        [[nodiscard]] BufferTypeId GetQuadBufferTypeId() const noexcept;
        [[nodiscard]] PSOHandle GetQuadConsecutivePSOHandle() const noexcept;
        [[nodiscard]] PSOHandle GetQuadIndexedPSOHandle() const noexcept;

        QuadRenderTypeId RegisterQuadShader(const StringView & function_name, const StringView & shader_code) noexcept;

    private:
        void UpdateShader() noexcept;
        bool Recompile() noexcept;

    private:

        struct ShaderData
        {
            String m_FunctionName;
            String m_ShaderCode;
        };

        BufferTypeId m_QuadBufferTypeId;
        PSOHandle m_ConsecutivePSOHandle;
        PSOHandle m_IndexedPSOHandle;

        Vector<ShaderData> m_ShaderData;
        Vector<uint32_t> m_ConsecutiveVertexShaderBinary;
        Vector<uint32_t> m_IndexedVertexShaderBinary;
        Vector<uint32_t> m_FragmentShaderBinary;
        bool m_NeedsRecompile = true;
    };

    UniquePtr<QuadRender> g_QuadRender;
}
