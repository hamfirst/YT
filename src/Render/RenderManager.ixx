module;

//import_std
#include <cstddef>
#include <cstdint>
#include <span>
#include <array>
#include <variant>
#include <atomic>
#include <queue>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <any>
#include <mutex>
#include <type_traits>

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
import :CoroEvent;
import :TransferManager;


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

    private:
        void CreatePhysicalDevice(const ApplicationInitInfo & init_info);
        void CreateLogicalDevice();
        void CreateQueue();
        void CreateCommandPool();
        void CreateTransferManager();
        void CreateFrameSemaphore();

        void CreateImageDescriptorPool();
        void CreateImageDescriptorLayout();
        void CreateImageDescriptorSet();

        void CreateVideoMemoryAllocator();

        void CreateFrameResources();

    public:

        Delegate<void ()> & GetPreRenderDelegate() noexcept { return m_PreRenderDelegate; }
        Delegate<void ()> & GetPostRenderDelegate() noexcept { return m_PostRenderDelegate; };

        bool CreateWindowResources(const WindowInitInfo & init_info, WindowResource & resource) noexcept;
        bool UpdateWindowResource(WindowResource & resource) noexcept;

        bool RenderWindowResources(const Vector<WindowResource*> & window_resources) noexcept;
        void ReleaseWindowResource(WindowResource & resource) noexcept;

        void CleanupImmediately();

        void RegisterShader(const std::uint8_t* shader_data, std::size_t shader_data_size) noexcept;
        void UnregisterShader(const std::uint8_t* shader_data) noexcept;
        void SetShaderInclude(const StringView & include_name, const StringView & include_code) noexcept;
        bool CompileShader(const StringView & shader_code, ShaderType type,
            const StringView & file_name_for_log_output, Vector<std::uint8_t> & out_shader_data,
            const Optional<String> & entry_point = {}) noexcept;

        [[nodiscard]] MaybeInvalid<PSOHandle> RegisterPSO(const PSOCreateInfo & create_info) noexcept;
        bool BindPSO(vk::CommandBuffer & command_buffer, OptionalPtr<const void> push_data, std::size_t push_data_size,
            const PSODeferredSettings & deferred_settings, PSOHandle handle) noexcept;

        BufferTypeId RegisterBufferType(std::uint32_t element_size,
            std::uint32_t aligned_element_size, std::size_t buffer_size) noexcept;

        BufferTypeId GetGlobalBufferTypeId() const noexcept;
        BufferTypeId GetIndexBufferTypeId() const noexcept;

        BufferDataHandle WriteToBuffer(BufferTypeId buffer_type_id, void * data, std::uint32_t data_size) noexcept;

        [[nodiscard]] MaybeInvalid<Pair<std::byte *, BufferDataHandle>> ReserveBufferSpace(
            BufferTypeId buffer_type_id, std::uint32_t buffer_size) noexcept;

        void CreateInternalTextures() noexcept;

        [[nodiscard]] MaybeInvalid<ImageReference> CreateImage(const Span<const std::byte>& image_file_data) noexcept;
        [[nodiscard]] MaybeInvalid<ImageReference> CreateImageFromPixels(
            const Span<const std::byte>& data, std::uint32_t width, std::uint32_t height, ImageFormat format) noexcept;

        [[nodiscard]] MaybeInvalid<ImageReference> CreateImageFromNativeHandle(
            std::uint64_t native_handle, std::uint32_t width, std::uint32_t height) noexcept;

        void FinalizeDeferredImageLoad() noexcept;
        void DestroyImage(ImageHandle handle) noexcept;

        [[nodiscard]] const ImageReference & GetWhiteImageReference() const noexcept { return m_WhiteImage; }
        [[nodiscard]] const ImageReference & GetBlackImageReference() const noexcept { return m_BlackImage; }

        void RegisterRenderGlobals();

        CoroEvent & GetImageGenerationReadyEvent() noexcept { return m_ImageGenerationReadyEvent; }

        template <typename Callback>
        void PushDeferredDeleteCallback(std::uint64_t timeline_value, Callback && callback)
        {
            m_DeferredDeleteInfos.emplace(DeferredDeleteInfo{ timeline_value,
                std::forward<Callback>(callback) });
        }

        template <typename ObjectType>
        void PushDeferredDeleteObject(std::uint64_t timeline_value, ObjectType && obj)
        {
           m_DeferredDeleteInfos.emplace(DeferredDeleteInfo{ timeline_value,
               DeferredDelete(std::forward<ObjectType>(obj)) });
        }

        template <typename ObjectType>
        void PushDeferredDeleteObjectList(std::uint64_t timeline_value, Vector<ObjectType> & obj_list)
        {
            for (ObjectType & obj : obj_list)
            {
                PushDeferredDeleteObject(timeline_value, std::move(obj));
            }

            obj_list.clear();
        }

    protected:

        bool CreateSwapChainResources(WindowResource & resource) noexcept;

        [[nodiscard]] OptionalPtr<vk::UniqueShaderModule> FindShaderModule(const std::uint8_t * shader_data) noexcept;

        bool UpdateBufferDescriptorSetInfo() noexcept;
        [[nodiscard]] OptionalPtr<PSOVariant> PreparePSO(const PSODeferredSettings & deferred_settings, PSO & pso) noexcept;

        bool PrepareCommandBufferForPresent(const WindowResource & resource) noexcept;
        bool CompleteCommandBufferForPresent(const WindowResource & resource) noexcept;
        bool PresentWindow(const WindowResource & resource) noexcept;

        bool SubmitImageUploadCommandBuffer() noexcept;

        std::uint64_t AllocateTimelineSemaphoreValue() noexcept;

    private:

        // Vulkan data
        vk::UniqueInstance m_Instance;

#if !defined(NDEBUG)
        vk::UniqueDebugUtilsMessengerEXT m_DebugUtilsMessenger;
#endif

        int m_BestDeviceIndex = -1;

        std::uint32_t m_TransferQueueIndexInFamily = 0;
        std::uint32_t m_DeviceGraphicsFamilyQueueCount = 1;

        vk::Queue m_TransferQueue;
        Vector<const char *> m_RequiredExtensions;

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

        // ShadersCleanup
        Map<const std::uint8_t*, vk::UniqueShaderModule> m_ShaderModules;
        ShaderBuilder m_ShaderBuilder;

        // PSOs
        PSOTable m_PSOTable;

        // Dynamic buffer data
        Vector<BufferType> m_BufferTypes;

        BufferTypeId m_GlobalBufferTypeId;
        BufferTypeId m_IndexBufferTypeId;

        vk::UniqueDescriptorSetLayout m_BufferDescriptorSetLayout;
        vk::UniqueDescriptorPool m_BufferDescriptorPool;
        std::size_t m_BufferDescriptorSetId = 0;

        // Images
        static constexpr std::size_t MaxImageDescriptors = 10000;
        vk::UniqueDescriptorSetLayout m_ImageDescriptorSetLayout;
        vk::UniqueDescriptorPool m_ImageDescriptorPool;
        vk::UniqueDescriptorSet m_ImageDescriptorSet;

        ImageTable m_ImageTable;
        ImageReference m_WhiteImage;
        ImageReference m_BlackImage;

        CoroEvent m_ImageGenerationReadyEvent;

        struct ImageTransferInfo
        {
            ImageHandle m_ImageHandle;
            vk::Image m_Image = nullptr;
            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
        };

        Vector<vk::ImageMemoryBarrier2> m_ImagePreTransferMemoryBarriers;
        Vector<ImageTransferInfo> m_ImageTransferInfos;
        Vector<UniquePtr<StagingBuffer>> m_ImageTransferStagingBuffers;
        Vector<vk::ImageMemoryBarrier2> m_ImagePostTransferMemoryBarriers;

        // Frame resources
        struct FrameResource
        {
            Vector<UniquePtr<TransientBuffer>> m_Buffers;
            vk::UniqueCommandBuffer m_CommandBuffer;

            vk::DescriptorSet m_BufferDescriptorSet;
        };

        static constexpr int FrameResourceCount = 3;
        std::array<FrameResource, FrameResourceCount> m_FrameResources;
        std::uint64_t m_FrameIndex = 0;

        vk::UniqueSemaphore m_FrameSemaphore;

        UniquePtr<TransferManager> m_TransferManager;

        // Deletion data
        struct DeferredDeleteInfo
        {
            std::uint64_t m_TimelineValue = 0;
            std::variant<DeferredDelete, Function<void()>> m_Data;
        };

        std::queue<DeferredDeleteInfo> m_DeferredDeleteInfos;
    };



    UniquePtr<RenderManager> g_RenderManager;

}
