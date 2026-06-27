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
import :ImageBuffer;
import :OpaqueBuffer;
import :ObjectPool;

namespace YT
{
    export class TransferManager final
    {
    public:
        TransferManager(vk::Device device, vk::Queue upload_queue, bool enable_worker_thread);
        TransferManager(const TransferManager &) = delete;
        TransferManager & operator=(const TransferManager &) = delete;
        TransferManager(TransferManager &&) = delete;
        TransferManager & operator=(TransferManager &&) = delete;

        ~TransferManager() noexcept;

        void TransferToFullImage(ImageBuffer * target_image, void * src_data, std::size_t src_data_size);
        void TransferToPartialImage(ImageBuffer *  target_image, void * src_data, std::size_t src_data_size,
            std::uint32_t dst_x, std::uint32_t dst_y, std::uint32_t src_width, std::uint32_t src_height);

    private:
        void Run() noexcept;

    private:

        vk::Device m_Device{};
        vk::Queue m_TransferQueue{};

        vk::UniqueCommandPool m_CommandPool;
        vk::UniqueSemaphore m_TransferSemaphore{};
        std::uint64_t m_TransferSemaphoreValue = 0;

        std::atomic_bool m_Running = false;
        std::counting_semaphore<> m_ThreadSemaphore;

        Thread m_WorkerThread;

        Vector<StagingBuffer> m_PendingStagingBuffers;
        ObjectPool<StagingBuffer> m_StagingBufferPool;

    private:

        struct ImageCopyInfo
        {
            StagingBuffer * m_StagingBuffer = nullptr;
            ImageBuffer * m_ImageBuffer = nullptr;

            std::uint64_t m_BufferOffset = 0;
            std::uint32_t m_DestinationX = 0;
            std::uint32_t m_DestinationY = 0;

            std::uint32_t m_SourceWidth = 0;
            std::uint32_t m_SourceHeight = 0;
        };

        struct BufferCopyInfo
        {
            StagingBuffer * m_StagingBuffer = nullptr;

        };

        struct TransferWork
        {
            Vector<vk::ImageMemoryBarrier2> m_PreTransferBarriers;
            Vector<vk::ImageMemoryBarrier2> m_PostTransferBarriers;

            Vector<ImageCopyInfo> m_ImageCopyInfos;

            uint64_t m_WaitSemaphoreValue = 0;
            uint64_t m_SignalSemaphoreValue = 0;
        };
    };
}
