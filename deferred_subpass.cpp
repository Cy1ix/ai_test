/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "rendering/subpass/deferred_subpass.h"

#include "common/buffer_pool.h"
#include "rendering/subpass.h"
#include "rendering/render_context.h"
#include "components/camera/camera.h"
#include "scene/scene.h"

namespace frame {
	namespace rendering {
		namespace subpass {
			DeferredSubpass::DeferredSubpass(
				RenderContext& render_context,
				core::ShaderSource&& vertex_shader,
				core::ShaderSource&& fragment_shader,
				scene::Camera& cam,
				scene::Scene& scene_) :
				Subpass{ render_context, std::move(vertex_shader), std::move(fragment_shader) },
				m_camera{ cam },
				m_scene{ scene_ }
			{
			}

			void DeferredSubpass::prepare() {

				m_lighting_variant.addDefinitions({ "MAX_LIGHT_COUNT " + std::to_string(MAX_DEFERRED_LIGHT_COUNT) });
				m_lighting_variant.addDefinitions(light_type_definitions);

				auto& resource_cache = getRenderContext().getDevice().getResourceCache();
				resource_cache.requestShaderModule(vk::ShaderStageFlagBits::eVertex, getVertexShader(), m_lighting_variant);
				resource_cache.requestShaderModule(vk::ShaderStageFlagBits::eFragment, getFragmentShader(), m_lighting_variant);
			}

			void DeferredSubpass::draw(core::CommandBuffer& command_buffer) {

				allocateLights<DeferredLights>(m_scene.getComponents<scene::Light>(), MAX_DEFERRED_LIGHT_COUNT);
				command_buffer.bindLighting(getLightingState(), 0, 4);

				auto& resource_cache = command_buffer.getDevice().getResourceCache();
				auto& vert_shader_module = resource_cache.requestShaderModule(vk::ShaderStageFlagBits::eVertex, getVertexShader(), m_lighting_variant);
				auto& frag_shader_module = resource_cache.requestShaderModule(vk::ShaderStageFlagBits::eFragment, getFragmentShader(), m_lighting_variant);

				std::vector<core::ShaderModuleCPP*> shader_modules{ &vert_shader_module, &frag_shader_module };

				auto& pipeline_layout = resource_cache.requestPipelineLayout(shader_modules);
				command_buffer.bindPipelineLayout(pipeline_layout);

				assert(pipeline_layout.getResources(core::ShaderResourceType::Input, vk::ShaderStageFlagBits::eVertex).empty());
				command_buffer.setVertexInputState({});

				auto& render_target = getRenderContext().getActiveFrame().getRenderTarget();
				auto& target_views = render_target.getViews();
				assert(3 < target_views.size());
				
				command_buffer.bindInput(render_target.getDepthView(), 0, 0, 0);
				
				command_buffer.bindInput(render_target.getAlbedoView(), 0, 1, 0);
				
				command_buffer.bindInput(render_target.getNormalView(), 0, 2, 0);
				
				command_buffer.bindInput(render_target.getMaterialView(), 0, 3, 0);
				
				command_buffer.bindInput(render_target.getPositionView(), 0, 4, 0);
				
				command_buffer.bindInput(render_target.getEmissiveView(), 0, 5, 0);

				RasterizationState rasterization_state;
				rasterization_state.cull_mode = vk::CullModeFlagBits::eFront;
				command_buffer.setRasterizationState(rasterization_state);

				LightUniform light_uniform;

				light_uniform.m_inv_resolution.x = 1.0f / render_target.getExtent().width;
				light_uniform.m_inv_resolution.y = 1.0f / render_target.getExtent().height;

				light_uniform.m_inv_view_proj = glm::inverse(vulkanStyleProjection(m_camera.getProjection()) * m_camera.getView());

				light_uniform.camera_position = glm::vec3(glm::inverse(m_camera.getView())[3]);

				auto& render_frame = getRenderContext().getActiveFrame();
				auto allocation = render_frame.allocateBuffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(LightUniform));
				allocation.update(light_uniform);
				command_buffer.bindBuffer(allocation.getBuffer(), allocation.getOffset(), allocation.getSize(), 0, 3, 0);

				command_buffer.draw(3, 1, 0, 0);
			}
		}
	}
}
