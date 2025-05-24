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

#include "global_common.h"
#include "components/light.h"
#include "glm/gtx/quaternion.hpp"
#include "filesystem/filesystem.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_context.h"
#include "scene/components/mesh/sub_mesh.h"
#include "scene/scene.h"

namespace frame {
	namespace core {
		class CommandBuffer;
	}

	namespace scene {
		std::string getExtension(const std::string& uri);

		std::string toSnakeCase(const std::string& name);

		void screenShot(rendering::RenderContext& render_context, const std::string& filename);

		Light& addLight(Scene& scene, LightType type, const glm::vec3& position, const glm::quat& rotation = {}, const LightProperties& props = {}, Node* parent_node = nullptr);

		Light& addPointLight(Scene& scene, const glm::vec3& position, const LightProperties& props = {}, Node* parent_node = nullptr);

		Light& addDirectionalLight(Scene& scene, const glm::quat& rotation, const LightProperties& props = {}, Node* parent_node = nullptr);

		Light& addSpotLight(Scene& scene, const glm::vec3& position, const glm::quat& rotation, const LightProperties& props = {}, Node* parent_node = nullptr);

		Node& addFreeCamera(Scene& scene, const std::string& node_name, VkExtent2D extent);
	}
}
