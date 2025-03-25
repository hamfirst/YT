module;

#include <cstddef>
#include <cstdint>

module YT:RenderReflectImpl;

import :Types;
import :RenderTypes;
import :RenderReflect;
import :RenderManager;

namespace YT
{
    BufferTypeId RegisterShaderStruct(size_t struct_size, size_t struct_aligned_size, size_t max_elements_per_frame) noexcept
    {
        return g_RenderManager->RegisterBufferType(struct_size,
            struct_aligned_size, max_elements_per_frame * struct_aligned_size);
    }

    void RegisterShaderInclude(const StringView & struct_name, const StringView & shader_code) noexcept
    {
        g_RenderManager->SetShaderInclude(struct_name, shader_code);
    }

}
