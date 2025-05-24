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

#include "scene/scene.h"

#include <queue>

#include "component.h"
#include "scene/components/mesh/sub_mesh.h"
#include "node.h"

namespace frame {
	namespace scene {
		Scene::Scene(const std::string& name) :
			m_name{ name }
		{
		}

		void Scene::setName(const std::string& name) {
			m_name = name;
		}

		const std::string& Scene::getName() const {
			return m_name;
		}

		void Scene::setNodes(std::vector<std::unique_ptr<Node>>&& nodes) {
			assert(m_nodes.empty() && "Scene nodes were already set");
			m_nodes = std::move(nodes);
		}

		std::unique_ptr<Node>& Scene::createNode(const size_t id, const std::string& name) {
			m_nodes.emplace_back(std::make_unique<Node>(id, name));
			return *std::find_if(m_nodes.rbegin(), m_nodes.rend(), [&name](const auto& node) {
				return node->getName() == name;
				});
		}

		void Scene::addNode(std::unique_ptr<Node>&& node) {
			m_nodes.emplace_back(std::move(node));
		}

		void Scene::addChild(Node& child) {
			m_root->addChild(child);
		}

		std::unique_ptr<Component> Scene::getModel(uint32_t index) {
			auto meshes = std::move(m_components.at(typeid(SubMesh)));

			assert(index < meshes.size());
			return std::move(meshes[index]);
		}

		void Scene::addComponent(std::unique_ptr<Component>&& component, Node& node) {
			node.setComponent(*component);

			if (component) {
				m_components[component->getType()].push_back(std::move(component));
			}
		}

		void Scene::addComponent(std::unique_ptr<Component>&& component) {
			if (component) {
				m_components[component->getType()].push_back(std::move(component));
			}
		}

		void Scene::setComponents(const std::type_index& type_info, std::vector<std::unique_ptr<Component>>&& components) {
			m_components[type_info] = std::move(components);
		}

		const std::vector<std::unique_ptr<Component>>& Scene::getComponents(const std::type_index& type_info) const {
			return m_components.at(type_info);
		}

		bool Scene::hasComponent(const std::type_index& type_info) const {
			auto component = m_components.find(type_info);
			return (component != m_components.end() && !component->second.empty());
		}

		Node* Scene::findNode(const std::string& node_name) {
			for (auto root_node : m_root->getChildren()) {
				std::queue<Node*> traverse_nodes{};
				traverse_nodes.push(root_node);

				while (!traverse_nodes.empty()) {
					auto node = traverse_nodes.front();
					traverse_nodes.pop();

					if (node->getName() == node_name) {
						return node;
					}

					for (auto child_node : node->getChildren()) {
						traverse_nodes.push(child_node);
					}
				}
			}

			return nullptr;
		}

		void Scene::setRootNode(Node& node) {
			m_root = &node;
		}

		Node& Scene::getRootNode() {
			return *m_root;
		}
	}
}
