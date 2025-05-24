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

#pragma once

#include "core/resource_cache.h"
#include "core/command_pool.h"
#include <vulkan/vulkan.hpp>

namespace frame {
    namespace core {

	    class DebugUtils;
	    class PhysicalDevice;
        class CommandPool;
        class Queue;
        class FencePool;
        class ResourceCache;

        class Device : public VulkanResource<vk::Device> {
        public:
            Device(PhysicalDevice& physical_device,
                vk::SurfaceKHR surface,
                std::unique_ptr<DebugUtils>&& debug_utils,
                std::unordered_map<const char*, bool> requested_extensions = {});

            Device(Device&) = delete;
            Device(Device&&) = delete;
            Device& operator=(Device&) = delete;
            Device& operator=(Device&&) = delete;
            
            ~Device();

            PhysicalDevice const& getPhysicalDevice() const;
            DebugUtils const& getDebugUtils() const;
            Queue const& getQueue(uint32_t queue_family_index, uint32_t queue_index) const;
            Queue const& getQueueByFlags(vk::QueueFlags queue_flags, uint32_t queue_index) const;
            Queue const& getQueueByPresent(uint32_t queue_index) const;
            Queue const& getSuitableGraphicsQueue() const;
            CommandPool& getCommandPool();
            FencePool& getFencePool();
            ResourceCache& getResourceCache();
            uint32_t getQueueFamilyIndex(vk::QueueFlagBits queue_flag) const;
            
            bool isExtensionSupported(std::string const& extension) const;
            bool isEnabled(std::string const& extension) const;
            bool isImageFormatSupported(vk::Format format) const;
            
            std::pair<vk::Image, vk::DeviceMemory> createImage(
                vk::Format format,
                vk::Extent2D const& extent,
                uint32_t mip_levels,
                vk::ImageUsageFlags usage,
                vk::MemoryPropertyFlags properties) const;
            
            vk::CommandBuffer createCommandBuffer(
                vk::CommandBufferLevel level,
                bool begin = false) const;

            void flushCommandBuffer(
                vk::CommandBuffer command_buffer,
                vk::Queue queue,
                bool free = true,
                vk::Semaphore signal_semaphore = vk::Semaphore()) const;

        private:
            PhysicalDevice const& m_physical_device;
            vk::SurfaceKHR m_surface{ nullptr };
            std::unique_ptr<DebugUtils> m_debug_utils;
            std::vector<const char*> m_enabled_device_extensions{};
            std::vector<std::vector<Queue>> m_queues;
            std::unique_ptr<CommandPool> m_command_pool;
            std::unique_ptr<FencePool> m_fence_pool;
            ResourceCache m_resource_cache;
        };
    }
}
