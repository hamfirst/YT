module;

#include <memory>
#include <optional>
#include <thread>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

export module YT:TransferManager;

import :ImageBuffer;
import :MultiProducerSingleConsumer;

namespace YT
{
    export class TransferManager final
    {
    public:
        TransferManager(vk::Device device, vk::Queue upload_queue, std::uint32_t upload_queue_family_index,
            bool enable_worker_thread);
        TransferManager(const TransferManager &) = delete;
        TransferManager & operator=(const TransferManager &) = delete;
        TransferManager(TransferManager &&) = delete;
        TransferManager & operator=(TransferManager &&) = delete;

        ~TransferManager() noexcept;

        void TransferEntireImage(ImageBuffer & target_image, const Span<const std::byte> & src_data);
        void TransferPartialImage(ImageBuffer & target_image, std::uint32_t x, std::uint32_t y,
            std::uint32_t width, std::uint32_t height, const Span<const std::byte> & src_data);


    private:
        [[nodiscard]] vk::UniqueCommandBuffer AllocatePrimaryUploadCommandBuffer();

        [[nodiscard]] vk::Result SubmitToUploadQueue(vk::CommandBuffer command_buffer,
            vk::Fence fence = vk::Fence{});

        void Run(std::stop_token stop_token) noexcept;

        struct WorkerSync;

    private:

        vk::Device m_Device{};
        vk::Queue m_UploadQueue{};
        std::uint32_t m_UploadQueueFamilyIndex = 0;

        vk::UniqueCommandPool m_CommandPool;

        std::atomic_bool m_Running = false;
        std::counting_semaphore<> m_Semaphore;

        std::thread m_WorkerThread;


    };
}
