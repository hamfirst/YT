module;

#include <memory>
#include <unordered_map>
#include <chrono>
#include <any>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

module YT:RenderManager;

import :Types;
import :RenderTypes;
import :WindowResource;
import :BlockTable;
import :TransientBuffer;
import :StagingBuffer;
import :ImageBuffer;
import :ImageReference;
import :ShaderBuilder;
import :BitAllocator;
import :ObjectPool;
import :DeferredDelete;
import :Delegate;

namespace YT
{
    struct PSO
    {
        PSOCreateInfo m_CreateInfo;
        Vector<PSOVariant> m_Variants;
    };

    using PSOTable = BlockTable<PSO>;

    using ImageTable = BlockTable<ImageBuffer>;

    class RenderManager final
    {
    public:
        static bool CreateRenderManager(const ApplicationInitInfo & init_info) noexcept;

        explicit RenderManager(const ApplicationInitInfo & init_info);
        ~RenderManager();

    public:

        Delegate<void ()> & GetPreRenderDelegate() noexcept { return m_PreRenderDelegate; }
        Delegate<void ()> & GetPostRenderDelegate() noexcept { return m_PostRenderDelegate; };

        bool CreateWindowResources(const WindowInitInfo & init_info, WindowResource & resource) noexcept;
        bool UpdateWindowResource(WindowResource & resource) noexcept;

        bool RenderWindowResources(const Vector<WindowResource*> & window_resources) noexcept;
        void ReleaseWindowResource(WindowResource & resource) noexcept;

        void RegisterShader(const uint8_t* shader_data, std::size_t shader_data_size) noexcept;
        void UnregisterShader(const uint8_t* shader_data) noexcept;
        void SetShaderInclude(const StringView & include_name, const StringView & include_code) noexcept;
        [[nodiscard]] bool CompileShader(const StringView & shader_code, ShaderType type,
            const StringView & file_name_for_log_output, Vector<uint32_t> & out_shader_data,
            const Optional<String> & entry_point = {}) noexcept;

        [[nodiscard]] MaybeInvalid<PSOHandle> RegisterPSO(const PSOCreateInfo & create_info) noexcept;
        [[nodiscard]] bool BindPSO(vk::CommandBuffer & command_buffer, OptionalPtr<const void> push_data, size_t push_data_size,
            const PSODeferredSettings & deferred_settings, PSOHandle handle) noexcept;

        BufferTypeId RegisterBufferType(uint32_t element_size,
            uint32_t aligned_element_size, size_t buffer_size) noexcept;

        BufferTypeId GetGlobalBufferTypeId() const noexcept;
        BufferTypeId GetIndexBufferTypeId() const noexcept;

        BufferDataHandle WriteToBuffer(BufferTypeId buffer_type_id, void * data, uint32_t data_size) noexcept;

        [[nodiscard]] MaybeInvalid<Pair<std::byte *, BufferDataHandle>> ReserveBufferSpace(
            BufferTypeId buffer_type_id, uint32_t buffer_size) noexcept;

        [[nodiscard]] MaybeInvalid<ImageReference> CreateImage(const Span<const std::byte>& data) noexcept;
        void DestroyImage(ImageHandle handle) noexcept;

        void RegisterRenderGlobals();


    protected:
;
        [[nodiscard]] bool CreateSwapChainResources(WindowResource & resource) noexcept;

        [[nodiscard]] OptionalPtr<vk::UniqueShaderModule> FindShaderModule(const uint8_t * shader_data) noexcept;

        [[nodiscard]] bool UpdateBufferDescriptorSetInfo() noexcept;
        [[nodiscard]] OptionalPtr<PSOVariant> PreparePSO(const PSODeferredSettings & deferred_settings, PSO & pso) noexcept;

        [[nodiscard]] bool PrepareCommandBuffer(const WindowResource & resource) noexcept;
        [[nodiscard]] bool CompleteCommandBuffer(const WindowResource & resource) noexcept;
        [[nodiscard]] bool SubmitCommandBuffer(const WindowResource & resource) noexcept;

        template <typename Callback>
        void PushDeferredDeleteCallback(Callback && callback)
        {
            m_FrameResources[m_FrameIndex].m_DeletionCallbacks.emplace_back(std::forward<Callback>(callback));
        }

        template <typename ObjectType>
        void PushDeferredDeleteObject(ObjectType && obj)
        {
            m_FrameResources[m_FrameIndex].m_DeferredDeletionObjects.emplace_back(std::forward<ObjectType>(obj));
        }

    private:

        // Vulkan data
        vk::UniqueInstance m_Instance;

#if !defined(NDEBUG)
        vk::UniqueDebugUtilsMessengerEXT m_DebugUtilsMessenger;
#endif

        vk::PhysicalDevice m_PhysicalDevice;
        vk::UniqueDevice m_Device;
        vk::Queue m_Queue;
        vma::UniqueAllocator m_Allocator;
        vk::UniqueCommandPool m_CommandPool;

        // Timers
        std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
        std::chrono::time_point<std::chrono::steady_clock> m_LastRenderTime;

        // Delegates
        Delegate<void ()> m_PreRenderDelegate;
        Delegate<void ()> m_PostRenderDelegate;

        // shaders
        Map<const uint8_t*, vk::UniqueShaderModule> m_ShaderModules;
        ShaderBuilder m_ShaderBuilder;

        // PSOs
        PSOTable m_PSOTable;

        // Dynamic buffer data
        Vector<BufferType> m_BufferTypes;

        BufferTypeId m_GlobalBufferTypeId;
        BufferTypeId m_IndexBufferTypeId;

        vk::UniqueDescriptorSetLayout m_BufferDescriptorSetLayout;
        vk::UniqueDescriptorPool m_BufferDescriptorPool;
        size_t m_BufferDescriptorSetId = 0;

        // Images
        static constexpr size_t MaxImageDescriptors = 100000;
        vk::UniqueDescriptorSetLayout m_ImageDescriptorSetLayout;
        vk::UniqueDescriptorPool m_ImageDescriptorPool;
        vk::DescriptorSet m_ImageDescriptorSet;

        ImageTable m_ImageTable;

        // Frame resources
        struct FrameResource
        {
            Vector<UniquePtr<TransientBuffer>> m_Buffers;
            vk::UniqueCommandBuffer m_CommandBuffer;
            vk::UniqueFence m_Fence;

            vk::DescriptorSet m_BufferDescriptorSet;

            Vector<Function<void()>> m_DeletionCallbacks;
            Vector<DeferredDelete> m_DeferredDeletionObjects;
        };

        static constexpr int FrameResourceCount = 3;
        std::array<FrameResource, FrameResourceCount> m_FrameResources;
        uint32_t m_FrameIndex = 0;

        ObjectPool<vk::UniqueSemaphore> m_SemaphorePool;
    };



    UniquePtr<RenderManager> g_RenderManager;

}
