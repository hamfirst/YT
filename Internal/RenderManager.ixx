module;

#include <memory>
#include <unordered_map>
#include <chrono>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:RenderManager;

import :Types;
import :RenderTypes;
import :WindowResource;
import :BlockTable;
import :TransientBuffer;
import :ShaderBuilder;

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

        bool RenderWindowResources(const Vector<WindowResource*> & window_resources) noexcept;
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

        BufferTypeId RegisterBufferType(uint32_t element_size,
            uint32_t aligned_element_size, size_t buffer_size) noexcept;

        BufferTypeId GetGlobalBufferTypeId() const noexcept;
        BufferTypeId GetQuadBufferTypeId() const noexcept;

        BufferDataHandle WriteToBuffer(BufferTypeId buffer_type_id, void * data, uint32_t data_size) noexcept;

    protected:
;
        [[nodiscard]] bool CreateSwapChainResources(WindowResource & resource) noexcept;

        [[nodiscard]] OptionalPtr<vk::UniqueShaderModule> FindShaderModule(const uint8_t * shader_data) noexcept;

        [[nodiscard]] bool UpdateBufferDescriptorSetInfo() noexcept;
        [[nodiscard]] MaybeInvalid<vk::UniquePipelineLayout> CreatePipelineLayout() noexcept;
        [[nodiscard]] OptionalPtr<vk::UniquePipeline> PreparePSO(const PSODeferredSettings & deferred_settings, PSO & pso) noexcept;

        [[nodiscard]] bool PrepareCommandBuffer(const WindowResource & resource) noexcept;
        [[nodiscard]] bool CompleteCommandBuffer(const WindowResource & resource) noexcept;
        [[nodiscard]] bool SubmitCommandBuffer(const WindowResource & resource) noexcept;

        template <typename Callback>
        void PushDeletionCallback(Callback && callback)
        {
            m_FrameResources[m_FrameIndex].m_DeletionCallbacks.emplace_back(std::forward<Callback>(callback));
        }

    private:

        vk::UniqueInstance m_Instance;

#if !defined(NDEBUG)
        vk::UniqueDebugUtilsMessengerEXT m_DebugUtilsMessenger;
#endif

        vk::PhysicalDevice m_PhysicalDevice;
        vk::UniqueDevice m_Device;
        vk::Queue m_Queue;
        vma::UniqueAllocator m_Allocator;
        vk::UniqueCommandPool m_CommandPool;

        std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
        std::chrono::time_point<std::chrono::steady_clock> m_LastRenderTime;

        Map<const uint8_t*, vk::UniqueShaderModule> m_ShaderModules;

        PSOTable m_PSOTable;

        Vector<BufferType> m_BufferTypes;

        BufferTypeId m_GlobalBufferTypeId;
        BufferTypeId m_QuadBufferTypeId;

        struct FrameResource
        {
            Vector<UniquePtr<TransientBuffer>> m_Buffers;
            vk::UniqueCommandBuffer m_CommandBuffer;
            vk::UniqueFence m_Fence;

            vk::DescriptorSet m_BufferDescriptorSet;

            Vector<Function<void()>> m_DeletionCallbacks;
        };

        vk::UniqueDescriptorSetLayout m_BufferDescriptorSetLayout;
        vk::UniqueDescriptorPool m_BufferDescriptorPool;
        size_t m_BufferDescriptorSetId = 0;

        vk::UniquePipelineLayout m_PipelineLayout;

        static constexpr int FrameResourceCount = 3;
        std::array<FrameResource, FrameResourceCount> m_FrameResources;
        uint32_t m_FrameIndex = 0;
    };


    export UniquePtr<RenderManager> g_RenderManager;

}
