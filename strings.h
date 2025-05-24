/* Copyright (c) 2018-2024, Arm Limited and Contributors
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

#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace frame {
	namespace core {
		enum class ShaderResourceType;
	}
	namespace scene {
		enum class AlphaMode;
	}

	namespace common {
		std::vector<std::string> split(const std::string& str, const std::string& delimiter);
		std::string join(const std::vector<std::string>& str, const std::string& separator);

		template <class T>
		std::string toString(const T& value) {
			std::stringstream ss;
			ss << std::fixed << value;
			return ss.str();
		}

		const std::string toString(vk::Format format);
		const std::string toString(vk::PresentModeKHR present_mode);
		const std::string toString(vk::Result result);
		const std::string toString(vk::PhysicalDeviceType type);
		const std::string toString(vk::SurfaceTransformFlagBitsKHR transform_flag);
		const std::string toString(vk::SurfaceFormatKHR surface_format);
		const std::string toString(vk::CompositeAlphaFlagBitsKHR composite_alpha);
		const std::string toString(vk::ImageUsageFlagBits image_usage);
		const std::string toString(vk::SampleCountFlagBits flags);
		const std::string toString(vk::ImageTiling tiling);
		const std::string toString(vk::ImageType type);
		const std::string toString(vk::BlendFactor blend);
		const std::string toString(vk::VertexInputRate rate);
		const std::string toString(vk::Bool32 state);
		const std::string toString(vk::PrimitiveTopology topology);
		const std::string toString(vk::FrontFace face);
		const std::string toString(vk::PolygonMode mode);
		const std::string toString(vk::CompareOp operation);
		const std::string toString(vk::StencilOp operation);
		const std::string toString(vk::LogicOp operation);
		const std::string toString(vk::BlendOp operation);
		const std::string toString(scene::AlphaMode mode);
		const std::string toString(bool flag);
		const std::string toString(core::ShaderResourceType type);
		const std::string toString(vk::Extent2D extent);

		template <typename T>
		inline const std::string toString(uint32_t bitmask, const std::map<T, const char*> string_map) {
			std::stringstream result;
			bool append = false;
			for (const auto& s : string_map) {
				if (bitmask & static_cast<uint32_t>(s.first)) {
					if (append) {
						result << " | ";
					}
					result << s.second;
					append = true;
				}
			}
			return result.str();
		}

		const std::string bufferUsageToString(vk::BufferUsageFlags bitmask);
		const std::string shaderStageToString(vk::ShaderStageFlags bitmask);
		const std::string imageUsageToString(vk::ImageUsageFlags bitmask);
		const std::string imageAspectToString(vk::ImageAspectFlags bitmask);
		const std::string cullModeToString(vk::CullModeFlags bitmask);
		const std::string colorComponentToString(vk::ColorComponentFlags bitmask);
		const std::string imageCompressionFlagsToString(vk::ImageCompressionFlagsEXT flags);
		const std::string imageCompressionFixedRateFlagsToString(vk::ImageCompressionFixedRateFlagsEXT flags);
		std::vector<std::string> split(const std::string& input, char delim);
	}
}
