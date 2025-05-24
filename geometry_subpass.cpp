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

#include "rendering/subpass/geometry_subpass.h"
#include "common/common.h"
#include "rendering/render_context.h"
#include "scene/components/camera/camera.h"
#include "scene/components/image/image.h"
#include "scene/components/material/material.h"
#include "scene/components/mesh/mesh.h"
#include "scene/components/material/pbr_material.h"
#include "scene/components/texture.h"
#include "scene/node.h"
#include "scene/scene.h"

namespace frame {
	namespace rendering {
		namespace subpass {
			GeometrySubpass::GeometrySubpass(RenderContext& render_context, core::ShaderSource&& vertex_source, core::ShaderSource&& fragment_source, scene::Scene& scene_, scene::Camera& camera) :
				Subpass{ render_context, std::move(vertex_source), std::move(fragment_source) },
				m_meshes{ scene_.getComponents<scene::Mesh>() },
				m_camera{ camera },
				m_scene{ scene_ }
			{
			}

			void GeometrySubpass::prepare() {
				auto& device = getRenderContext().getDevice();
				for (auto& mesh : m_meshes) {
					for (auto& sub_mesh : mesh->getSubmeshes()) {
						auto& variant = sub_mesh->getShaderVariant();
						auto& vert_module = device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eVertex, getVertexShader(), variant);
						auto& frag_module = device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eFragment, getFragmentShader(), variant);
					}
				}
			}

			void GeometrySubpass::getSortedNodes(
				std::multimap<float, std::pair<scene::Node*, scene::SubMesh*>>& opaque_nodes,
				std::multimap<float, std::pair<scene::Node*, scene::SubMesh*>>& transparent_nodes)
			{
				auto camera_transform = m_camera.getNode()->getTransform().getWorldMatrix();

				for (auto& mesh : m_meshes) {
					for (auto& node : mesh->getNodes()) {

						auto node_transform = node->getTransform().getWorldMatrix();

						const scene::AABB& mesh_bounds = mesh->getBounds();

						scene::AABB world_bounds{ mesh_bounds.getMin(), mesh_bounds.getMax() };
						world_bounds.transform(node_transform);

						float distance = glm::length(glm::vec3(camera_transform[3]) - world_bounds.getCenter());

						for (auto& sub_mesh : mesh->getSubmeshes())
						{
							if (sub_mesh->getMaterial()->m_alpha_mode == scene::AlphaMode::Blend) {
								transparent_nodes.emplace(distance, std::make_pair(node, sub_mesh));
							}
							else {
								opaque_nodes.emplace(distance, std::make_pair(node, sub_mesh));
							}
						}
					}
				}
			}

			void GeometrySubpass::draw(core::CommandBuffer& command_buffer) {

				std::multimap<float, std::pair<scene::Node*, scene::SubMesh*>> opaque_nodes;
				std::multimap<float, std::pair<scene::Node*, scene::SubMesh*>> transparent_nodes;

				getSortedNodes(opaque_nodes, transparent_nodes);

				{
					core::ScopedDebugLabel opaque_debug_label{ command_buffer, "Opaque objects" };

					for (auto node_it = opaque_nodes.begin(); node_it != opaque_nodes.end(); node_it++) {

						updateUniform(command_buffer, *node_it->second.first, m_thread_index);

						const auto& scale = node_it->second.first->getTransform().getScale();
						bool flipped = scale.x * scale.y * scale.z < 0;
						vk::FrontFace front_face = flipped ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise;

						drawSubmesh(command_buffer, *node_it->second.second, front_face);
					}
				}

				ColorBlendAttachmentState color_blend_attachment{};
				color_blend_attachment.blend_enable = true;
				color_blend_attachment.src_color_blend_factor = vk::BlendFactor::eSrcAlpha;
				color_blend_attachment.dst_color_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;
				color_blend_attachment.src_alpha_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;

				ColorBlendState color_blend_state{};
				color_blend_state.attachments.resize(getOutputAttachments().size());
				for (auto& it : color_blend_state.attachments)
				{
					it = color_blend_attachment;
				}
				command_buffer.setColorBlendState(color_blend_state);

				command_buffer.setDepthStencilState(getDepthStencilState());

				{
					core::ScopedDebugLabel transparent_debug_label{ command_buffer, "Transparent objects" };

					for (auto node_it = transparent_nodes.rbegin(); node_it != transparent_nodes.rend(); node_it++) {
						updateUniform(command_buffer, *node_it->second.first, m_thread_index);
						drawSubmesh(command_buffer, *node_it->second.second);
					}
				}
			}

			void GeometrySubpass::updateUniform(core::CommandBuffer& command_buffer, scene::Node& node, size_t thread_index) {

				GlobalUniform global_uniform;

				global_uniform.camera_view_proj = m_camera.getPreRotation() * vulkanStyleProjection(m_camera.getProjection()) * m_camera.getView();

				auto& render_frame = getRenderContext().getActiveFrame();

				auto& transform = node.getTransform();

				auto allocation = render_frame.allocateBuffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(GlobalUniform), thread_index);

				global_uniform.model = transform.getWorldMatrix();

				global_uniform.camera_position = glm::vec3(glm::inverse(m_camera.getView())[3]);

				global_uniform.normal_matrix = glm::transpose(glm::inverse(glm::mat3(global_uniform.model)));

				allocation.update(global_uniform);

				command_buffer.bindBuffer(allocation.getBuffer(), allocation.getOffset(), allocation.getSize(), 0, 0, 0);
			}

			std::string ShaderStageFlagsToString(const vk::ShaderStageFlags& flags) {
				std::string result;
				
				if (flags & vk::ShaderStageFlagBits::eVertex) {
					result += "VERTEX | ";
				}
				if (flags & vk::ShaderStageFlagBits::eTessellationControl) {
					result += "TESSELLATION_CONTROL | ";
				}
				if (flags & vk::ShaderStageFlagBits::eTessellationEvaluation) {
					result += "TESSELLATION_EVALUATION | ";
				}
				if (flags & vk::ShaderStageFlagBits::eGeometry) {
					result += "GEOMETRY | ";
				}
				if (flags & vk::ShaderStageFlagBits::eFragment) {
					result += "FRAGMENT | ";
				}
				if (flags & vk::ShaderStageFlagBits::eCompute) {
					result += "COMPUTE | ";
				}
				if (flags & vk::ShaderStageFlagBits::eRaygenKHR) {
					result += "RAYGEN | ";
				}

				if (!result.empty()) {
					result.erase(result.size() - 3);
				}
				else {
					result = "NONE";
				}
				
				if (flags & ~(vk::ShaderStageFlagBits::eAllGraphics |
					vk::ShaderStageFlagBits::eAll)) {
					result += " | UNKNOWN(0x" +
						std::to_string(static_cast<uint32_t>(flags)) + ")";
				}

				return result;
			}

			void GeometrySubpass::drawSubmesh(core::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh, vk::FrontFace front_face) {

				auto& device = command_buffer.getDevice();

				core::ScopedDebugLabel submesh_debug_label{ command_buffer, sub_mesh.getName().c_str() };

				preparePipelineState(command_buffer, front_face, sub_mesh.getMaterial()->m_double_sided);

				MultisampleState multisample_state{};
				multisample_state.rasterization_samples = getSampleCount();
				command_buffer.setMultisampleState(multisample_state);

				auto& vert_shader_module = device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eVertex, getVertexShader(), sub_mesh.getShaderVariant());
				auto& frag_shader_module = device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eFragment, getFragmentShader(), sub_mesh.getShaderVariant());

				std::vector<core::ShaderModuleCPP*> shader_modules{ &vert_shader_module, &frag_shader_module };

				auto& pipeline_layout = preparePipelineLayout(command_buffer, shader_modules);

				command_buffer.bindPipelineLayout(pipeline_layout);

				if (pipeline_layout.getPushConstantRangeStage(sizeof(PBRMaterialUniform)) != vk::ShaderStageFlags{}) {
					preparePushConstants(command_buffer, sub_mesh);
				}

				const auto& shader_sets = pipeline_layout.getShaderSets();
				for (const auto& [set_index, resources] : shader_sets) {
					if (pipeline_layout.hasDescriptorSetLayout(set_index)) {
						core::DescriptorSetLayoutCPP& descriptor_set_layout = pipeline_layout.getDescriptorSetLayout(set_index);

						/*
						for(auto flag : descriptor_set_layout.getBindings()) {
							LOGI("{}", ShaderStageFlagsToString(flag.stageFlags));
						}
						*/
						for (auto& texture : sub_mesh.getMaterial()->m_textures) {
							if (auto layout_binding = descriptor_set_layout.getLayoutBinding(texture.first)) {
								command_buffer.bindImage(texture.second->getImage()->getImageView(),
									texture.second->getSampler()->m_sampler,
									set_index, layout_binding->binding, 0);
							}
						}
					}
				}
				
				auto vertex_input_resources = pipeline_layout.getResources(core::ShaderResourceType::Input, vk::ShaderStageFlagBits::eVertex);

				VertexInputState vertex_input_state;
				std::set<uint32_t> added_bindings;
				
				for (auto& input_resource : vertex_input_resources) {
					scene::VertexAttribute attribute;

					if (!sub_mesh.getAttribute(input_resource.name, attribute)) {
						continue;
					}

					vk::VertexInputAttributeDescription vertex_attribute{};
					vertex_attribute.binding = input_resource.location;
					vertex_attribute.format = attribute.format;
					vertex_attribute.location = input_resource.location;
					vertex_attribute.offset = attribute.offset;

					vertex_input_state.attributes.push_back(vertex_attribute);

					vk::VertexInputBindingDescription vertex_binding{};
					vertex_binding.binding = input_resource.location;
					vertex_binding.stride = attribute.stride;

					vertex_input_state.bindings.push_back(vertex_binding);
				}

				command_buffer.setVertexInputState(vertex_input_state);
				
				for (auto& input_resource : vertex_input_resources) {
					const auto& buffer_iter = sub_mesh.m_vertex_buffers.find(input_resource.name);
					
					if (buffer_iter != sub_mesh.m_vertex_buffers.end()) {
						std::vector<std::reference_wrapper<const common::Buffer>> buffers;
						buffers.emplace_back(std::ref(buffer_iter->second));
						
						command_buffer.bindVertexBuffers(input_resource.location, std::move(buffers), { 0 });
					}
				}

				drawSubmeshCommand(command_buffer, sub_mesh);
			}

			void GeometrySubpass::preparePipelineState(core::CommandBuffer& command_buffer, vk::FrontFace front_face, bool double_sided_material) {
				RasterizationState rasterization_state = m_base_rasterization_state;
				rasterization_state.front_face = front_face;

				if (double_sided_material)
				{
					rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
				}

				command_buffer.setRasterizationState(rasterization_state);

				MultisampleState multisample_state{};
				multisample_state.rasterization_samples = getSampleCount();
				command_buffer.setMultisampleState(multisample_state);
			}

			core::PipelineLayoutCPP& GeometrySubpass::preparePipelineLayout(core::CommandBuffer& command_buffer, const std::vector<core::ShaderModuleCPP*>& shader_modules) {

				for (auto& shader_module : shader_modules) {
					for (auto& resource_mode : getResourceModeMap()) {
						shader_module->setResourceMode(resource_mode.first, resource_mode.second);
					}
				}

				return command_buffer.getDevice().getResourceCache().requestPipelineLayout(shader_modules);
			}

			void GeometrySubpass::preparePushConstants(core::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh) {
				auto pbr_material = dynamic_cast<const scene::PBRMaterial*>(sub_mesh.getMaterial());

				PBRMaterialUniform pbr_material_uniform{};
				pbr_material_uniform.color = pbr_material->m_color;
				pbr_material_uniform.metallic = pbr_material->m_metallic;
				pbr_material_uniform.roughness = pbr_material->m_roughness;

				auto data = common::toBytes(pbr_material_uniform);

				if (!data.empty()) {
					command_buffer.pushConstants(data);
				}
			}

			void GeometrySubpass::drawSubmeshCommand(core::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh) {

				if (sub_mesh.m_vertex_indices != 0) {
					command_buffer.bindIndexBuffer(*sub_mesh.m_index_buffer, sub_mesh.m_index_offset, sub_mesh.m_index_type);

					command_buffer.drawIndexed(sub_mesh.m_vertex_indices, 1, 0, 0, 0);
				}
				else {
					command_buffer.draw(sub_mesh.m_vertices_count, 1, 0, 0);
				}
			}

			void GeometrySubpass::setThreadIndex(uint32_t index) {
				m_thread_index = index;
			}
		}
	}
}
