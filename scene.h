/* Copyright (c) 2018-2024, Arm Limited and Contributors
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

#include <algorithm>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "scene/components/camera/camera.h"
#include "scene/components/mesh/mesh.h"
#include "scene/components/light.h"
#include "scene/components/texture.h"
#include "scene/scripts/script.h"
#include "scene/scripts/animation.h"

namespace frame {
	namespace scene {
		class Node;
		class Component;
		class SubMesh;
		class Mesh;

		class Scene {
		public:
			Scene() = default;
			Scene(const std::string& name);

			void setName(const std::string& name);
			const std::string& getName() const;

			void setNodes(std::vector<std::unique_ptr<Node>>&& nodes);
			std::unique_ptr<Node>& createNode(const size_t id, const std::string& name);
			void addNode(std::unique_ptr<Node>&& node);
			void addChild(Node& child);

			std::unique_ptr<Component> getModel(uint32_t index = 0);

			void addComponent(std::unique_ptr<Component>&& component);
			void addComponent(std::unique_ptr<Component>&& component, Node& node);

			void setComponents(const std::type_index& type_info, std::vector<std::unique_ptr<Component>>&& components);

			template <class T>
			void setComponents(std::vector<std::unique_ptr<T>>&& components)
			{
				std::vector<std::unique_ptr<Component>> result(components.size());
				std::transform(components.begin(), components.end(), result.begin(),
					[](std::unique_ptr<T>& component) -> std::unique_ptr<Component> {
						return std::unique_ptr<Component>(std::move(component));
					});
				setComponents(typeid(T), std::move(result));
			}

			template <class T>
			void clearComponents() {
				setComponents(typeid(T), {});
			}

			template <class T>
			std::vector<T*> getComponents() const {
				std::vector<T*> result;
				if (hasComponent(typeid(T))) {
					auto& scene_components = getComponents(typeid(T));

					result.resize(scene_components.size());
					std::transform(scene_components.begin(), scene_components.end(), result.begin(),
						[](const std::unique_ptr<Component>& component) -> T* { return dynamic_cast<T*>(component.get()); });
				}
				return result;

			}
			
			const std::vector<std::unique_ptr<Component>>& getComponents(const std::type_index& type_info) const;

			template <class T>
			bool hasComponent() const {
				return hasComponent(typeid(T));
			}

			bool hasComponent(const std::type_index& type_info) const;

			Node* findNode(const std::string& name);

			void setRootNode(Node& node);
			Node& getRootNode();

		private:
			std::string m_name;
			std::vector<std::unique_ptr<Node>> m_nodes;
			Node* m_root{ nullptr };
			std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> m_components;
		};
	}
}
