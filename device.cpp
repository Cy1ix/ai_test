/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Volk/volk.h>

#include "core/device.h"
#include "core/command_pool.h"
#include "core/physical_device.h"
#include "core/queue.h"
#include "core/fence_pool.h"
#include <vulkan/vulkan.hpp>

namespace frame {
    namespace core {
        namespace {

            void initVma(const Device& device) {

                VmaVulkanFunctions vma_vulkan_func{};
                vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
                vma_vulkan_func.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
                
                VkDevice vkDevice = static_cast<VkDevice>(device.getHandle());
                VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(device.getPhysicalDevice().getHandle());
                VkInstance vkInstance = static_cast<VkInstance>(device.getPhysicalDevice().getInstance().getHandle());
                
                vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
                vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
                vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
                vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
                vma_vulkan_func.vkCreateImage = vkCreateImage;
                vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
                vma_vulkan_func.vkDestroyImage = vkDestroyImage;
                vma_vulkan_func.vkFreeMemory = vkFreeMemory;
                vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
                vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
                vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
                vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
                vma_vulkan_func.vkMapMemory = vkMapMemory;
                vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
                vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;
                
                VmaAllocatorCreateInfo allocator_info{};
                allocator_info.physicalDevice = vkPhysicalDevice;
                allocator_info.device = vkDevice;
                allocator_info.instance = vkInstance;
                allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
                allocator_info.pVulkanFunctions = &vma_vulkan_func;
                
                bool can_get_memory_requirements = device.isExtensionSupported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
                bool has_dedicated_allocation = device.isExtensionSupported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

                if (can_get_memory_requirements && has_dedicated_allocation &&
                    device.isEnabled(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) &&
                    device.isEnabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME)) {

                    vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(
                        vkGetDeviceProcAddr(vkDevice, "vkGetBufferMemoryRequirements2KHR"));
                    vma_vulkan_func.vkGetImageMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(
                        vkGetDeviceProcAddr(vkDevice, "vkGetImageMemoryRequirements2KHR"));

                    if (vma_vulkan_func.vkGetBufferMemoryRequirements2KHR && vma_vulkan_func.vkGetImageMemoryRequirements2KHR) {
                        allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
                        LOGI("Dedicated Allocation enabled with function pointers");
                    }
                    else {
                        LOGW("Failed to get function pointers for Dedicated Allocation, feature disabled");
                    }
                }
                
                if (device.isExtensionSupported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
                    device.isEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
                	allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
                }
                
                if (device.isExtensionSupported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) &&
                    device.isEnabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
                    
                    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(
                        vkGetInstanceProcAddr(vkInstance, "vkGetPhysicalDeviceMemoryProperties2"));

                    if (vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR) {
                        allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
                        LOGI("Memory Budget extension enabled");
                    }
                    else {
                        LOGW("Failed to get function pointer for Memory Budget, feature disabled");
                    }
                }
                
                if (device.isExtensionSupported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) &&
                    device.isEnabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME)) {
                    
                    allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
                    LOGI("Memory Priority extension enabled");
                }
                
                if (device.isExtensionSupported(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) &&
                    device.isEnabled(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME)) {

                    vma_vulkan_func.vkBindBufferMemory2KHR = reinterpret_cast<PFN_vkBindBufferMemory2KHR>(
                        vkGetDeviceProcAddr(vkDevice, "vkBindBufferMemory2KHR"));
                    vma_vulkan_func.vkBindImageMemory2KHR = reinterpret_cast<PFN_vkBindImageMemory2KHR>(
                        vkGetDeviceProcAddr(vkDevice, "vkBindImageMemory2KHR"));

                    if (vma_vulkan_func.vkBindBufferMemory2KHR && vma_vulkan_func.vkBindImageMemory2KHR) {
                        allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
                        LOGI("Bind Memory 2 extension enabled");
                    }
                    else {
                        LOGW("Failed to get function pointers for Bind Memory 2, feature disabled");
                    }
                }
                
                if (device.isExtensionSupported(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) &&
                    device.isEnabled(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME)) {
                    allocator_info.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
                    LOGI("AMD Device Coherent Memory extension enabled");
                }

                auto& allocator = alloc::getMemoryAllocator();
                if (allocator == VK_NULL_HANDLE) {
                    VkResult result = vmaCreateAllocator(&allocator_info, &allocator);
                    if (result != VK_SUCCESS) {
                        LOGE("Failed to create VMA allocator: {}", vk::to_string(static_cast<vk::Result>(result)));
                        throw std::runtime_error("[Device] ERROR: Cannot create allocator");
                    }
                    LOGI("VMA allocator successfully created");
                }
            }
        }

        Device::Device(PhysicalDevice& physical_device,
            vk::SurfaceKHR surface,
            std::unique_ptr<DebugUtils>&& debug_utils,
            std::unordered_map<const char*, bool> requested_extensions) :
            m_physical_device{ physical_device },
            m_surface{ surface },
            m_debug_utils{ std::move(debug_utils) },
            m_resource_cache{ *this }
        {
            LOGI("[Device] Selected GPU: {}", physical_device.getProperties().deviceName.data());

            std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();
            std::vector<vk::DeviceQueueCreateInfo> queue_create_infos(queue_family_properties.size());
            std::vector<std::vector<float>> queue_priorities(queue_family_properties.size());

            for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index) {
                vk::QueueFamilyProperties const& queue_family_property = queue_family_properties[queue_family_index];

                if (physical_device.hasHighPriorityGraphicsQueue()) {
                    uint32_t graphics_queue_family = getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
                    if (graphics_queue_family == queue_family_index) {
                        queue_priorities[queue_family_index].reserve(queue_family_property.queueCount);
                        queue_priorities[queue_family_index].push_back(1.0f);
                        for (uint32_t i = 1; i < queue_family_property.queueCount; i++) {
                            queue_priorities[queue_family_index].push_back(0.5f);
                        }
                    }
                    else {
                        queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
                    }
                }
                else {
                    queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
                }

                vk::DeviceQueueCreateInfo& queue_create_info = queue_create_infos[queue_family_index];
                queue_create_info.queueFamilyIndex = queue_family_index;
                queue_create_info.queueCount = queue_family_property.queueCount;
                queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
            }

            bool can_get_memory_requirements = isExtensionSupported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
            bool has_dedicated_allocation = isExtensionSupported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

            if (can_get_memory_requirements && has_dedicated_allocation) {
                m_enabled_device_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
                m_enabled_device_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
                LOGI("Dedicated Allocation enabled");
            }

            if(isExtensionSupported(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME)) {
                m_enabled_device_extensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
            }

            if (isExtensionSupported(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME) &&
                isExtensionSupported(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME)) {

                auto perf_counter_features = physical_device.getExtensionFeatures<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
                auto host_query_reset_features = physical_device.getExtensionFeatures<vk::PhysicalDeviceHostQueryResetFeatures>();

                if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset) {
                    physical_device.addExtensionFeatures<vk::PhysicalDevicePerformanceQueryFeaturesKHR>().performanceCounterQueryPools = true;
                    physical_device.addExtensionFeatures<vk::PhysicalDeviceHostQueryResetFeatures>().hostQueryReset = true;
                    m_enabled_device_extensions.push_back(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
                    m_enabled_device_extensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
                    LOGI("Performance query enabled");
                }
            }

            std::vector<const char*> unsupported_extensions{};
            for (auto& extension : requested_extensions) {
                if (isExtensionSupported(extension.first)) {
                    m_enabled_device_extensions.emplace_back(extension.first);
                }
                else {
                    unsupported_extensions.emplace_back(extension.first);
                }
            }

            if (m_enabled_device_extensions.size() > 0) {
                LOGI("Device supports the following requested extensions:");
                for (auto& extension : m_enabled_device_extensions) {
                    LOGI("  \t{}", extension);
                }
            }

            if (unsupported_extensions.size() > 0) {
                auto error = false;
                for (auto& extension : unsupported_extensions) {
                    auto extension_is_optional = requested_extensions[extension];
                    if (extension_is_optional) {
                        LOGW("Optional device extension {} not available, some features may be disabled", extension);
                    }
                    else {
                        LOGE("Required device extension {} not available, cannot run", extension);
                        error = true;
                    }
                    LOGE("Required device extension {} not available, cannot run", extension);
                    error = true;
                }

                if (error) {
                    throw std::runtime_error("[Device] ERROR: Extensions not present");
                }
            }

            vk::DeviceCreateInfo create_info({}, queue_create_infos, {}, m_enabled_device_extensions, &physical_device.getMutableRequestedFeatures());

            create_info.pNext = physical_device.getExtensionFeatureChain();

            setHandle(physical_device.getHandle().createDevice(create_info));
            
            volkLoadDevice(getHandle());

            m_queues.resize(queue_family_properties.size());

            for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index) {
                vk::QueueFamilyProperties const& queue_family_property = queue_family_properties[queue_family_index];
                vk::Bool32 present_supported = physical_device.getHandle().getSurfaceSupportKHR(queue_family_index, m_surface);

                for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index) {
                    m_queues[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property, present_supported, queue_index);
                }
            }

            initVma(*this);

            m_command_pool = std::make_unique<CommandPool>(
                *this, getQueueByFlags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).getFamilyIndex());
            m_fence_pool = std::make_unique<FencePool>(*this);
        }

        Device::~Device() {
            m_resource_cache.clear();
            m_command_pool.reset();
            m_fence_pool.reset();
            alloc::shutdown();

            if (getHandle()) {
                getHandle().destroy();
            }
        }

        bool Device::isExtensionSupported(std::string const& extension) const {
            return m_physical_device.isExtensionSupported(extension);
        }

        bool Device::isEnabled(std::string const& extension) const {
            return std::find_if(m_enabled_device_extensions.begin(),
                m_enabled_device_extensions.end(),
                [&extension](const char* enabled_extension) {
                    return extension == enabled_extension;
                }) != m_enabled_device_extensions.end();
        }


        bool Device::isImageFormatSupported(vk::Format format) const {
            try {
                const auto format_properties = m_physical_device.getHandle().getImageFormatProperties(
                    format,
                    vk::ImageType::e2D,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eSampled,
                    {}
                );
                
                return true;
            }
            catch (const vk::SystemError& e) {
                if (e.code() == vk::Result::eErrorFormatNotSupported) {
                    return false;
                }
                throw std::runtime_error(std::string("Failed to check image format support: ") + e.what());
            }
        }

        PhysicalDevice const& Device::getPhysicalDevice() const { return m_physical_device; }
        
        DebugUtils const& Device::getDebugUtils() const { return *m_debug_utils; }

        Queue const& Device::getQueue(uint32_t queue_family_index, uint32_t queue_index) const { return m_queues[queue_family_index][queue_index]; }

        Queue const& Device::getQueueByFlags(vk::QueueFlags required_queue_flags, uint32_t queue_index) const {
            for (size_t queue_family_index = 0U; queue_family_index < m_queues.size(); ++queue_family_index) {
                Queue const& first_queue = m_queues[queue_family_index][0];
                vk::QueueFlags queue_flags = first_queue.getProperties().queueFlags;
                uint32_t queue_count = first_queue.getProperties().queueCount;

                if (((queue_flags & required_queue_flags) == required_queue_flags) && queue_index < queue_count) {
                    return m_queues[queue_family_index][queue_index];
                }
            }

            throw std::runtime_error("[Device] ERROR: Queue not found");
        }

        Queue const& Device::getQueueByPresent(uint32_t queue_index) const {
            for (uint32_t queue_family_index = 0U; queue_family_index < m_queues.size(); ++queue_family_index) {
                Queue const& first_queue = m_queues[queue_family_index][0];
                uint32_t queue_count = first_queue.getProperties().queueCount;

                if (first_queue.supportPresent() && queue_index < queue_count) {
                    return m_queues[queue_family_index][queue_index];
                }
            }

            throw std::runtime_error("[Device] ERROR: Queue not found");
        }

        uint32_t Device::getQueueFamilyIndex(vk::QueueFlagBits queue_flag) const {
            const auto& queue_family_properties = m_physical_device.getQueueFamilyProperties();

            if (queue_flag & vk::QueueFlagBits::eCompute) {
                for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
                    if ((queue_family_properties[i].queueFlags & queue_flag) &&
                        !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
                        return i;
                    }
                }
            }

            if (queue_flag & vk::QueueFlagBits::eTransfer) {
                for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
                    if ((queue_family_properties[i].queueFlags & queue_flag) &&
                        !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                        !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eCompute)) {
                        return i;
                    }
                }
            }

            for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
                if (queue_family_properties[i].queueFlags & queue_flag) {
                    return i;
                }
            }

            throw std::runtime_error("[Device] ERROR: Could not find a matching queue family index");
        }

        Queue const& Device::getSuitableGraphicsQueue() const {
            for (size_t queue_family_index = 0U; queue_family_index < m_queues.size(); ++queue_family_index) {
                Queue const& first_queue = m_queues[queue_family_index][0];
                uint32_t queue_count = first_queue.getProperties().queueCount;

                if (first_queue.supportPresent() && 0 < queue_count) {
                    return m_queues[queue_family_index][0];
                }
            }

            return getQueueByFlags(vk::QueueFlagBits::eGraphics, 0);
        }

        std::pair<vk::Image, vk::DeviceMemory> Device::createImage(
            vk::Format format,
            vk::Extent2D const& extent,
            uint32_t mip_levels,
            vk::ImageUsageFlags usage,
            vk::MemoryPropertyFlags properties) const {

            vk::ImageCreateInfo image_create_info(
                {},
                vk::ImageType::e2D,
                format,
                vk::Extent3D(extent, 1),
                mip_levels,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                usage
            );

            vk::Image image = getHandle().createImage(image_create_info);
            vk::MemoryRequirements memory_requirements = getHandle().getImageMemoryRequirements(image);
            vk::MemoryAllocateInfo memory_allocation(
                memory_requirements.size,
                m_physical_device.getMemoryType(memory_requirements.memoryTypeBits, properties)
            );

            vk::DeviceMemory memory = getHandle().allocateMemory(memory_allocation);
            getHandle().bindImageMemory(image, memory, 0);

            return std::make_pair(image, memory);
        }

        vk::CommandBuffer Device::createCommandBuffer(vk::CommandBufferLevel level, bool begin) const {
            assert(m_command_pool && "No command pool exists in the device");

            vk::CommandBuffer command_buffer = getHandle().allocateCommandBuffers({
                m_command_pool->getHandle(), level, 1
                }).front();

            if (begin) {
                command_buffer.begin(vk::CommandBufferBeginInfo());
            }

            return command_buffer;
        }

        void Device::flushCommandBuffer(
            vk::CommandBuffer command_buffer,
            vk::Queue queue,
            bool free,
            vk::Semaphore signal_semaphore) const {

            if (!command_buffer) {
                return;
            }

            command_buffer.end();

            vk::SubmitInfo submit_info({}, {}, command_buffer);
            if (signal_semaphore) {
                submit_info.setSignalSemaphores(signal_semaphore);
            }

            vk::Fence fence = getHandle().createFence({});

            queue.submit(submit_info, fence);

            vk::Result result = getHandle().waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
            if (result != vk::Result::eSuccess) {
                LOGE("[Device] Detected Vulkan error: {}", vk::to_string(result));
                abort();
            }

            getHandle().destroyFence(fence);

            if (m_command_pool && free) {
                getHandle().freeCommandBuffers(m_command_pool->getHandle(), command_buffer);
            }
        }

        CommandPool& Device::getCommandPool() {
            return *m_command_pool;
        }

        FencePool& Device::getFencePool() {
            return *m_fence_pool;
        }

        ResourceCache& Device::getResourceCache() {
            return m_resource_cache;
        }
    }
}
