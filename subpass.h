/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/render_context.h"
#include "common/buffer_pool.h"
#include "scene/components/light.h"
#include "scene/node.h"
#include "common/strings.h"

namespace frame {
	namespace common {
		class BufferAllocation;
	}

	namespace core {
		class ShaderSource;
		class CommandBuffer;
	}

	namespace rendering {
		class RenderContext;

		struct alignas(16) Light {
			glm::vec4 position;
			glm::vec4 color;
			glm::vec4 direction;
			glm::vec2 info;
		};

		struct LightingState {
			std::vector<Light> directional_lights;
			std::vector<Light> point_lights;
			std::vector<Light> spot_lights;
			common::BufferAllocation light_buffer;
		};
		
		glm::mat4 vulkanStyleProjection(const glm::mat4& proj);

		inline const std::vector<std::string> light_type_definitions = {
			"DIRECTIONAL_LIGHT " + std::to_string(static_cast<float>(scene::LightType::Directional)),
			"POINT_LIGHT " + std::to_string(static_cast<float>(scene::LightType::Point)),
			"SPOT_LIGHT " + std::to_string(static_cast<float>(scene::LightType::Spot)) };
		
		class Subpass {
		public:
			Subpass(RenderContext& render_context, core::ShaderSource&& vertex_shader, core::ShaderSource&& fragment_shader);

			Subpass(const Subpass&) = delete;
			Subpass(Subpass&&) = default;
			virtual ~Subpass() = default;
			Subpass& operator=(const Subpass&) = delete;
			Subpass& operator=(Subpass&&) = delete;
			
			virtual void draw(core::CommandBuffer& command_buffer) = 0;
			
			virtual void prepare() = 0;
			
			template <typename T>
			void allocateLights(const std::vector<scene::Light*>& scene_lights, size_t max_lights_per_type);

			const std::vector<uint32_t>& getColorResolveAttachments() const;
			const std::string& getDebugName() const;
			const uint32_t& getDepthStencilResolveAttachment() const;
			vk::ResolveModeFlagBits getDepthStencilResolveMode() const;
			DepthStencilState& getDepthStencilState();
			const bool& getDisableDepthStencilAttachment() const;
			const core::ShaderSource& getFragmentShader() const;
			const std::vector<uint32_t>& getInputAttachments() const;
			LightingState& getLightingState();
			const std::vector<uint32_t>& getOutputAttachments() const;
			RenderContext& getRenderContext();
			std::unordered_map<std::string, core::ShaderResourceMode> const& getResourceModeMap() const;
			vk::SampleCountFlagBits getSampleCount() const;
			const core::ShaderSource& getVertexShader() const;

			void setColorResolveAttachments(std::vector<uint32_t> const& color_resolve);
			void setDebugName(const std::string& name);
			void setDisableDepthStencilAttachment(bool disable_depth_stencil);
			void setDepthStencilResolveAttachment(uint32_t depth_stencil_resolve);
			void setDepthStencilResolveMode(vk::ResolveModeFlagBits mode);
			void setInputAttachments(std::vector<uint32_t> const& input);
			void setOutputAttachments(std::vector<uint32_t> const& output);
			void setSampleCount(vk::SampleCountFlagBits sample_count);
			
			void updateRenderTargetAttachments(RenderTarget& render_target);

		private:
			std::vector<uint32_t> m_color_resolve_attachments = {};

			std::string m_debug_name{};
			
			vk::ResolveModeFlagBits m_depth_stencil_resolve_mode{ vk::ResolveModeFlagBits::eNone };

			DepthStencilState m_depth_stencil_state{};
			
			bool m_disable_depth_stencil_attachment{ false };
			
			uint32_t m_depth_stencil_resolve_attachment{ VK_ATTACHMENT_UNUSED };
			
			LightingState m_lighting_state{};

			core::ShaderSource m_fragment_shader;
			
			std::vector<uint32_t> m_input_attachments = {};
			
			std::vector<uint32_t> m_output_attachments = { 0 };

			RenderContext& m_render_context;
			
			std::unordered_map<std::string, core::ShaderResourceMode> m_resource_mode_map;

			vk::SampleCountFlagBits m_sample_count{ vk::SampleCountFlagBits::e1 };
			core::ShaderSource m_vertex_shader;
		};

		template <typename T>
		void Subpass::allocateLights(const std::vector<scene::Light*>& scene_lights, size_t max_lights_per_type) {
			m_lighting_state.directional_lights.clear();
			m_lighting_state.point_lights.clear();
			m_lighting_state.spot_lights.clear();

			for (auto& scene_light : scene_lights) {
				const auto& properties = scene_light->getProperties();
				auto& transform = scene_light->getNode()->getTransform();

				Light light{ {transform.getTranslation(), static_cast<float>(scene_light->getLightType())},
							{properties.color, properties.intensity},
							{transform.getRotation() * properties.direction, properties.range},
							{properties.inner_cone_angle, properties.outer_cone_angle} };

				switch (scene_light->getLightType()) {
				case scene::LightType::Directional: {
					if (m_lighting_state.directional_lights.size() < max_lights_per_type) {
						m_lighting_state.directional_lights.push_back(light);
					}
					else {
						LOGE("Exceeding max_lights_per_type of {} for directional lights", max_lights_per_type);
					}
					break;
				}
				case scene::LightType::Point: {
					if (m_lighting_state.point_lights.size() < max_lights_per_type) {
						m_lighting_state.point_lights.push_back(light);
					}
					else {
						LOGE("Exceeding max_lights_per_type of {} for point lights", max_lights_per_type);
					}
					break;
				}
				case scene::LightType::Spot: {
					if (m_lighting_state.spot_lights.size() < max_lights_per_type) {
						m_lighting_state.spot_lights.push_back(light);
					}
					else {
						LOGE("Exceeding max_lights_per_type of {} for spot lights", max_lights_per_type);
					}
					break;
				}
				default:
					LOGE("Encountered unknown light type {}", common::toString(scene_light->getLightType()));
					break;
				}
			}

			T light_info;

			std::copy(m_lighting_state.directional_lights.begin(), m_lighting_state.directional_lights.end(), light_info.directional_lights);
			std::copy(m_lighting_state.point_lights.begin(), m_lighting_state.point_lights.end(), light_info.point_lights);
			std::copy(m_lighting_state.spot_lights.begin(), m_lighting_state.spot_lights.end(), light_info.spot_lights);

			auto& render_frame = m_render_context.getActiveFrame();
			m_lighting_state.light_buffer = render_frame.allocateBuffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(T));
			m_lighting_state.light_buffer.update(light_info);
		}
	}
}