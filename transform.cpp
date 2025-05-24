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

#include "scene/components/transform.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "scene/node.h"

namespace frame {
	namespace scene {
        Transform::Transform(Node& node) :
            m_node{ node }
        {}

        Node& Transform::getNode() {
            return m_node;
        }

        std::type_index Transform::getType() {
            return typeid(Transform);
        }

        void Transform::setTranslation(const glm::vec3& new_translation) {
            m_translation = new_translation;
            invalidateWorldMatrix();
        }

        void Transform::setRotation(const glm::quat& new_rotation) {
            m_rotation = new_rotation;
            invalidateWorldMatrix();
        }

        void Transform::setScale(const glm::vec3& new_scale) {
            m_scale = new_scale;
            invalidateWorldMatrix();
        }

        const glm::vec3& Transform::getTranslation() const {
            return m_translation;
        }

        const glm::quat& Transform::getRotation() const {
            return m_rotation;
        }

        const glm::vec3& Transform::getScale() const {
            return m_scale;
        }

        void Transform::setMatrix(const glm::mat4& matrix) {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(matrix, m_scale, m_rotation, m_translation, skew, perspective);
            invalidateWorldMatrix();
        }

        glm::mat4 Transform::getMatrix() const {
            return glm::translate(glm::mat4(1.0), m_translation) *
                glm::mat4_cast(m_rotation) *
                glm::scale(glm::mat4(1.0), m_scale);
        }

        glm::mat4 Transform::getWorldMatrix() {
            updateWorldTransform();
            return m_world_matrix;
        }

        void Transform::invalidateWorldMatrix() {
            m_update_world_matrix = true;
        }

        void Transform::updateWorldTransform() {
            if (!m_update_world_matrix) {
                return;
            }

            m_world_matrix = getMatrix();

            auto parent = m_node.getParent();

            if (parent) {
                auto& transform = parent->getComponent<Transform>();
                m_world_matrix = transform.getWorldMatrix() * m_world_matrix;
            }

            m_update_world_matrix = false;
        }
	}
}
