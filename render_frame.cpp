/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/render_frame.h"
#include "core/queue.h"
#include "core/command_pool.h"
#include "common/buffer_pool.h"
#include "common/resource_caching.h"

constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

namespace frame {
    namespace rendering {
        RenderFrame::RenderFrame(core::Device& device, std::unique_ptr<RenderTarget>&& render_target, size_t thread_count) :
            m_device{ device },
            m_fence_pool{ device },
            m_semaphore_pool{ device },
            m_thread_count{ thread_count },
            m_swapchain_render_target{ std::move(render_target) }
        {
            for (auto& usage_it : m_supported_usage_map) {
                auto [buffer_pools_it, inserted] = m_buffer_pools.emplace(usage_it.first, std::vector<std::pair<common::BufferPool, common::BufferBlock*>>{});
                if (!inserted) {
                    throw std::runtime_error("[RenderFrame] ERROR: Failed to insert buffer pool");
                }

                for (size_t i = 0; i < m_thread_count; ++i) {
                    buffer_pools_it->second.emplace_back(std::make_pair(common::BufferPool{ m_device, static_cast<uint64_t>(BUFFER_POOL_BLOCK_SIZE * 1024 * usage_it.second), usage_it.first}, nullptr));
                }
            }

            for (size_t i = 0; i < m_thread_count; ++i) {
                m_descriptor_pools.push_back(std::make_unique<std::unordered_map<std::size_t, core::DescriptorPoolCPP>>());
                m_descriptor_sets.push_back(std::make_unique<std::unordered_map<std::size_t, core::DescriptorSetCPP>>());
            }
        }

        common::BufferAllocation RenderFrame::allocateBuffer(const vk::BufferUsageFlags usage, const vk::DeviceSize size, size_t thread_index) {

            assert(thread_index < m_thread_count && "[RenderFrame] ASSERT: Thread index is out of bounds");

            auto buffer_pool_it = m_buffer_pools.find(usage);
            if (buffer_pool_it == m_buffer_pools.end()) {
                LOGE("No buffer pool for buffer usage {}", vk::to_string(usage));
                return common::BufferAllocation{};
            }

            assert(thread_index < buffer_pool_it->second.size());
            auto& buffer_pool = buffer_pool_it->second[thread_index].first;
            auto& buffer_block = buffer_pool_it->second[thread_index].second;

            bool want_minimal_block = m_buffer_allocation_strategy == BufferAllocationStrategy::OneAllocationPerBuffer;

            if (want_minimal_block || !buffer_block || !buffer_block->canAllocate(size)) {
                buffer_block = &buffer_pool.requestBufferBlock(size, want_minimal_block);
            }

            return buffer_block->allocate(common::toU32(size));
        }

        void RenderFrame::clearDescriptors() {
            for (auto& desc_sets_per_thread : m_descriptor_sets) {
                desc_sets_per_thread->clear();
            }

            for (auto& desc_pools_per_thread : m_descriptor_pools) {
                for (auto& desc_pool : *desc_pools_per_thread) {
                    desc_pool.second.reset();
                }
            }
        }

        std::vector<uint32_t> RenderFrame::collectBindingsToUpdate(const core::DescriptorSetLayoutCPP& descriptor_set_layout,
            const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
            const BindingMap<vk::DescriptorImageInfo>& image_infos)
        {
            std::set<uint32_t> bindings_to_update;

            auto aggregate_binding_to_update = [&bindings_to_update, &descriptor_set_layout](const auto& infos_map) {
                for (const auto& [binding_index, ignored] : infos_map) {
                    if (!(descriptor_set_layout.getLayoutBindingFlag(binding_index) & vk::DescriptorBindingFlagBits::eUpdateAfterBind)) {
                        bindings_to_update.insert(binding_index);
                    }
                }
            };

            aggregate_binding_to_update(buffer_infos);
            aggregate_binding_to_update(image_infos);

            return { bindings_to_update.begin(), bindings_to_update.end() };
        }

        std::vector<std::unique_ptr<core::CommandPool>>& RenderFrame::getCommandPools(const core::Queue& queue,
            core::CommandBuffer::ResetMode reset_mode)
        {
            auto command_pool_it = m_command_pools.find(queue.getFamilyIndex());

            if (command_pool_it != m_command_pools.end()) {
                assert(!command_pool_it->second.empty());
                if (command_pool_it->second[0]->getResetMode() != reset_mode) {
                    m_device.getHandle().waitIdle();
                    m_command_pools.erase(command_pool_it);
                }
                else {
                    return command_pool_it->second;
                }
            }

            bool inserted = false;
            std::tie(command_pool_it, inserted) = m_command_pools.emplace(queue.getFamilyIndex(), std::vector<std::unique_ptr<core::CommandPool>>{});
            if (!inserted) {
                throw std::runtime_error("[RenderFrame] ERROR: Failed to insert command pool");
            }

            for (size_t i = 0; i < m_thread_count; i++) {
                command_pool_it->second.push_back(std::make_unique<core::CommandPool>(m_device, queue.getFamilyIndex(), this, i, reset_mode));
            }

            return command_pool_it->second;
        }

        core::Device& RenderFrame::getDevice() {
            return m_device;
        }
        
        const core::FencePool& RenderFrame::getFencePool() const {
            return m_fence_pool;
        }

        RenderTarget& RenderFrame::getRenderTarget() {
            return *m_swapchain_render_target;
        }

        RenderTarget const& RenderFrame::getRenderTarget() const {
            return *m_swapchain_render_target;
        }

        const core::SemaphorePool& RenderFrame::getSemaphorePool() const {
            return m_semaphore_pool;
        }

        void RenderFrame::releaseOwnedSemaphore(vk::Semaphore semaphore) {
            m_semaphore_pool.releaseOwnedSemaphore(semaphore);
        }

        core::CommandBuffer& RenderFrame::requestCommandBuffer(const core::Queue& queue,
            core::CommandBuffer::ResetMode reset_mode,
            vk::CommandBufferLevel level,
            size_t thread_index)
        {
            assert(thread_index < m_thread_count && "[RenderFrame] ASSERT: Thread index is out of bounds");

            auto& command_pools = getCommandPools(queue, reset_mode);

            auto command_pool_it =
                std::find_if(command_pools.begin(),
                    command_pools.end(),
                    [&thread_index](std::unique_ptr<core::CommandPool>& cmd_pool) { return cmd_pool->getThreadIndex() == thread_index; });
            assert(command_pool_it != command_pools.end());

            return (*command_pool_it)->requestCommandBuffer(level);
        }

        vk::DescriptorSet RenderFrame::requestDescriptorSet(const core::DescriptorSetLayoutCPP& descriptor_set_layout,
            const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
            const BindingMap<vk::DescriptorImageInfo>& image_infos,
            bool update_after_bind,
            size_t thread_index)
        {
            assert(thread_index < m_thread_count && "[RenderFrame] ASSERT: Thread index is out of bounds");
            assert(thread_index < m_descriptor_pools.size());

            auto& descriptor_pool = common::requestResources(m_device, nullptr, *m_descriptor_pools[thread_index], descriptor_set_layout);
            
            if (m_descriptor_management_strategy == DescriptorManagementStrategy::StoreInCache) {

                std::vector<uint32_t> bindings_to_update;

                if (update_after_bind) {
                    bindings_to_update = collectBindingsToUpdate(descriptor_set_layout, buffer_infos, image_infos);
                }

                assert(thread_index < m_descriptor_sets.size());
                auto& descriptor_set =
                    common::requestResources(m_device, nullptr, *m_descriptor_sets[thread_index],
                        descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
                descriptor_set.update(bindings_to_update);
                return descriptor_set.getHandle();
            }
            else {
                core::DescriptorSetCPP descriptor_set{ m_device, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos };
                descriptor_set.applyWrites();
                return descriptor_set.getHandle();
            }
        }

        vk::Fence RenderFrame::requestFence() {
            return m_fence_pool.requestFence();
        }

        vk::Semaphore RenderFrame::requestSemaphore() {
            return m_semaphore_pool.requestSemaphore();
        }

        vk::Semaphore RenderFrame::requestSemaphoreWithOwnership() {
            return m_semaphore_pool.requestSemaphoreWithOwnership();
        }

        void RenderFrame::reset() {
            if (m_fence_pool.wait() != vk::Result::eSuccess) {
                throw std::runtime_error("[RenderFrame] ERROR: Fence pool wait fail");
            }
            m_fence_pool.reset();

            for (auto& command_pools_per_queue : m_command_pools) {
                for (auto& command_pool : command_pools_per_queue.second) {
                    command_pool->resetPool();
                }
            }

            for (auto& buffer_pools_per_usage : m_buffer_pools) {
                for (auto& buffer_pool : buffer_pools_per_usage.second) {
                    buffer_pool.first.reset();
                    buffer_pool.second = nullptr;
                }
            }

            m_semaphore_pool.reset();

            if (m_descriptor_management_strategy == DescriptorManagementStrategy::CreateDirectly) {
                clearDescriptors();
            }
        }

        void RenderFrame::setBufferAllocationStrategy(BufferAllocationStrategy new_strategy) {
            m_buffer_allocation_strategy = new_strategy;
        }

        void RenderFrame::setDescriptorManagementStrategy(DescriptorManagementStrategy new_strategy) {
            m_descriptor_management_strategy = new_strategy;
        }

        void RenderFrame::updateDescriptorSets(size_t thread_index) {
            assert(thread_index < m_descriptor_sets.size());
            auto& thread_descriptor_sets = *m_descriptor_sets[thread_index];
            for (auto& descriptor_set_it : thread_descriptor_sets) {
                descriptor_set_it.second.update();
            }
        }

        void RenderFrame::updateRenderTarget(std::unique_ptr<RenderTarget>&& render_target) {
            m_swapchain_render_target = std::move(render_target);
        }
    }
}
