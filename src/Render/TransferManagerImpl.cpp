
module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <array>
#include <condition_variable>
#include <mutex>

module YT:TransferManagerImpl;

import :RenderTypes;
import :TransferManager;

namespace YT
{
    TransferManager::TransferManager(vk::Device device, vk::Queue upload_queue, bool enable_worker_thread)
        : m_Device(device)
        , m_TransferQueue(upload_queue)
        , m_ThreadSemaphore{0}
    {
        vk::CommandPoolCreateInfo pool_info;
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = GTransferQueueIndex;
        m_CommandPool = m_Device.createCommandPoolUnique(pool_info);

        vk::SemaphoreTypeCreateInfo semaphore_type_create_info;
        semaphore_type_create_info.setSemaphoreType(vk::SemaphoreType::eTimeline);
        semaphore_type_create_info.setInitialValue(m_TransferSemaphoreValue);

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.pNext = &semaphore_type_create_info;

        m_TransferSemaphore = m_Device.createSemaphoreUnique(semaphore_create_info);

        if (enable_worker_thread)
        {
            m_WorkerThread = std::thread(&TransferManager::Run, this);
        }
    }

    TransferManager::~TransferManager() noexcept
    {
        if (m_WorkerThread.joinable())
        {
            m_Running = false;
            m_ThreadSemaphore.release();
            m_WorkerThread.join();
        }
    }

    void TransferManager::Run() noexcept
    {
        while (m_Running)
        {
            m_ThreadSemaphore.acquire();


        }
    }

    // vk::UniqueCommandBuffer TransferManager::AllocatePrimaryUploadCommandBuffer()
    // {
    //     vk::CommandBufferAllocateInfo allocate_info;
    //     allocate_info.commandPool = m_CommandPool.get();
    //     allocate_info.level = vk::CommandBufferLevel::ePrimary;
    //     allocate_info.commandBufferCount = 1;
    //
    //     auto buffer_list = m_Device.allocateCommandBuffersUnique(allocate_info);
    //     return std::move(buffer_list.front());
    // }
    //
    // vk::Result TransferManager::SubmitToUploadQueue(vk::CommandBuffer command_buffer, vk::Fence fence)
    // {
    //     vk::CommandBufferSubmitInfo command_buffer_submit_info;
    //     command_buffer_submit_info.setCommandBuffer(command_buffer);
    //
    //     std::array command_buffer_submit_infos{ command_buffer_submit_info };
    //
    //     vk::SubmitInfo2 submit_info;
    //     submit_info.setCommandBufferInfos(command_buffer_submit_infos);
    //
    //     return m_TransferQueue.submit2(1, &submit_info, fence);
    // }
}
