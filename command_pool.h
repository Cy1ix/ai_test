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

#pragma once

#include <cstdint>
#include "core/command_buffer.h"

namespace frame {
    namespace rendering {
        class RenderFrame;
    }

    namespace core {

        class Device;
        class CommandBuffer;
        
        class CommandPool : public VulkanResource<vk::CommandPool> {
        public:
            CommandPool(Device& device,
                uint32_t queue_family_index,
                rendering::RenderFrame* render_frame = nullptr,
                size_t thread_index = 0,
                CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool);
            CommandPool(const CommandPool&) = delete;
            CommandPool(CommandPool&& other);
            ~CommandPool();

            CommandPool& operator=(const CommandPool&) = delete;
            CommandPool& operator=(CommandPool&&) = delete;
            
            uint32_t getQueueFamilyIndex() const;
            rendering::RenderFrame* getRenderFrame();
            CommandBuffer::ResetMode getResetMode() const;
            size_t getThreadIndex() const;
            CommandBuffer& requestCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
            void resetPool();

        private:
            void resetCommandBuffers();
            
            rendering::RenderFrame* m_render_frame = nullptr;
            size_t m_thread_index = 0;
            uint32_t m_queue_family_index = 0;
            std::vector<std::unique_ptr<CommandBuffer>> m_primary_command_buffers;
            uint32_t m_active_primary_command_buffer_count = 0;
            std::vector<std::unique_ptr<CommandBuffer>> m_secondary_command_buffers;
            uint32_t m_active_secondary_command_buffer_count = 0;
            CommandBuffer::ResetMode m_reset_mode = CommandBuffer::ResetMode::ResetPool;
        };
    }
}