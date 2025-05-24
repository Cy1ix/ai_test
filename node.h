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

#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "scene/components/transform.h"

namespace frame {
    namespace scene {

        class Node {
        public:
            Node(const size_t id, const std::string& name);
            virtual ~Node() = default;

            const size_t getId() const;
            const std::string& getName() const;

            Transform& getTransform();

            void setParent(Node& parent);
            Node* getParent() const;

            void addChild(Node& child);
            const std::vector<Node*>& getChildren() const;

            void setComponent(Component& component);
            
            template <class T>
            inline T& getComponent() {
                return dynamic_cast<T&>(getComponent(typeid(T)));
            }

            Component& getComponent(const std::type_index index);
            
            template <class T>
            bool hasComponent() {
                return hasComponent(typeid(T));
            }

            bool hasComponent(const std::type_index index);

        private:
            size_t m_id;
            std::string m_name;
            Transform m_transform;
            Node* m_parent{ nullptr };
            std::vector<Node*> m_children;
            std::unordered_map<std::type_index, Component*> m_components;
        };
    }
}
