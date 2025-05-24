/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "camera.h"

#include "scene/components/transform.h"
#include "scene/node.h"

namespace frame {
    namespace scene {
        Camera::Camera(const std::string& name) :
            Component{ name }
        {}

        std::type_index Camera::getType() {
            return typeid(Camera);
        }

        glm::mat4 Camera::getView() {
            if (!m_node) {
                throw std::runtime_error{ "Camera component is not attached to a node" };
            }

            auto& transform = m_node->getComponent<Transform>();
            return glm::inverse(transform.getWorldMatrix());
        }

        void Camera::setNode(Node& node) {
            m_node = &node;
        }

        Node* Camera::getNode() {
            return m_node;
        }

        const glm::mat4 Camera::getPreRotation() {
            return m_pre_rotation;
        }

        void Camera::setPreRotation(const glm::mat4& pre_rotation) {
            m_pre_rotation = pre_rotation;
        }
    }
}
