module;

#include <memory>
#include <iostream>
#include <unordered_map>
#include <any>

#include <glm/glm.hpp>

#include <stb_image.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

// Loaded extensions
PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator,
                                                              VkDebugUtilsMessengerEXT* pMessenger)
{
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const* pAllocator)
{
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

module YT:RenderManagerImpl;

import :Types;
import :BlockTable;
import :TransientBuffer;
import :RenderTypes;
import :WindowResource;
import :WindowManager;
import :RenderManager;
import :RenderReflect;
import :Drawer;

VKAPI_ATTR static VkBool32 VKAPI_CALL DebugMessageFunc(
    vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
    vk::DebugUtilsMessageTypeFlagsEXT message_types,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
    void* /*user_data*/)
{
    std::string message;

    message += vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)) + ": " +
        vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_types)) + ":\n";
    message += std::string("\t") + "messageIDName   = <" + callback_data->pMessageIdName + ">\n";
    message += std::string("\t") + "messageIdNumber = " + std::to_string(callback_data->messageIdNumber) + "\n";
    message += std::string("\t") + "message         = <" + callback_data->pMessage + ">\n";
    if (callback_data->queueLabelCount > 0)
    {
        message += std::string("\t") + "Queue Labels:\n";
        for (uint32_t i = 0; i < callback_data->queueLabelCount; i++)
        {
            message += std::string("\t\t") + "labelName = <" + callback_data->pQueueLabels[i].pLabelName + ">\n";
        }
    }
    if (callback_data->cmdBufLabelCount > 0)
    {
        message += std::string("\t") + "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < callback_data->cmdBufLabelCount; i++)
        {
            message += std::string("\t\t") + "labelName = <" + callback_data->pCmdBufLabels[i].pLabelName + ">\n";
        }
    }
    if (callback_data->objectCount > 0)
    {
        for (uint32_t i = 0; i < callback_data->objectCount; i++)
        {
            message += std::string("\t") + "Object " + std::to_string(i) + "\n";
            message += std::string("\t\t") + "objectType   = " + vk::to_string(
                static_cast<vk::ObjectType>(callback_data->pObjects[i].objectType)) + "\n";
            message += std::string("\t\t") + "objectHandle = " + std::to_string(
                callback_data->pObjects[i].objectHandle) + "\n";
            if (callback_data->pObjects[i].pObjectName)
            {
                message += std::string("\t\t") + "objectName   = <" + callback_data->pObjects[i].pObjectName + ">\n";
            }
        }
    }

    YT::FatalPrint("{}", message);
    return false;
}

namespace YT
{
    bool RenderManager::CreateRenderManager(const ApplicationInitInfo & init_info) noexcept
    {
        try
        {
            g_RenderManager = std::make_unique<RenderManager>(init_info);
            return true;
        }
        catch (const Exception & e)
        {
            FatalPrint("Error creating render manager: {}", e.what());
        }
        catch (...)
        {
            FatalPrint("Unknown exception creating render manager");
        }

        return false;
    }

    RenderManager::RenderManager(const ApplicationInitInfo & init_info)
    {
        try
        {

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
            VULKAN_HPP_DEFAULT_DISPATCHER.init();
#endif

            auto CheckLayers = [](Vector<char const*> const& layers,
                                  Vector<vk::LayerProperties> const& properties)
            {
                // return true if all layers are listed in the properties
                return std::all_of(
                    layers.begin(),
                    layers.end(),
                    [&properties](char const* name)
                    {
                        return std::any_of(
                            properties.begin(),
                            properties.end(),
                            [&name](vk::LayerProperties const& property)
                            {
                                return strcmp(property.layerName, name) == 0;
                            });
                    });
            };

            constexpr int engine_version = 1;
            const char* engine_name = "YT";

            vk::ApplicationInfo application_info(
                init_info.m_ApplicationName.data(),
                init_info.m_ApplicationVersion,
                engine_name,
                engine_version,
                VK_API_VERSION_1_3);

            Vector<vk::LayerProperties> instance_layer_properties = vk::enumerateInstanceLayerProperties();

            Vector<char const*> instance_layer_names;

#if !defined(NDEBUG)
            instance_layer_names.push_back("VK_LAYER_KHRONOS_validation");
#endif
            if (!CheckLayers(instance_layer_names, instance_layer_properties))
            {
                throw Exception(
                    "Set the environment variable VK_LAYER_PATH to point to the location of your layers");
            }

            // Enable debug callback extension
            Vector<char const*> instance_extension_names;
#if !defined(NDEBUG)
            instance_extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
            instance_extension_names.push_back(g_WindowManager->GetSurfaceExtensionName());
            vk::InstanceCreateInfo create_info(
                vk::InstanceCreateFlags(),
                &application_info,
                instance_layer_names,
                instance_extension_names);
            m_Instance = vk::createInstanceUnique(create_info);

#if !defined(NDEBUG)

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
            // initialize function pointers for instance
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance.get());
#endif

            pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                m_Instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
            if (!pfnVkCreateDebugUtilsMessengerEXT)
            {
                throw Exception("GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function.");
            }

            pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                m_Instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
            if (!pfnVkDestroyDebugUtilsMessengerEXT)
            {
                throw Exception("GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.");
            }

            vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

            vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

            m_DebugUtilsMessenger = m_Instance->createDebugUtilsMessengerEXTUnique(
                    vk::DebugUtilsMessengerCreateInfoEXT({}, severity_flags, message_type_flags, &DebugMessageFunc));
#endif

            Vector<vk::PhysicalDevice> physical_devices = m_Instance->enumeratePhysicalDevices();
            if(physical_devices.empty())
            {
                throw Exception("No physical devices found");
            }

            // Pick which device to use
            Optional<vk::PhysicalDevice> best_physical_device;
            Optional<uint32_t> best_queue_index;

            std::array required_extensions =
                {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                };

            for(const vk::PhysicalDevice & physical_device : physical_devices)
            {
                // Check for extension support
                std::vector<std::string> extension_names;
                for(const vk::ExtensionProperties & extension : physical_device.enumerateDeviceExtensionProperties())
                {
                    extension_names.push_back(extension.extensionName);
                }

                bool has_required_extensions = true;
                for (auto extension_name : required_extensions)
                {
                    if (std::ranges::find(extension_names, extension_name) == extension_names.end())
                    {
                        has_required_extensions = false;
                        break;
                    }
                }

                if (!has_required_extensions)
                {
                    continue;
                }

                Vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

                for(size_t index = 0; index < queue_family_properties.size(); index++)
                {
                    const vk::QueueFamilyProperties& queue_family_info = queue_family_properties[index];
                    if ((queue_family_info.queueFlags & vk::QueueFlagBits::eGraphics) &&
                        (queue_family_info.queueFlags & vk::QueueFlagBits::eCompute) &&
                        (queue_family_info.queueFlags & vk::QueueFlagBits::eTransfer))
                    {
                        if(g_WindowManager->CheckDeviceSupport(physical_device, index))
                        {
                            best_physical_device = physical_device;
                            best_queue_index = index;
                        }
                    }
                }
            }

            if(!best_physical_device.has_value() || !best_queue_index.has_value())
            {
                throw Exception("No physical device found");
            }

            VerbosePrint("Found valid device: {} - {}", best_physical_device->getProperties().deviceName.data(),
                best_queue_index.value());

            m_PhysicalDevice = best_physical_device.value();

            // create a Device
            float graphics_queue_priority = 0.0f;
            vk::DeviceQueueCreateInfo device_queue_create_info(
                vk::DeviceQueueCreateFlags(),
                best_queue_index.value(), 1,
                &graphics_queue_priority);

            std::vector<const char *> device_layer_names {};

            vk::PhysicalDeviceDescriptorIndexingFeatures physical_device_descriptor_indexing_features;
            physical_device_descriptor_indexing_features.descriptorBindingSampledImageUpdateAfterBind = true;
            physical_device_descriptor_indexing_features.descriptorBindingPartiallyBound = true;
            physical_device_descriptor_indexing_features.descriptorBindingVariableDescriptorCount = true;

            vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR buffer_device_address_features(true);
            buffer_device_address_features.pNext = &physical_device_descriptor_indexing_features;

            vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features(true);
            dynamic_rendering_features.pNext = &buffer_device_address_features;

            vk::PhysicalDeviceFeatures device_features;
            device_features.shaderInt64 = true;

            vk::DeviceCreateInfo device_create_info(
                vk::DeviceCreateFlags(),
                device_queue_create_info,
                device_layer_names,
                required_extensions,
                &device_features, &dynamic_rendering_features);

            m_Device = m_PhysicalDevice.createDeviceUnique(device_create_info);

            // get the queues
            m_Device->getQueue(best_queue_index.value(), 0, &m_Queue);

            // create the command pool
            vk::CommandPoolCreateInfo command_pool_create_info;
            command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            command_pool_create_info.queueFamilyIndex = best_queue_index.value();
            m_CommandPool = m_Device->createCommandPoolUnique(command_pool_create_info);

            // create the image pool
            std::array descriptor_pool_sizes =
            {
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MaxImageDescriptors)
            };

            vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
            descriptor_pool_create_info.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
            descriptor_pool_create_info.maxSets = 1;
            descriptor_pool_create_info.setPoolSizes(descriptor_pool_sizes);
            m_ImageDescriptorPool = m_Device->createDescriptorPoolUnique(descriptor_pool_create_info);

            // create the image descriptor layout
            std::array image_set_bindings =
                {
                    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler,
                        MaxImageDescriptors, vk::ShaderStageFlagBits::eFragment)
                };

            std::array image_set_binding_flags =
                {
                    vk::DescriptorBindingFlagBits::ePartiallyBound |
                    vk::DescriptorBindingFlagBits::eVariableDescriptorCount |
                    vk::DescriptorBindingFlagBits::eUpdateAfterBind
                };

            vk::DescriptorSetLayoutBindingFlagsCreateInfo descriptor_set_layout_binding_flags_create_info;
            descriptor_set_layout_binding_flags_create_info.setBindingFlags(image_set_binding_flags);

            vk::DescriptorSetLayoutCreateInfo image_set_layout_create_info;
            image_set_layout_create_info.setBindings(image_set_bindings);
            image_set_layout_create_info.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
            image_set_layout_create_info.pNext = &descriptor_set_layout_binding_flags_create_info;
            m_ImageDescriptorSetLayout = m_Device->createDescriptorSetLayoutUnique(image_set_layout_create_info);

            std::array descriptor_set_layouts =
            {
                m_ImageDescriptorSetLayout.get(),
            };

            vk::DescriptorSetAllocateInfo descriptor_set_allocate_info;
            descriptor_set_allocate_info.descriptorPool = m_ImageDescriptorPool.get();
            descriptor_set_allocate_info.setSetLayouts(descriptor_set_layouts);

            auto descriptor_sets = m_Device->allocateDescriptorSets(
                descriptor_set_allocate_info);

            m_ImageDescriptorSet = descriptor_sets.front();

            // create the vma allocator
            vma::AllocatorCreateInfo allocator_create_info;
            allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
            allocator_create_info.instance = m_Instance.get();
            allocator_create_info.physicalDevice = m_PhysicalDevice;
            allocator_create_info.device = m_Device.get();

            m_Allocator = vma::createAllocatorUnique(allocator_create_info);

            // create per-frame buffers
            vk::FenceCreateInfo fence_create_info;
            fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

            vk::CommandBufferAllocateInfo allocate_info;
            allocate_info.commandPool = m_CommandPool.get();
            allocate_info.level = vk::CommandBufferLevel::ePrimary;
            allocate_info.commandBufferCount = 1;

            for (auto & frame_resource : m_FrameResources)
            {
                frame_resource.m_Fence = m_Device->createFenceUnique(fence_create_info);

                auto buffer_list = m_Device->allocateCommandBuffersUnique(allocate_info);
                frame_resource.m_CommandBuffer = std::move(buffer_list.front());
            }

            if (!UpdateBufferDescriptorSetInfo())
            {
                throw Exception("Failed to update buffer descriptor set");
            }

            m_StartTime = std::chrono::steady_clock::now();
            m_LastRenderTime = m_StartTime;
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Failed to create vk instance: {}", err.what());
            throw Exception("vk::SystemError");
        }
    }

    RenderManager::~RenderManager()
    {
        m_Device->waitIdle();

        m_PSOTable.Clear();

        for (auto & frame_resource : m_FrameResources)
        {
            for (const auto & func : frame_resource.m_DeletionCallbacks)
            {
                func();
            }
        }

    }

    bool RenderManager::CreateWindowResources(const WindowInitInfo & init_info, WindowResource & resource) noexcept
    {
        try
        {
            resource.m_AlphaBackground = init_info.m_AlphaBackground;
            resource.m_WantsRedraw = true;

            if (!g_WindowManager->CreateRenderSurface(m_Instance, resource))
            {
                return false;
            }

            if (!CreateSwapChainResources(resource))
            {
                return false;
            }

            vk::SemaphoreCreateInfo semaphore_create_info;
            vk::FenceCreateInfo fence_create_info;
            fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

            for (const vk::Image & swap_chain_image : resource.m_SwapChainImages)
            {
                resource.m_ImageAvailableSemaphores.emplace_back(
                    m_Device->createSemaphoreUnique(semaphore_create_info));

                resource.m_RenderFinishedSemaphores.emplace_back(
                    m_Device->createSemaphoreUnique(semaphore_create_info));

                resource.m_RenderFinishedFences.emplace_back(
                    m_Device->createFenceUnique(fence_create_info));
            }

            vk::CommandBufferAllocateInfo allocate_info;
            allocate_info.commandPool = m_CommandPool.get();
            allocate_info.level = vk::CommandBufferLevel::ePrimary;
            allocate_info.commandBufferCount = resource.m_SwapChainImages.size();

            resource.m_CommandBuffers = m_Device->allocateCommandBuffersUnique(allocate_info);

            return true;
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Failed to create window resource: {}", err.what());
        }
        catch (...)
        {
            FatalPrint("Failed to create window resource: unknown exception");
        }
        return false;
    }

    bool RenderManager::UpdateWindowResource(WindowResource & resource) noexcept
    {
        if (!CreateSwapChainResources(resource))
        {
            return false;
        }

        return true;
    }

    bool RenderManager::RenderWindowResources(const Vector<WindowResource*> & window_resources) noexcept
    {
        vk::Result result = vk::Result::eSuccess;

        // Sync the frame
        FrameResource & frame_resource = m_FrameResources[m_FrameIndex];

        result = m_Device->waitForFences(1, &frame_resource.m_Fence.get(), true, UINT64_MAX);
        if (result != vk::Result::eSuccess)
        {
            FatalPrint("Failed to wait for fences: {}", vk::to_string(result));
            return false;
        }

        result = m_Device->resetFences(1, &frame_resource.m_Fence.get());
        if (result != vk::Result::eSuccess)
        {
            FatalPrint("Failed to reset fences: {}", vk::to_string(result));
            return false;
        }

        m_PreRenderDelegate.Execute();

        if (!UpdateBufferDescriptorSetInfo())
        {
            return false;
        }

        // Map all resources for writing
        for (UniquePtr<TransientBuffer> & buffer : frame_resource.m_Buffers)
        {
            buffer->Begin();
        }

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> elapsed_since_start = now - m_StartTime;
        std::chrono::duration<double, std::milli> elapsed_since_last_render = now - m_LastRenderTime;

        GlobalData global_data;
        global_data.m_Time = static_cast<float>(elapsed_since_start.count() / 1000.0);
        global_data.m_DeltaTime = static_cast<float>(elapsed_since_last_render.count() / 1000.0);
        global_data.m_Random1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        global_data.m_Random2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        WriteToBuffer(GetGlobalBufferTypeId(), &global_data, sizeof(global_data));

        m_LastRenderTime = now;

        for (WindowResource * resource_ptr : window_resources)
        {
            WindowResource & resource = *resource_ptr;
            if (resource.m_Widget)
            {
                try
                {
                    result = m_Device->waitForFences(1, &resource.m_RenderFinishedFences[resource.m_FrameIndex].get(), true, UINT64_MAX);
                    if (result != vk::Result::eSuccess)
                    {
                        FatalPrint("Failed to wait for fences: {}", vk::to_string(result));
                        return false;
                    }

                    result = m_Device->resetFences(1, &resource.m_RenderFinishedFences[resource.m_FrameIndex].get());
                    if (result != vk::Result::eSuccess)
                    {
                        FatalPrint("Failed to reset fences: {}", vk::to_string(result));
                        return false;
                    }

                    if (resource.m_RequestedExtent != resource.m_SwapChainExtent)
                    {
                        VerbosePrint("Recreating swap chain due to requested size change");
                        if (!UpdateWindowResource(resource))
                        {
                            FatalPrint("Failed to update swap chain due to requested size");
                            return false;
                        }
                    }

                    result = m_Device->acquireNextImageKHR(resource.m_SwapChain.get(), UINT64_MAX,
                            resource.m_ImageAvailableSemaphores[resource.m_FrameIndex].get(), {}, &resource.m_SwapChainImageIndex);

                    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
                    {
                        VerbosePrint("Recreating swap chain due to vulkan response");
                        if (!UpdateWindowResource(resource))
                        {
                            FatalPrint("Failed to update swap chain due to vulkan response");
                            return false;
                        }
                    }

                    if (!PrepareCommandBuffer(resource))
                    {
                        FatalPrint("Failed to prepare command buffer");
                        return false;
                    }

                    DrawerData drawer_data;
                    drawer_data.m_ViewportSize = glm::vec2(resource_ptr->m_SwapChainExtent.width, resource_ptr->m_SwapChainExtent.height);
                    drawer_data.m_Size = drawer_data.m_ViewportSize;
                    drawer_data.m_Offset = glm::vec2(0.0f);

                    PSODeferredSettings pso_deferred_settings;
                    pso_deferred_settings.m_SurfaceFormat = resource.m_SwapChainFormat;
                    pso_deferred_settings.m_BufferDescriptorSetId = m_BufferDescriptorSetId;

                    Drawer drawer(resource.m_CommandBuffers[resource.m_FrameIndex].get(), drawer_data, pso_deferred_settings);
                    resource.m_Widget->OnDraw(drawer);

                    drawer.Flush();

                    if (!CompleteCommandBuffer(resource))
                    {
                        FatalPrint("Failed to complete command buffer");
                        return false;
                    }
                }
                catch (vk::SystemError& err)
                {
                    FatalPrint("Failed to render window: {}", err.what());
                    return false;
                }
                catch (...)
                {
                    FatalPrint("Failed to render window: unknown exception");
                    return false;
                }
            }
        }

        // Finalize buffers
        for (UniquePtr<TransientBuffer> & buffer : frame_resource.m_Buffers)
        {
            buffer->End();
        }

        // Submit everything
        for (WindowResource * resource_ptr : window_resources)
        {
            WindowResource & resource = *resource_ptr;
            if (resource.m_Widget)
            {
                try
                {
                    if (!SubmitCommandBuffer(resource))
                    {
                        FatalPrint("Failed to submit command buffer");
                        return false;
                    }

                    resource.m_FrameIndex++;
                    if (resource.m_FrameIndex >= resource.m_SwapChainImages.size())
                    {
                        resource.m_FrameIndex = 0;
                    }
                }
                catch (vk::SystemError& err)
                {
                    FatalPrint("Failed to submit window: {}", err.what());
                    return false;
                }
                catch (...)
                {
                    FatalPrint("Failed to submit window: unknown exception");
                    return false;
                }
            }
        }

        try
        {
            // Submit the frame fence
            frame_resource.m_CommandBuffer->reset();

            vk::CommandBufferBeginInfo begin_info;
            frame_resource.m_CommandBuffer->begin(begin_info);

            vk::DebugUtilsLabelEXT label_info;
            label_info.color = std::array { 1.0f, 1.0f, 1.0f, 1.0f };
            label_info.pLabelName = "EOF";
            frame_resource.m_CommandBuffer->insertDebugUtilsLabelEXT(label_info);

            frame_resource.m_CommandBuffer->end();

            vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eAllCommands;

            vk::SubmitInfo submit_info;
            submit_info.setCommandBuffers(frame_resource.m_CommandBuffer.get());

            result = m_Queue.submit(1, &submit_info, frame_resource.m_Fence.get());
            if (result != vk::Result::eSuccess)
            {
                FatalPrint("Failed to submit end of frame: {}", vk::to_string(result));
                return false;
            }
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Failed to submit window: {}", err.what());
            return false;
        }
        catch (...)
        {
            FatalPrint("Failed to submit window: unknown exception");
            return false;
        }

        m_FrameIndex++;
        if (m_FrameIndex >= FrameResourceCount)
        {
            m_FrameIndex = 0;
        }

        try
        {
            // Do any queued deletes for last frame
            for (const auto & func : frame_resource.m_DeletionCallbacks)
            {
                func();
            }

            frame_resource.m_DeletionCallbacks.clear();
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Exception while running queued deletes: {}", err.what());
            return false;
        }
        catch (...)
        {
            FatalPrint("Exception while running queued deletes: unknown exception");
            return false;
        }

        m_PostRenderDelegate.Execute();
        return true;
    }

    void RenderManager::ReleaseWindowResource(WindowResource & resource) noexcept
    {
        m_Device->waitIdle();

        resource.m_SwapChainImageViews.clear();
        resource.m_SwapChainImages.clear();

        resource.m_SwapChain.reset();
        resource.m_VkSurface.reset();
    }

    void RenderManager::RegisterShader(const uint8_t* shader_data, std::size_t shader_data_size) noexcept
    {
        if (m_ShaderModules.contains(shader_data))
        {
            return;
        }

        vk::ShaderModuleCreateInfo shader_module_create_info;
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_data);
        shader_module_create_info.codeSize = shader_data_size;
        m_ShaderModules.emplace(shader_data, m_Device->createShaderModuleUnique(shader_module_create_info));
    }

    void RenderManager::UnregisterShader(const uint8_t * shader_data) noexcept
    {
        if (auto shader_module = m_ShaderModules.find(shader_data); shader_module != m_ShaderModules.end())
        {
            PushDeferredDeleteCallback([this, shader_module_ptr = shader_module->second.release()]()
            {
               m_Device->destroyShaderModule(shader_module_ptr);
            });

            m_ShaderModules.erase(shader_module);
        }
    }

    void RenderManager::SetShaderInclude(const StringView & include_name, const StringView & include_code) noexcept
    {
        m_ShaderBuilder.SetInclude(include_name, include_code);
    }

    bool RenderManager::CompileShader(const StringView & shader_code, ShaderType type,
        const StringView & file_name_for_log_output, Vector<uint32_t> & out_shader_data,
        const Optional<String> & entry_point) noexcept
    {
        vk::ShaderStageFlagBits vk_shader_type = vk::ShaderStageFlagBits::eFragment;
        switch (type)
        {
            case ShaderType::Vertex:
                vk_shader_type = vk::ShaderStageFlagBits::eVertex;
                break;
            case ShaderType::Fragment:
                vk_shader_type = vk::ShaderStageFlagBits::eFragment;
                break;
        }

        return m_ShaderBuilder.BuildShader(vk_shader_type, file_name_for_log_output,
            shader_code, out_shader_data, entry_point);
    }

    MaybeInvalid<PSOHandle> RenderManager::RegisterPSO(const PSOCreateInfo & create_info) noexcept
    {
        return MakeCustomBlockTableHandle<PSOHandle>(m_PSOTable.AllocateHandle(
            PSO
            {
                .m_CreateInfo = create_info,
            }));
    }

    bool RenderManager::BindPSO(vk::CommandBuffer & command_buffer, OptionalPtr<const void> push_data, size_t push_data_size,
                                const PSODeferredSettings & deferred_settings, PSOHandle handle) noexcept
    {
        PSO * pso = m_PSOTable.ResolveHandle(handle);
        if (!pso)
        {
            return false;
        }

        PSOVariant * target_variant = nullptr;
        for (PSOVariant & variant : pso->m_Variants)
        {
            if (variant.m_DeferredSettings == deferred_settings)
            {
                target_variant = &variant;
            }
        }

        if (!target_variant)
        {
            if (target_variant = PreparePSO(deferred_settings, *pso); !target_variant)
            {
                return false;
            }
        }

        vk::DescriptorSet descriptor_set = m_FrameResources[m_FrameIndex].m_BufferDescriptorSet;
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            target_variant->m_Layout.get(), 0, 1,
            &descriptor_set, 0, nullptr);

        if (push_data_size > 0 && push_data != nullptr)
        {
            command_buffer.pushConstants(target_variant->m_Layout.get(),
                vk::ShaderStageFlagBits::eAll, 0, push_data_size, push_data);
        }

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, target_variant->m_Pipeline.get());
        return true;
    }

    BufferTypeId RenderManager::RegisterBufferType(
        uint32_t element_size, uint32_t aligned_element_size, size_t buffer_size) noexcept
    {
        BufferTypeId buffer_type_id { .m_BufferTypeIndex = static_cast<uint32_t>(m_BufferTypes.size()) };
        m_BufferTypes.emplace_back(
            BufferType
            {
                .m_ElementSize = element_size,
                .m_AlignedSize = aligned_element_size,
                .m_BufferSize = buffer_size,
            });

        return buffer_type_id;
    }

    BufferTypeId RenderManager::GetGlobalBufferTypeId() const noexcept
    {
        return m_GlobalBufferTypeId;
    }

    BufferTypeId RenderManager::GetIndexBufferTypeId() const noexcept
    {
        return m_IndexBufferTypeId;
    }

    BufferDataHandle RenderManager::WriteToBuffer(BufferTypeId buffer_type_id, void * data, uint32_t data_size) noexcept
    {
        auto [ptr, buffer_handle] = ReserveBufferSpace(buffer_type_id, data_size);

        if (ptr)
        {
            memcpy(ptr, data, data_size);
        }

        return buffer_handle;
    }

    MaybeInvalid<Pair<std::byte *, BufferDataHandle>> RenderManager::ReserveBufferSpace(
        BufferTypeId buffer_type_id, uint32_t buffer_size) noexcept
    {
        TransientBuffer & buffer = *m_FrameResources[m_FrameIndex].m_Buffers[buffer_type_id.m_BufferTypeIndex];
        auto [ptr, offset] = buffer.ReserveSpace(buffer_size);

        BufferDataHandle data_handle
        {
            .m_Index = offset / m_BufferTypes[buffer_type_id.m_BufferTypeIndex].m_AlignedSize,
            .m_Type = static_cast<uint8_t>(buffer_type_id.m_BufferTypeIndex),
        };

        return MakePair(ptr, data_handle);
    }

    MaybeInvalid<ImageReference> RenderManager::CreateImage(const Span<const std::byte>& data) noexcept
    {
        int tex_width, tex_height, tex_channels;
        stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(data.data()),
            static_cast<int>(data.size()), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

        if (!pixels || tex_channels != STBI_rgb_alpha)
        {
            return {};
        }

        try
        {
            UniquePtr<StagingBuffer> staging_buffer = MakeUnique<StagingBuffer>(
                m_Device, m_Allocator, pixels, tex_width * tex_height * 4);

            vk::CommandBufferAllocateInfo allocate_info;
            allocate_info.commandPool = m_CommandPool.get();
            allocate_info.level = vk::CommandBufferLevel::ePrimary;
            allocate_info.commandBufferCount = 1;

            auto buffer_list = m_Device->allocateCommandBuffersUnique(allocate_info);
            vk::UniqueCommandBuffer command_buffer = std::move(buffer_list.front());

            vk::CommandBufferBeginInfo begin_info;
            command_buffer->begin(begin_info);

            auto handle = m_ImageTable.AllocateHandle(m_Device, m_Allocator, command_buffer.get(),
                *staging_buffer.get(), tex_width, tex_height, ImageFormat::R8G8B8A8Unorm);

            ImageHandle image_handle = MakeCustomBlockTableHandle<ImageHandle>(handle);
            uint32_t image_index = image_handle.m_BlockIndex * ImageTable::BlockSizeValue + image_handle.m_ElemIndex;

            command_buffer->end();

            PushDeferredDeleteObject(std::move(staging_buffer));
            PushDeferredDeleteObject(std::move(command_buffer));


        }
        catch (...)
        {
            FatalPrint("Failed to allocate staging buffer");
            return {};
        }

        return {};
    }

    void RenderManager::DestroyImage(ImageHandle handle) noexcept
    {

    }

    void RenderManager::RegisterRenderGlobals()
    {
        m_GlobalBufferTypeId = RegisterShaderBufferStruct<GlobalData>(1);
        m_IndexBufferTypeId = RegisterShaderBufferStruct<IndexData>(128 * 1024);
    }

    bool RenderManager::CreateSwapChainResources(WindowResource & resource) noexcept
    {
        try
        {
            VerbosePrint("Creating swap chain {} {}...", resource.m_RequestedExtent.width, resource.m_RequestedExtent.height);
            m_Device->waitIdle();

            resource.m_SwapChainImageViews.clear();
            resource.m_SwapChainImages.clear();

            vk::SurfaceCapabilitiesKHR surface_caps;
            if (vk::Result result = m_PhysicalDevice.getSurfaceCapabilitiesKHR(resource.m_VkSurface.get(), &surface_caps); result != vk::Result::eSuccess)
            {
                FatalPrint("Failed to get surface capabilities: {}", vk::to_string(result));
                return false;
            }

            Vector<vk::SurfaceFormatKHR> surface_formats = m_PhysicalDevice.getSurfaceFormatsKHR(resource.m_VkSurface.get());
            if (surface_formats.empty())
            {
                FatalPrint("Failed to get surface formats");
                return false;
            }

            Optional<vk::SurfaceFormatKHR> chosen_surface_format;
            for (vk::SurfaceFormatKHR & surface_format : surface_formats)
            {
                if (surface_format.format == vk::Format::eB8G8R8A8Srgb && surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                {
                    chosen_surface_format = surface_format;
                    break;
                }
            }

            if (!chosen_surface_format.has_value())
            {
                chosen_surface_format = surface_formats.front();
            }

            resource.m_SwapChainFormat = chosen_surface_format.value().format;

            vk::SwapchainCreateInfoKHR swap_chain_create_info;

            swap_chain_create_info.surface = resource.m_VkSurface.get();
            swap_chain_create_info.minImageCount = surface_caps.minImageCount;
            swap_chain_create_info.imageFormat = chosen_surface_format->format;
            swap_chain_create_info.imageColorSpace = chosen_surface_format->colorSpace;
            swap_chain_create_info.imageExtent = resource.m_RequestedExtent;
            swap_chain_create_info.imageArrayLayers = 1;
            swap_chain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            swap_chain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
            swap_chain_create_info.queueFamilyIndexCount = 0;
            swap_chain_create_info.pQueueFamilyIndices = nullptr;
            swap_chain_create_info.presentMode = vk::PresentModeKHR::eFifo;
            swap_chain_create_info.preTransform = surface_caps.currentTransform;
            swap_chain_create_info.compositeAlpha = resource.m_AlphaBackground ?
                vk::CompositeAlphaFlagBitsKHR::ePreMultiplied : vk::CompositeAlphaFlagBitsKHR::eOpaque;
            swap_chain_create_info.clipped = VK_TRUE;
            swap_chain_create_info.oldSwapchain = resource.m_SwapChain ? resource.m_SwapChain.get() : VK_NULL_HANDLE;

            resource.m_SwapChain = m_Device->createSwapchainKHRUnique(swap_chain_create_info);
            resource.m_SwapChainImages = m_Device->getSwapchainImagesKHR(resource.m_SwapChain.get());

            resource.m_SwapChainExtent = resource.m_RequestedExtent;

            for (const vk::Image & image : resource.m_SwapChainImages)
            {
                vk::ImageViewCreateInfo image_view_create_info;
                image_view_create_info.image = image;
                image_view_create_info.viewType = vk::ImageViewType::e2D;
                image_view_create_info.format = resource.m_SwapChainFormat;
                image_view_create_info.components.r = vk::ComponentSwizzle::eIdentity;
                image_view_create_info.components.g = vk::ComponentSwizzle::eIdentity;
                image_view_create_info.components.b = vk::ComponentSwizzle::eIdentity;
                image_view_create_info.components.a = vk::ComponentSwizzle::eIdentity;
                image_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                image_view_create_info.subresourceRange.baseMipLevel = 0;
                image_view_create_info.subresourceRange.levelCount = 1;
                image_view_create_info.subresourceRange.baseArrayLayer = 0;
                image_view_create_info.subresourceRange.layerCount = 1;

                resource.m_SwapChainImageViews.emplace_back(m_Device->createImageViewUnique(image_view_create_info));
            }

            return true;
        }
        catch (const vk::SystemError & e)
        {
            FatalPrint("Failed to create swapchain resources: {}", e.what());
            return false;
        }
        catch (...)
        {
            FatalPrint("Failed to create swapchain resources: unknown error");
            return false;
        }
    }

    OptionalPtr<vk::UniqueShaderModule> RenderManager::FindShaderModule(const uint8_t * shader_data) noexcept
    {
        auto itr = m_ShaderModules.find(shader_data);
        if (itr != m_ShaderModules.end())
        {
            return &itr->second;
        }

        return nullptr;
    }


    bool RenderManager::UpdateBufferDescriptorSetInfo() noexcept
    {
        if (m_BufferDescriptorSetId == m_BufferTypes.size())
        {
            return true;
        }

        for (FrameResource & frame_resource : m_FrameResources)
        {
            while (frame_resource.m_Buffers.size() < m_BufferTypes.size())
            {
                try
                {
                    size_t index = frame_resource.m_Buffers.size();
                    frame_resource.m_Buffers.emplace_back(
                        MakeUnique<TransientBuffer>(m_Device, m_Allocator, m_BufferTypes[index].m_BufferSize));
                }
                catch (vk::SystemError& err)
                {
                    FatalPrint("Failed to create buffer: {}", err.what());
                    return false;
                }
                catch (...)
                {
                    FatalPrint("Failed to create buffer: unknown exception");
                    return false;
                }
            }
        }

        try
        {
            if (m_BufferDescriptorPool.get())
            {
                PushDeferredDeleteCallback([this, buffer_pool = m_BufferDescriptorPool.release()]() mutable
                {
                    m_Device->destroyDescriptorPool(buffer_pool);
                });
            }

            if (m_BufferDescriptorSetLayout.get())
            {
                PushDeferredDeleteCallback([this, buffer_set_layout = m_BufferDescriptorSetLayout.release()]() mutable
                {
                    m_Device->destroyDescriptorSetLayout(buffer_set_layout);
                });
            }

            for (FrameResource & resource : m_FrameResources)
            {
                resource.m_BufferDescriptorSet = VK_NULL_HANDLE;
            }

            Vector<vk::DescriptorSetLayoutBinding> buffer_set_bindings;
            uint32_t buffer_set_binding_index = 0;
            for (const BufferType & buffer_type : m_BufferTypes)
            {
                buffer_set_bindings.emplace_back(
                    buffer_set_binding_index, vk::DescriptorType::eStorageBuffer, 1,
                    vk::ShaderStageFlagBits::eAll);

                buffer_set_binding_index++;
            }

            vk::DescriptorSetLayoutCreateInfo buffer_set_layout_create_info;
            buffer_set_layout_create_info.setBindings(buffer_set_bindings);
            m_BufferDescriptorSetLayout = m_Device->createDescriptorSetLayoutUnique(buffer_set_layout_create_info);

            std::array descriptor_pool_sizes =
            {
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer,
                    m_BufferTypes.size() * FrameResourceCount)
            };

            vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
            descriptor_pool_create_info.maxSets = FrameResourceCount;
            descriptor_pool_create_info.setPoolSizes(descriptor_pool_sizes);

            m_BufferDescriptorPool = m_Device->createDescriptorPoolUnique(descriptor_pool_create_info);

            std::array descriptor_set_layouts =
            {
                m_BufferDescriptorSetLayout.get(),
            };

            vk::DescriptorSetAllocateInfo descriptor_set_allocate_info;
            descriptor_set_allocate_info.descriptorPool = m_BufferDescriptorPool.get();
            descriptor_set_allocate_info.setSetLayouts(descriptor_set_layouts);

            for (FrameResource & resource : m_FrameResources)
            {
                auto descriptor_sets = m_Device->allocateDescriptorSets(
                    descriptor_set_allocate_info);

                resource.m_BufferDescriptorSet = descriptor_sets.front();

                Vector<vk::WriteDescriptorSet> write_descriptor_sets;
                write_descriptor_sets.reserve(resource.m_Buffers.size());

                Vector<vk::DescriptorBufferInfo> descriptor_buffer_infos;
                descriptor_buffer_infos.reserve(resource.m_Buffers.size());

                int binding_index = 0;
                for (UniquePtr<TransientBuffer> & buffer : resource.m_Buffers)
                {
                    descriptor_buffer_infos.emplace_back(buffer->GetBuffer(), 0, buffer->GetAllocationSize());

                    vk::WriteDescriptorSet & write_descriptor_set = write_descriptor_sets.emplace_back();
                    write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
                    write_descriptor_set.dstSet = resource.m_BufferDescriptorSet;
                    write_descriptor_set.dstBinding = binding_index;
                    write_descriptor_set.dstArrayElement = 0;
                    write_descriptor_set.descriptorCount = 1;
                    write_descriptor_set.pBufferInfo = &descriptor_buffer_infos.back();

                    binding_index++;
                }

                m_Device->updateDescriptorSets(
                    write_descriptor_sets.size(), write_descriptor_sets.data(),
                    0, nullptr);
            }

            m_BufferDescriptorSetId = m_BufferTypes.size();
            return true;
        }
        catch (const vk::SystemError & e)
        {
            FatalPrint("Failed to create buffer descriptor layout: {}", e.what());
        }
        catch (...)
        {
            FatalPrint("Failed to create buffer descriptor layout: unknown error");
        }

        return false;
    }

    OptionalPtr<PSOVariant> RenderManager::PreparePSO(const PSODeferredSettings & deferred_settings, PSO & pso) noexcept
    {
        vk::UniqueShaderModule* vertex_shader_module = FindShaderModule(pso.m_CreateInfo.m_VertexShader);
        vk::UniqueShaderModule* mesh_shader_module = FindShaderModule(pso.m_CreateInfo.m_MeshShader);
        vk::UniqueShaderModule* fragment_shader_module = FindShaderModule(pso.m_CreateInfo.m_FragmentShader);

        try
        {
            // create the layout
            std::array descriptor_set_layouts =
            {
                m_BufferDescriptorSetLayout.get(),
            };

            std::array push_constant_ranges =
            {
                vk::PushConstantRange(vk::ShaderStageFlagBits::eAll, 0, pso.m_CreateInfo.m_PushConstantsSize),
            };

            vk::PipelineLayoutCreateInfo layout_create_info;

            if (pso.m_CreateInfo.m_PushConstantsSize > 0)
            {
                layout_create_info.setPushConstantRanges(push_constant_ranges);
            }

            layout_create_info.setSetLayouts(descriptor_set_layouts);
            vk::UniquePipelineLayout layout = m_Device->createPipelineLayoutUnique(layout_create_info, nullptr);

            // set up shaders
            vk::PipelineShaderStageCreateInfo vertex_shader_stage_create_info;
            if (vertex_shader_module)
            {
                vertex_shader_stage_create_info.stage = vk::ShaderStageFlagBits::eVertex;
                vertex_shader_stage_create_info.module = vertex_shader_module->get();
                vertex_shader_stage_create_info.pName = pso.m_CreateInfo.m_VertexShaderEntryPoint.data();
            }
            else if (mesh_shader_module)
            {
                vertex_shader_stage_create_info.stage = vk::ShaderStageFlagBits::eMeshEXT;
                vertex_shader_stage_create_info.module = mesh_shader_module->get();
                vertex_shader_stage_create_info.pName = pso.m_CreateInfo.m_MeshShaderEntryPoint.data();
            }
            else
            {
                FatalPrint("Failed to create PSO - no vertex or mesh shader specified");
                return nullptr;
            }

            vk::PipelineShaderStageCreateInfo fragment_shader_stage_create_info;
            if (fragment_shader_module)
            {
                fragment_shader_stage_create_info.stage = vk::ShaderStageFlagBits::eFragment;
                fragment_shader_stage_create_info.module = fragment_shader_module->get();
                fragment_shader_stage_create_info.pName = pso.m_CreateInfo.m_FragmentShaderEntryPoint.data();
            }
            else
            {
                FatalPrint("Failed to create PSO - no fragment shader specified");
                return nullptr;
            }

            // set up dynamic state
            vk::PipelineDynamicStateCreateInfo dynamic_state_create_info;

            std::array dynamic_states =
            {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor,
            };

            dynamic_state_create_info.setDynamicStates(dynamic_states);

            // set up vertex inpus
            vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;

            vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
            input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

            // set up viewport
            vk::PipelineViewportStateCreateInfo viewport_state_create_info;
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.scissorCount = 1;

            // set up raster info
            vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
            rasterization_state_create_info.depthClampEnable = false;
            rasterization_state_create_info.rasterizerDiscardEnable = false;
            rasterization_state_create_info.polygonMode = vk::PolygonMode::eFill;
            rasterization_state_create_info.lineWidth = 1.0f;
            rasterization_state_create_info.cullMode = vk::CullModeFlagBits::eNone;
            rasterization_state_create_info.frontFace = vk::FrontFace::eClockwise;
            rasterization_state_create_info.depthBiasEnable = false;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

            vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
            multisample_state_create_info.sampleShadingEnable = false;

            // set up blend state
            std::array color_blend_attachment_states =
            {
                vk::PipelineColorBlendAttachmentState(
                    true,
                    vk::BlendFactor::eSrcAlpha,
                    vk::BlendFactor::eOneMinusSrcAlpha,
                    vk::BlendOp::eAdd,
                    vk::BlendFactor::eSrcAlpha,
                    vk::BlendFactor::eDstAlpha,
                    vk::BlendOp::eAdd,
                    vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags
                )
            };

            vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info;
            color_blend_state_create_info.logicOpEnable = false;
            color_blend_state_create_info.logicOp = vk::LogicOp::eCopy;
            color_blend_state_create_info.setAttachments(color_blend_attachment_states);

            // set up attachments
            std::array color_attachment_formats =
            {
                deferred_settings.m_SurfaceFormat
            };

            vk::PipelineRenderingCreateInfoKHR rendering_create_info;
            rendering_create_info.setColorAttachmentFormats(color_attachment_formats);

            // set up graphics pipeline
            std::array stages =
            {
                vertex_shader_stage_create_info,
                fragment_shader_stage_create_info,
            };

            vk::GraphicsPipelineCreateInfo pipeline_create_info;
            pipeline_create_info.pNext = &rendering_create_info;
            pipeline_create_info.setStages(stages);
            pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
            pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
            pipeline_create_info.pViewportState = &viewport_state_create_info;
            pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
            pipeline_create_info.pMultisampleState = &multisample_state_create_info;
            pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
            pipeline_create_info.pDepthStencilState = nullptr;
            pipeline_create_info.pDynamicState = &dynamic_state_create_info;
            pipeline_create_info.layout = layout.get();
            pipeline_create_info.renderPass = nullptr;
            pipeline_create_info.subpass = 0;
            pipeline_create_info.basePipelineHandle = nullptr;
            pipeline_create_info.basePipelineIndex = -1;
            auto pipeline_result = m_Device->createGraphicsPipelineUnique(nullptr, pipeline_create_info);

            if (pipeline_result.result != vk::Result::eSuccess)
            {
                FatalPrint("Failed to create PSO: {}", vk::to_string(pipeline_result.result));
                return nullptr;
            }

            PSOVariant variant;

            variant.m_DeferredSettings = deferred_settings;
            variant.m_Layout = std::move(layout);
            variant.m_Pipeline = std::move(pipeline_result.value);

            auto & result = pso.m_Variants.emplace_back(std::move(variant));

            // auto & result = pso.m_Variants.emplace_back(PSOVariant
            //     {
            //         .m_DeferredSettings = deferred_settings,
            //         .m_Layout = std::move(layout),
            //         .m_Pipeline = std::move(pipeline_result.value),
            //     });
            return &result;
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Failed to create PSO: {}", err.what());
        }
        catch (...)
        {
            FatalPrint("Failed to create PSO");
        }

        return nullptr;
    }

    bool RenderManager::PrepareCommandBuffer(const WindowResource & resource) noexcept
    {
        try
        {
            const vk::UniqueCommandBuffer & command_buffer = resource.m_CommandBuffers[resource.m_FrameIndex];

            command_buffer->reset();

            vk::CommandBufferBeginInfo begin_info;
            command_buffer->begin(begin_info);

            vk::ImageSubresourceRange subresource_range;
            subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresource_range.baseMipLevel = 0;
            subresource_range.levelCount = 1;
            subresource_range.baseArrayLayer = 0;
            subresource_range.layerCount = 1;

            vk::ImageMemoryBarrier barrier;
            barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            barrier.image = resource.m_SwapChainImages[resource.m_SwapChainImageIndex];
            barrier.setSubresourceRange(subresource_range);

            command_buffer->pipelineBarrier(
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                {},
                0, nullptr,
                0, nullptr,
                1, &barrier);

            std::array color_attachments =
            {
                vk::RenderingAttachmentInfoKHR
                (
                    resource.m_SwapChainImageViews[resource.m_SwapChainImageIndex].get(),
                    vk::ImageLayout::eAttachmentOptimalKHR,
                    vk::ResolveModeFlagBits::eNone,
                    {},
                    {},
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f)
                )
            };

            vk::Rect2D render_area(vk::Offset2D(), resource.m_SwapChainExtent);

            vk::RenderingInfoKHR rendering_info;
            rendering_info.renderArea = render_area;
            rendering_info.layerCount = 1;
            rendering_info.setColorAttachments(color_attachments);

            command_buffer->beginRendering(rendering_info);

            vk::Viewport viewport(0, 0,
                resource.m_SwapChainExtent.width, resource.m_SwapChainExtent.height, 0.0f, 1.0f);
            command_buffer->setViewport(0, 1, &viewport);

            command_buffer->setScissor(0, 1, &render_area);
            return true;
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Failed to prepare command buffer: {}", err.what());
        }
        catch (...)
        {
            FatalPrint("Failed to prepare command buffer: unknown exception");
        }

        return false;
    }

    bool RenderManager::CompleteCommandBuffer(const WindowResource & resource) noexcept
    {
        try
        {
            const vk::UniqueCommandBuffer & command_buffer = resource.m_CommandBuffers[resource.m_FrameIndex];
            command_buffer->endRendering();

            vk::ImageSubresourceRange subresource_range;
            subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresource_range.baseMipLevel = 0;
            subresource_range.levelCount = 1;
            subresource_range.baseArrayLayer = 0;
            subresource_range.layerCount = 1;

            vk::ImageMemoryBarrier barrier;
            barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
            barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
            barrier.image = resource.m_SwapChainImages[resource.m_SwapChainImageIndex];
            barrier.setSubresourceRange(subresource_range);

            command_buffer->pipelineBarrier(
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                {},
                0, nullptr,
                0, nullptr,
                1, &barrier);

            command_buffer->end();
            return true;
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Failed to complete command buffer: {}", err.what());
        }
        catch (...)
        {
            FatalPrint("Failed to complete command buffer: unknown exception");
        }

        return false;
    }

    bool RenderManager::SubmitCommandBuffer(const WindowResource & resource) noexcept
    {
        try
        {
            const vk::UniqueCommandBuffer & command_buffer = resource.m_CommandBuffers[resource.m_FrameIndex];
            std::array image_avail_semaphores = { resource.m_ImageAvailableSemaphores[resource.m_FrameIndex].get() };
            std::array render_finish_semaphores = { resource.m_RenderFinishedSemaphores[resource.m_FrameIndex].get() };
            std::array cmd_buffers = { command_buffer.get() };
            std::array swap_chains = { resource.m_SwapChain.get() };
            std::array image_indices = { resource.m_SwapChainImageIndex };

            vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

            vk::SubmitInfo submit_info;
            submit_info.setWaitSemaphores(image_avail_semaphores);
            submit_info.setWaitDstStageMask(pipe_stage_flags);
            submit_info.setCommandBuffers(cmd_buffers);
            submit_info.setSignalSemaphores(render_finish_semaphores);

            vk::Result result = m_Queue.submit(1, &submit_info,
                resource.m_RenderFinishedFences[resource.m_FrameIndex].get());

            if (result != vk::Result::eSuccess)
            {
                FatalPrint("Failed to submit command buffer submission: {}", vk::to_string(result));
                return false;
            }

            vk::PresentInfoKHR present_info;
            present_info.setWaitSemaphores(render_finish_semaphores);
            present_info.setSwapchains(swap_chains);
            present_info.setImageIndices(image_indices);

            result = m_Queue.presentKHR(&present_info);

            if (result != vk::Result::eSuccess)
            {
                FatalPrint("Failed to preset command buffer submission: {}", vk::to_string(result));
                return false;
            }

            return true;
        }
        catch (vk::SystemError& err)
        {
            FatalPrint("Failed to submit command buffer: {}", err.what());
        }
        catch (...)
        {
            FatalPrint("Failed to submit command buffer: unknown exception");
        }
        return false;
    }

}
