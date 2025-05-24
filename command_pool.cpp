/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <core/command_pool.h>
#include <core/device.h>

namespace frame {
    namespace core {
        CommandPool::CommandPool(Device& device,
            uint32_t queue_family_index,
            frame::rendering::RenderFrame* render_frame,
            size_t thread_index,
            CommandBuffer::ResetMode reset_mode) :
				VulkanResource{ VK_NULL_HANDLE, &device },
    			m_render_frame{ render_frame },
    			m_thread_index{ thread_index },
    			m_reset_mode{ reset_mode }
        {
            vk::CommandPoolCreateFlags flags;
            switch (m_reset_mode) {
            case CommandBuffer::ResetMode::ResetIndividually:
            case CommandBuffer::ResetMode::AlwaysAllocate:
                flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
                break;
            case CommandBuffer::ResetMode::ResetPool:
            default:
                flags = vk::CommandPoolCreateFlagBits::eTransient;
                break;
            }

            vk::CommandPoolCreateInfo command_pool_create_info(flags, queue_family_index);
            setHandle(getDevice().getHandle().createCommandPool(command_pool_create_info));
        }

        CommandPool::CommandPool(CommandPool&& other) :
    		VulkanResource{ std::exchange(other.getHandle(), {}), std::move(&other.getDevice())},
            m_queue_family_index(std::exchange(other.m_queue_family_index, {})),
            m_primary_command_buffers{ std::move(other.m_primary_command_buffers) },
            m_active_primary_command_buffer_count(std::exchange(other.m_active_primary_command_buffer_count, {})),
            m_secondary_command_buffers{ std::move(other.m_secondary_command_buffers) },
            m_active_secondary_command_buffer_count(std::exchange(other.m_active_secondary_command_buffer_count, {})),
            m_render_frame(std::exchange(other.m_render_frame, {})),
            m_thread_index(std::exchange(other.m_thread_index, {})),
            m_reset_mode(std::exchange(other.m_reset_mode, {})) {
        }

        CommandPool::~CommandPool() {
            m_primary_command_buffers.clear();
            m_secondary_command_buffers.clear();

            if (hasHandle()) {
                getDevice().getHandle().destroyCommandPool(getHandle());
            }
        }
        
        uint32_t CommandPool::getQueueFamilyIndex() const {
            return m_queue_family_index;
        }

        frame::rendering::RenderFrame* CommandPool::getRenderFrame() {
            return m_render_frame;
        }

        size_t CommandPool::getThreadIndex() const {
            return m_thread_index;
        }

        void CommandPool::resetPool() {
            switch (m_reset_mode) {
            case CommandBuffer::ResetMode::ResetIndividually:
                resetCommandBuffers();
                break;

            case CommandBuffer::ResetMode::ResetPool:
                getDevice().getHandle().resetCommandPool(getHandle());
                resetCommandBuffers();
                break;

            case CommandBuffer::ResetMode::AlwaysAllocate:
                m_primary_command_buffers.clear();
                m_active_primary_command_buffer_count = 0;
                m_secondary_command_buffers.clear();
                m_active_secondary_command_buffer_count = 0;
                break;

            default:
                throw std::runtime_error("Unknown reset mode for command pools");
            }
        }

        CommandBuffer& CommandPool::requestCommandBuffer(vk::CommandBufferLevel level) {
            if (level == vk::CommandBufferLevel::ePrimary) {
                if (m_active_primary_command_buffer_count < m_primary_command_buffers.size()) {
                    return *m_primary_command_buffers[m_active_primary_command_buffer_count++];
                }

                m_primary_command_buffers.emplace_back(std::make_unique<CommandBuffer>(*this, level));
                m_active_primary_command_buffer_count++;
                return *m_primary_command_buffers.back();
            }
            else {
                if (m_active_secondary_command_buffer_count < m_secondary_command_buffers.size()) {
                    return *m_secondary_command_buffers[m_active_secondary_command_buffer_count++];
                }

                m_secondary_command_buffers.emplace_back(std::make_unique<CommandBuffer>(*this, level));
                m_active_secondary_command_buffer_count++;
                return *m_secondary_command_buffers.back();
            }
        }

        CommandBuffer::ResetMode CommandPool::getResetMode() const {
            return m_reset_mode;
        }

        void CommandPool::resetCommandBuffers() {
            for (auto& cmd_buf : m_primary_command_buffers) {
                cmd_buf->reset(m_reset_mode);
            }
            m_active_primary_command_buffer_count = 0;

            for (auto& cmd_buf : m_secondary_command_buffers) {
                cmd_buf->reset(m_reset_mode);
            }
            m_active_secondary_command_buffer_count = 0;
        }
    }
}
