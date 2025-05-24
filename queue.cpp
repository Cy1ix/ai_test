/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include "core/queue.h"
#include "core/command_buffer.h"
#include "core/device.h"

namespace frame {
    namespace core {
        Queue::Queue(Device& device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present, uint32_t index) :
            m_device{ device },
            m_family_index{ family_index },
            m_index{ index },
            m_can_present{ can_present },
            m_properties{ properties }
        {
            m_handle = m_device.getHandle().getQueue(family_index, index);
        }

        Queue::Queue(Queue&& other) noexcept :
            m_device(other.m_device),
            m_handle(std::exchange(other.m_handle, {})),
            m_family_index(std::exchange(other.m_family_index, {})),
            m_index(std::exchange(other.m_index, 0)),
            m_can_present(std::exchange(other.m_can_present, false)),
            m_properties(std::exchange(other.m_properties, {}))
        {}

        void Queue::submit(const CommandBuffer& command_buffer, vk::Fence fence) const {
            vk::CommandBuffer commandBuffer = command_buffer.getHandle();
            vk::SubmitInfo submit_info({}, {}, commandBuffer);
            m_handle.submit(submit_info, fence);
        }

        vk::Result Queue::present(const vk::PresentInfoKHR& present_info) const {
            return m_can_present ? m_handle.presentKHR(present_info) : vk::Result::eErrorIncompatibleDisplayKHR;
        }
    }
}
