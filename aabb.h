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

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "global_common.h"
#include "scene/component.h"
#include "scene/components/mesh/sub_mesh.h"

namespace frame {
    namespace scene {
        class AABB : public Component {
        public:
            AABB();

            AABB(const glm::vec3& min, const glm::vec3& max);

            virtual ~AABB() = default;

            virtual std::type_index getType() override;
            
            void update(const glm::vec3& point);
            
            void update(const std::vector<glm::vec3>& vertex_data, const std::vector<uint16_t>& index_data);
            
            void transform(glm::mat4& transform);
            
            glm::vec3 getScale() const;
            
            glm::vec3 getCenter() const;
            
            glm::vec3 getMin() const;
            
            glm::vec3 getMax() const;
            
            void reset();

        private:
            glm::vec3 m_min;
            glm::vec3 m_max;
        };
    }
}
