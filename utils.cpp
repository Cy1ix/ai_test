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

#include "scene/utils.h"

#include <algorithm>
#include <cctype>
#include <queue>
#include <stdexcept>

#include "core/queue.h"
#include "scene/components/light.h"
#include "scene/components/material/material.h"
#include "scene/components/camera/perspective_camera.h"
#include "scene/components/mesh/sub_mesh.h"
#include "scene/node.h"
#include "scene/scripts/script.h"
#include "scene/components/camera/free_camera.h"

namespace frame {
	namespace scene {
		std::string getExtension(const std::string& uri) {
			auto dot_pos = uri.find_last_of('.');
			if (dot_pos == std::string::npos) {
				throw std::runtime_error{ "Uri has no extension" };
			}
			return uri.substr(dot_pos + 1);
		}

		void screenShot(rendering::RenderContext& render_context, const std::string& filename) {

			assert(render_context.getFormat() == vk::Format::eR8G8B8A8Unorm ||
				render_context.getFormat() == vk::Format::eB8G8R8A8Unorm ||
				render_context.getFormat() == vk::Format::eR8G8B8A8Srgb ||
				render_context.getFormat() == vk::Format::eB8G8R8A8Srgb);
			
			auto& frame = render_context.getLastRenderedFrame();
			assert(!frame.getRenderTarget().getViews().empty());
			auto& src_image_view = frame.getRenderTarget().getViews()[0];
			
			auto width = render_context.getSurfaceExtent().width;
			auto height = render_context.getSurfaceExtent().height;
			auto dst_size = width * height * 4;

			common::Buffer dst_buffer{ render_context.getDevice(),
										 dst_size,
										 vk::BufferUsageFlagBits::eTransferDst,
										 VMA_MEMORY_USAGE_GPU_TO_CPU,
										 VMA_ALLOCATION_CREATE_MAPPED_BIT };
			
			const auto& queue = render_context.getDevice().getQueueByFlags(vk::QueueFlagBits::eGraphics, 0);
			auto& cmd_buf = render_context.getDevice().getCommandPool().requestCommandBuffer();
			cmd_buf.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			
			{
				common::BufferMemoryBarrier buffer_barrier{};
				buffer_barrier.m_src_access_mask = vk::AccessFlags{};
				buffer_barrier.m_dst_access_mask = vk::AccessFlagBits::eTransferWrite;
				buffer_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eTransfer;
				buffer_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
				cmd_buf.bufferMemoryBarrier(dst_buffer, 0, dst_size, buffer_barrier);
			
				common::ImageMemoryBarrier img_barrier_to_src{};
				img_barrier_to_src.m_old_layout = vk::ImageLayout::ePresentSrcKHR;
				img_barrier_to_src.m_new_layout = vk::ImageLayout::eTransferSrcOptimal;
				img_barrier_to_src.m_src_stage_mask = vk::PipelineStageFlagBits::eTransfer;
				img_barrier_to_src.m_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
				cmd_buf.imageMemoryBarrier(src_image_view, img_barrier_to_src);
			}
				
			auto bgr_formats = { vk::Format::eB8G8R8A8Srgb, vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Snorm };
			bool swizzle = std::find(bgr_formats.begin(), bgr_formats.end(), src_image_view.getFormat()) != bgr_formats.end();
				
			VkBufferImageCopy image_copy_region{};
			image_copy_region.bufferRowLength = width;
			image_copy_region.bufferImageHeight = height;
			image_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_copy_region.imageSubresource.layerCount = 1;
			image_copy_region.imageExtent = { width, height, 1 };
			cmd_buf.copyImageToBuffer(src_image_view.getImage(), vk::ImageLayout::eTransferSrcOptimal, dst_buffer, { image_copy_region });

			{
				common::BufferMemoryBarrier buffer_barrier{};
				buffer_barrier.m_src_access_mask = vk::AccessFlagBits::eTransferWrite;
				buffer_barrier.m_dst_access_mask = vk::AccessFlagBits::eHostRead;
				buffer_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eTransfer;
				buffer_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eHost;
				cmd_buf.bufferMemoryBarrier(dst_buffer, 0, dst_size, buffer_barrier);
			
				common::ImageMemoryBarrier img_barrier_to_present{};
				img_barrier_to_present.m_old_layout = vk::ImageLayout::eTransferSrcOptimal;
				img_barrier_to_present.m_new_layout = vk::ImageLayout::ePresentSrcKHR;
				img_barrier_to_present.m_src_stage_mask = vk::PipelineStageFlagBits::eTransfer;
				img_barrier_to_present.m_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
				cmd_buf.imageMemoryBarrier(src_image_view, img_barrier_to_present);
			}

			cmd_buf.end();
			queue.submit(cmd_buf, frame.requestFence());
			queue.getHandle().waitIdle();
			
			auto raw_data = dst_buffer.map();
			uint8_t* data = raw_data;
			
			for (size_t i = 0; i < height; ++i) {
				for (size_t j = 0; j < width; ++j) {
					if (swizzle) {
						std::swap(data[0], data[2]);
					}
					data[3] = 255;
					data += 4;
				}
			}
			
			filesystem::writeImage(raw_data, filename, width, height, 4, width * 4);
			dst_buffer.unmap();
		}

		std::string toSnakeCase(const std::string& text) {
			std::stringstream result;

			for (const auto ch : text) {
				if (std::isalpha(ch)) {
					if (std::isspace(ch)) {
						result << '_';
					}
					else if (std::isupper(ch)) {
						result << '_' << static_cast<char>(std::tolower(ch));
					}
					else {
						result << static_cast<char>(std::tolower(ch));
					}
				}
				else {
					result << ch;
				}
			}

			return result.str();
		}

		Light& addLight(Scene& scene, LightType type, const glm::vec3& position, const glm::quat& rotation, const LightProperties& props, Node* parent_node) {
			 
			auto light_ptr = std::make_unique<Light>("light");
			auto node = std::make_unique<Node>(-1, "light node");
			
			if (parent_node) {
				node->setParent(*parent_node);
			}
			
			light_ptr->setNode(*node);
			light_ptr->setLightType(type);
			light_ptr->setProperties(props);
			
			auto& transform = node->getTransform();
			transform.setTranslation(position);
			transform.setRotation(rotation);
			
			auto& light = *light_ptr;
			node->setComponent(light);
			scene.addChild(*node);
			scene.addComponent(std::move(light_ptr));
			scene.addNode(std::move(node));

			return light;
		}

		Light& addPointLight(Scene& scene, const glm::vec3& position, const LightProperties& props, Node* parent_node) {
			return addLight(scene, LightType::Point, position, {}, props, parent_node);
		}

		Light& addDirectionalLight(Scene& scene, const glm::quat& rotation, const LightProperties& props, Node* parent_node) {
			return addLight(scene, LightType::Directional, {}, rotation, props, parent_node);
		}

		Light& addSpotLight(Scene& scene, const glm::vec3& position, const glm::quat& rotation, const LightProperties& props, Node* parent_node) {
			return addLight(scene, LightType::Spot, position, rotation, props, parent_node);
		}

		Node& addFreeCamera(Scene& scene, const std::string& node_name, VkExtent2D extent) {

			auto camera_node = scene.findNode(node_name);
			if (!camera_node) {
				LOGW("Camera node `{}` not found. Looking for `default_camera` node.", node_name.c_str());
				camera_node = scene.findNode("default_camera");
			}
			
			if (!camera_node) {
				throw std::runtime_error("Camera node with name `" + node_name + "` not found.");
			}
			if (!camera_node->hasComponent<Camera>()) {
				throw std::runtime_error("No camera component found for `" + node_name + "` node.");
			}
			
			auto free_camera_script = std::make_unique<FreeCamera>(*camera_node);
			free_camera_script->resize(extent.width, extent.height);
			scene.addComponent(std::move(free_camera_script), *camera_node);

			return *camera_node;
		}
	}
}
