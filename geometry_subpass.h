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

#pragma once

#include "global_common.h"
#include "rendering/subpass.h"

namespace frame {
	namespace scene {
		class Scene;
		class Node;
		class Mesh;
		class SubMesh;
		class Camera;
	}

	namespace rendering {
		struct alignas(16) GlobalUniform {
			glm::mat4 model;
			glm::mat4 camera_view_proj;
			glm::vec3 camera_position;
			glm::mat4 normal_matrix;
		};

		struct PBRMaterialUniform {
			glm::vec4 color;
			float metallic;
			float roughness;
		};

		namespace subpass {
			class GeometrySubpass : public Subpass {
			public:
				GeometrySubpass(RenderContext& render_context, core::ShaderSource&& vertex_shader, core::ShaderSource&& fragment_shader, scene::Scene& scene, scene::Camera& camera);

				virtual ~GeometrySubpass() = default;

				virtual void prepare() override;

				virtual void draw(core::CommandBuffer& command_buffer) override;

				void setThreadIndex(uint32_t index);

			protected:
				virtual void updateUniform(core::CommandBuffer& command_buffer, scene::Node& node, size_t thread_index);

				void drawSubmesh(core::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh, vk::FrontFace front_face = vk::FrontFace::eCounterClockwise);

				virtual void preparePipelineState(core::CommandBuffer& command_buffer, vk::FrontFace front_face, bool double_sided_material);

				virtual core::PipelineLayoutCPP& preparePipelineLayout(core::CommandBuffer& command_buffer, const std::vector<core::ShaderModuleCPP*>& shader_modules);

				virtual void preparePushConstants(core::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh);

				virtual void drawSubmeshCommand(core::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh);

				void getSortedNodes(std::multimap<float, std::pair<scene::Node*, scene::SubMesh*>>& opaque_nodes,
					std::multimap<float, std::pair<scene::Node*, scene::SubMesh*>>& transparent_nodes);

				scene::Camera& m_camera;
				std::vector<scene::Mesh*> m_meshes;
				scene::Scene& m_scene;
				uint32_t m_thread_index{ 0 };
				RasterizationState m_base_rasterization_state{};
			};
		}
	}
}