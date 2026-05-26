module;

#include <memory>
#include <optional>
#include <thread>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

export module YT:TransferManager;

import :Types;
import :RenderTypes;
import :MultiProducerSingleConsumer;
import :StagingBuffer;
import :OpaqueBuffer;
import :ObjectPool;

namespace YT
{
    export struct AsyncCopyInfo
    {
        const void * m_SourceData = nullptr;
        void * m_TargetData = nullptr;
        std::uint64_t m_CopySize = 0;

        OpaqueBuffer m_Buffer;
    };

    /**
     * Owns upload-queue resources for buffer-to-image (and related) transfers: a CommandPool scoped to the
     * upload queue family. When \p enable_worker_thread is true, starts a worker thread scaffold for future
     * async submissions; when false (single-queue fallback), uploads are recorded and submitted from the caller.
     * Constructed after vk::Device and upload vk::Queue are valid.
     */
    export class TransferManager final
    {
    public:
        TransferManager(vk::Device device, vk::Queue upload_queue, bool enable_worker_thread);
        TransferManager(const TransferManager &) = delete;
        TransferManager & operator=(const TransferManager &) = delete;
        TransferManager(TransferManager &&) = delete;
        TransferManager & operator=(TransferManager &&) = delete;

        ~TransferManager() noexcept;

        void TransferToFullImage(ImageHandle target_image, void * src_data, std::size_t src_data_size);
        void TransferToPartialImage(ImageHandle target_image, void * src_data, std::size_t src_data_size,
            int dst_x, int dst_y, int src_width, int src_height);

    private:
        [[nodiscard]] vk::UniqueCommandBuffer AllocatePrimaryUploadCommandBuffer();

        [[nodiscard]] vk::Result SubmitToUploadQueue(vk::CommandBuffer command_buffer,
            vk::Fence fence = vk::Fence{});

        void Run() noexcept;

        struct WorkerSync;

    private:

        vk::Device m_Device{};
        vk::Queue m_TransferQueue{};

        vk::UniqueCommandPool m_CommandPool;

        std::atomic_bool m_Running = false;
        std::counting_semaphore<> m_Semaphore;

        Thread m_WorkerThread;

        Vector<StagingBuffer> m_PendingStagingBuffers;
        ObjectPool<StagingBuffer> m_StagingBufferPool;
    };
}
