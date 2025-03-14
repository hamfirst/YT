module;

#include <memory>
#include <atomic>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>


import YT.Types;
import YT.RenderTypes;
import YT.WindowResource;
import YT.BlockTable;
import YT.TransientBuffer;

export module YT.RenderManager;

namespace YT
{
    export class RenderManager final
    {
    public:
        static bool CreateRenderManager(const ApplicationInitInfo & init_info) noexcept;

        explicit RenderManager(const ApplicationInitInfo & init_info);
        ~RenderManager();

    public:
        bool CreateWindowResources(const WindowInitInfo & init_info, WindowResource & resource) noexcept;
        bool UpdateWindowResource(WindowResource & resource) noexcept;

        bool RenderWindowResource(WindowResource & resource) noexcept;
        void ReleaseWindowResource(WindowResource & resource) noexcept;

        template <int ArraySize>
        void RegisterShader(uint8_t (&shader_data)[ArraySize]) noexcept
        {
            RegisterShader(shader_data, ArraySize);
        }

        void RegisterShader(uint8_t* shader_data, std::size_t shader_data_size) noexcept;

        MaybeInvalid<PSOHandle> RegisterPSO(const PSOCreateInfo & create_info) noexcept;
        [[nodiscard]] bool BindPSO(vk::CommandBuffer & command_buffer,
            const PSODeferredSettings & deferred_settings, PSOHandle handle) noexcept;

    protected:
;
        [[nodiscard]] bool CreateSwapChainResources(WindowResource & resource) noexcept;

        [[nodiscard]] OptionalPtr<vk::UniqueShaderModule> FindShaderModule(const uint8_t * shader_data) noexcept;

        [[nodiscard]] OptionalPtr<vk::UniquePipeline> PreparePSO(const PSODeferredSettings & deferred_settings, PSO & pso) noexcept;

        [[nodiscard]] bool PrepareCommandBuffer(const WindowResource & resource) noexcept;
        [[nodiscard]] bool CompleteCommandBuffer(const WindowResource & resource) noexcept;
        [[nodiscard]] bool SubmitCommandBuffer(const WindowResource & resource) noexcept;

    private:

        vk::UniqueInstance m_Instance;

#if !defined(NDEBUG)
        vk::UniqueDebugUtilsMessengerEXT m_DebugUtilsMessenger;
#endif

        vk::PhysicalDevice m_PhysicalDevice;
        vk::UniqueDevice m_Device;
        vk::Queue m_Queue;
        vma::UniqueAllocator m_Allocator;

        vk::UniquePipelineLayout m_PipelineLayout;

        vk::UniqueCommandPool m_CommandPool;

        Map<const uint8_t*, vk::UniqueShaderModule> m_ShaderModules;

        PSOTable m_PSOTable;

        static constexpr int FrameResourceCount = 3;
        struct FrameResource
        {
            UniquePtr<TransientBuffer> m_QuadBuffer;
        };

        std::array<FrameResource, FrameResourceCount> m_FrameResources;
        uint32_t m_FrameIndex = 0;

    };

    export UniquePtr<RenderManager> g_RenderManager;

}
