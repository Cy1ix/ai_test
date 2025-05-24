/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include "scene/node.h"
#include "scene/component.h"

namespace frame {
	namespace scene {

        Node::Node(const size_t id, const std::string& name) :
            m_id{ id },
            m_name{ name },
            m_transform{ *this }
		{
        	setComponent(m_transform);
        }

        const size_t Node::getId() const {
            return m_id;
        }

        const std::string& Node::getName() const {
            return m_name;
        }

        Transform& Node::getTransform() {
            return m_transform;
        }

        void Node::setParent(Node& p) {
            m_parent = &p;
            m_transform.invalidateWorldMatrix();
        }

        Node* Node::getParent() const {
            return m_parent;
        }

        void Node::addChild(Node& child) {
            m_children.push_back(&child);
        }

        const std::vector<Node*>& Node::getChildren() const {
            return m_children;
        }

        void Node::setComponent(Component& component) {
            auto it = m_components.find(component.getType());
            if (it != m_components.end()) {
                it->second = &component;
            }
            else {
                m_components.insert(std::make_pair(component.getType(), &component));
            }
        }

        Component& Node::getComponent(const std::type_index index) {
            return *m_components.at(index);
        }

        bool Node::hasComponent(const std::type_index index) {
            return m_components.count(index) > 0;
        }
	}
}