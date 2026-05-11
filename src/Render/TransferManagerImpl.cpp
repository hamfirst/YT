
module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <array>
#include <condition_variable>
#include <mutex>

module YT:TransferManagerImpl;

import :TransferManager;

namespace YT
{
    TransferManager::TransferManager(vk::Device device, vk::Queue upload_queue,
        std::uint32_t upload_queue_family_index, bool enable_worker_thread)
        : m_Device(device)
        , m_UploadQueue(upload_queue)
        , m_UploadQueueFamilyIndex(upload_queue_family_index)
    {
        vk::CommandPoolCreateInfo pool_info;
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = upload_queue_family_index;
        m_CommandPool = m_Device.createCommandPoolUnique(pool_info);

        if (enable_worker_thread)
        {
            m_WorkerSync = std::make_unique<WorkerSync>();
            m_WorkerThread.emplace(
                [this](std::stop_token st)
                {
                    Run(st);
                });
        }
    }

    TransferManager::~TransferManager() noexcept
    {
        if (m_WorkerThread)
        {
            m_WorkerThread->request_stop();
            if (m_WorkerSync)
            {
                const std::lock_guard lock(m_WorkerSync->Mutex);
                m_WorkerSync->Cv.notify_all();
            }
        }
    }

    void TransferManager::Run(std::stop_token stop_token) noexcept
    {
        if (!m_WorkerSync)
        {
            return;
        }

        while (!stop_token.stop_requested())
        {
            std::unique_lock lock(m_WorkerSync->Mutex);
            m_WorkerSync->Cv.wait(lock,
                [&stop_token]
                {
                    return stop_token.stop_requested();
                });
        }
    }

    vk::UniqueCommandBuffer TransferManager::AllocatePrimaryUploadCommandBuffer()
    {
        vk::CommandBufferAllocateInfo allocate_info;
        allocate_info.commandPool = m_CommandPool.get();
        allocate_info.level = vk::CommandBufferLevel::ePrimary;
        allocate_info.commandBufferCount = 1;

        auto buffer_list = m_Device.allocateCommandBuffersUnique(allocate_info);
        return std::move(buffer_list.front());
    }

    vk::Result TransferManager::SubmitToUploadQueue(vk::CommandBuffer command_buffer, vk::Fence fence)
    {
        vk::CommandBufferSubmitInfo command_buffer_submit_info;
        command_buffer_submit_info.setCommandBuffer(command_buffer);

        std::array command_buffer_submit_infos{ command_buffer_submit_info };

        vk::SubmitInfo2 submit_info;
        submit_info.setCommandBufferInfos(command_buffer_submit_infos);

        return m_UploadQueue.submit2(1, &submit_info, fence);
    }
}
