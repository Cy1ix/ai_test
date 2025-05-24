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

#include "common/buffer_pool.h"
#include "rendering/subpass.h"
#include "global_common.h"

#define MAX_DEFERRED_LIGHT_COUNT 48

namespace frame {
	namespace scene {
		class Camera;
		class Light;
		class Scene;
	}

	namespace rendering {

		struct alignas(16) LightUniform {
			glm::mat4 m_inv_view_proj;
			glm::vec2 m_inv_resolution;
			glm::vec3 camera_position;
			float padding;
		};

		struct alignas(16) DeferredLights {
			Light directional_lights[MAX_DEFERRED_LIGHT_COUNT];
			Light point_lights[MAX_DEFERRED_LIGHT_COUNT];
			Light spot_lights[MAX_DEFERRED_LIGHT_COUNT];
		};

		namespace subpass {
			class DeferredSubpass : public Subpass {
			public:
				DeferredSubpass(RenderContext& render_context, core::ShaderSource&& vertex_shader, core::ShaderSource&& fragment_shader, scene::Camera& camera, scene::Scene& scene);

				virtual void prepare() override;

				void draw(core::CommandBuffer& command_buffer) override;

			private:
				scene::Camera& m_camera;
				scene::Scene& m_scene;
				core::ShaderVariant m_lighting_variant;
			};
		}
	}
}