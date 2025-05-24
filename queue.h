/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <vulkan/vulkan.hpp>

namespace frame {
	namespace core {
        class CommandBuffer;
        class Device;

        class Queue {
        public:
            Queue(Device& device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present, uint32_t index);

            Queue(const Queue&) = default;
            Queue(Queue&& other) noexcept;
            Queue& operator=(const Queue&) = delete;
            Queue& operator=(Queue&&) = delete;

            const Device& getDevice() const { return m_device; }
            vk::Queue getHandle() const { return m_handle; }
            uint32_t getFamilyIndex() const { return m_family_index; }
            uint32_t getIndex() const { return m_index; }
            const vk::QueueFamilyProperties& getProperties() const { return m_properties; }
            vk::Bool32 supportPresent() const { return m_can_present; }

            void submit(const CommandBuffer& command_buffer, vk::Fence fence) const;
            vk::Result present(const vk::PresentInfoKHR& present_info) const;

        private:
            Device& m_device;
            vk::Queue m_handle{};
            uint32_t m_family_index{ 0 };
            uint32_t m_index{ 0 };
            vk::Bool32 m_can_present{ false };
            vk::QueueFamilyProperties m_properties{};
        };
	}
}