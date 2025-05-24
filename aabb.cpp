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

#include "aabb.h"

#include "utils/logger.h"

namespace frame {
    namespace scene {
        AABB::AABB() {
            reset();
        }

        AABB::AABB(const glm::vec3& min, const glm::vec3& max) :
            m_min{ min },
            m_max{ max }
        {
        }

        std::type_index AABB::getType() {
            return typeid(AABB);
        }

        void AABB::update(const glm::vec3& point) {
            m_min = glm::min(m_min, point);
            m_max = glm::max(m_max, point);
        }

        void AABB::update(const std::vector<glm::vec3>& vertex_data, const std::vector<uint16_t>& index_data) {

            if (!index_data.empty()) {
                for (const auto& index : index_data) {
                    update(vertex_data[index]);
                }
            }
            else {
                for (const auto& vertex : vertex_data) {
                    update(vertex);
                }
            }
        }

        void AABB::transform(glm::mat4& transform) {
            m_min = m_max = glm::vec4(m_min, 1.0f) * transform;

            update(glm::vec4(m_min.x, m_min.y, m_max.z, 1.0f) * transform);
            update(glm::vec4(m_min.x, m_max.y, m_min.z, 1.0f) * transform);
            update(glm::vec4(m_min.x, m_max.y, m_max.z, 1.0f) * transform);
            update(glm::vec4(m_max.x, m_min.y, m_min.z, 1.0f) * transform);
            update(glm::vec4(m_max.x, m_min.y, m_max.z, 1.0f) * transform);
            update(glm::vec4(m_max.x, m_max.y, m_min.z, 1.0f) * transform);
            update(glm::vec4(m_max, 1.0f) * transform);
        }

        glm::vec3 AABB::getScale() const {
            return (m_max - m_min);
        }

        glm::vec3 AABB::getCenter() const {
            return (m_min + m_max) * 0.5f;
        }

        glm::vec3 AABB::getMin() const {
            return m_min;
        }

        glm::vec3 AABB::getMax() const {
            return m_max;
        }

        void AABB::reset() {
            m_min = std::numeric_limits<glm::vec3>::max();
            m_max = std::numeric_limits<glm::vec3>::min();
        }
    }
}