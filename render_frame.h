/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "common/buffer_pool.h"
#include "core/fence_pool.h"
#include "core/semaphore_pool.h"
#include "core/device.h"
#include "core/command_buffer.h"
#include "common/resource_caching.h"

namespace frame {
	namespace core {
		class CommandBuffer;
	}
    namespace rendering {
        enum class BufferAllocationStrategy {
            OneAllocationPerBuffer,
            MultipleAllocationsPerBuffer
        };

        enum class DescriptorManagementStrategy {
            StoreInCache,
            CreateDirectly
        };
        
        class RenderFrame {
        public:
            RenderFrame(core::Device& device, std::unique_ptr<RenderTarget>&& render_target, size_t thread_count = 1);
            
            void clearDescriptors();
            core::Device& getDevice();
            const core::FencePool& getFencePool() const;
            RenderTarget& getRenderTarget();
            RenderTarget const& getRenderTarget() const;
            const core::SemaphorePool& getSemaphorePool() const;
            void releaseOwnedSemaphore(vk::Semaphore semaphore);
            vk::DescriptorSet requestDescriptorSet(const core::DescriptorSetLayoutCPP& descriptor_set_layout,
                const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
                const BindingMap<vk::DescriptorImageInfo>& image_infos,
                bool update_after_bind,
                size_t thread_index = 0);
            vk::Fence requestFence();
            vk::Semaphore requestSemaphore();
            vk::Semaphore requestSemaphoreWithOwnership();
            void reset();
            
            common::BufferAllocation allocateBuffer(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index = 0);
            
            core::CommandBuffer& requestCommandBuffer(const core::Queue& queue,
                core::CommandBuffer::ResetMode reset_mode = core::CommandBuffer::ResetMode::ResetPool,
                vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary,
                size_t thread_index = 0);
            
            void setBufferAllocationStrategy(BufferAllocationStrategy new_strategy);
            
            void setDescriptorManagementStrategy(DescriptorManagementStrategy new_strategy);
            
            void updateRenderTarget(std::unique_ptr<RenderTarget>&& render_target);
            
            void updateDescriptorSets(size_t thread_index = 0);

        private:
            std::vector<std::unique_ptr<core::CommandPool>>& getCommandPools(const core::Queue& queue,
                core::CommandBuffer::ResetMode reset_mode);

            static std::vector<uint32_t> collectBindingsToUpdate(const core::DescriptorSetLayoutCPP& descriptor_set_layout,
                const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
                const BindingMap<vk::DescriptorImageInfo>& image_infos);

        private:
            const std::unordered_map<vk::BufferUsageFlags, uint32_t> m_supported_usage_map = {
                {vk::BufferUsageFlagBits::eUniformBuffer, 1},
                {vk::BufferUsageFlagBits::eStorageBuffer, 2},
                {vk::BufferUsageFlagBits::eVertexBuffer, 1},
                {vk::BufferUsageFlagBits::eIndexBuffer, 1} };

            core::Device& m_device;
            std::map<uint32_t, std::vector<std::unique_ptr<core::CommandPool>>> m_command_pools;
            std::vector<std::unique_ptr<std::unordered_map<std::size_t, core::DescriptorPoolCPP>>> m_descriptor_pools;
            std::vector<std::unique_ptr<std::unordered_map<std::size_t, core::DescriptorSetCPP>>> m_descriptor_sets;
            core::FencePool m_fence_pool;
            core::SemaphorePool m_semaphore_pool;
            size_t m_thread_count;
            std::unique_ptr<RenderTarget> m_swapchain_render_target;
            BufferAllocationStrategy m_buffer_allocation_strategy{ BufferAllocationStrategy::MultipleAllocationsPerBuffer };
            DescriptorManagementStrategy m_descriptor_management_strategy{ DescriptorManagementStrategy::StoreInCache };
            std::map<vk::BufferUsageFlags, std::vector<std::pair<common::BufferPool, common::BufferBlock*>>> m_buffer_pools;
        };
    }
}
