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

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace frame {
	namespace core {
		class CommandBuffer;
		
		class DebugUtils {
		public:
			virtual ~DebugUtils() = default;
			
			virtual void setDebugName(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char* name) const = 0;
			
			virtual void setDebugTag(
				vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void* tag_data, size_t tag_data_size) const = 0;
			
			virtual void cmdBeginLabel(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color = {}) const = 0;
			
			virtual void cmdEndLabel(vk::CommandBuffer command_buffer) const = 0;
			
			virtual void cmdInsertLabel(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color = {}) const = 0;
		};
		
		class DebugUtilsExtDebugUtils final : public DebugUtils {
		public:
			~DebugUtilsExtDebugUtils() override = default;

			void setDebugName(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char* name) const override;

			void setDebugTag(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void* tag_data, size_t tag_data_size) const override;

			void cmdBeginLabel(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;

			void cmdEndLabel(vk::CommandBuffer command_buffer) const override;

			void cmdInsertLabel(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;
		};
		
		class DebugMarkerExtDebugUtils final : public DebugUtils {
		public:
			~DebugMarkerExtDebugUtils() override = default;

			void setDebugName(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char* name) const override;

			void setDebugTag(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void* tag_data, size_t tag_data_size) const override;

			void cmdBeginLabel(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;

			void cmdEndLabel(vk::CommandBuffer command_buffer) const override;

			void cmdInsertLabel(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;
		};
		
		class DummyDebugUtils final : public DebugUtils {
		public:
			~DummyDebugUtils() override = default;

			inline void setDebugName(vk::Device, vk::ObjectType, uint64_t, const char*) const override {}

			inline void setDebugTag(vk::Device, vk::ObjectType, uint64_t, uint64_t, const void*, size_t) const override {}

			inline void cmdBeginLabel(vk::CommandBuffer, const char*, glm::vec4 const) const override {}

			inline void cmdEndLabel(vk::CommandBuffer) const override {}

			inline void cmdInsertLabel(vk::CommandBuffer, const char*, glm::vec4 const) const override {}
		};

		class ScopedDebugLabel final {
		public:
			ScopedDebugLabel(const DebugUtils& debug_utils, vk::CommandBuffer command_buffer, std::string const& name, glm::vec4 const color = {});

			ScopedDebugLabel(const CommandBuffer& command_buffer, std::string const& name, glm::vec4 const color = {});

			~ScopedDebugLabel();

		private:
			const DebugUtils* m_debug_utils;
			vk::CommandBuffer m_command_buffer;
		};
	}
}
