/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "scene/component.h"
#include "scene/components/aabb.h"

namespace frame {
    namespace scene {
        
        class Node;
        class SubMesh;
        
        class Mesh : public Component {
        public:
            Mesh(const std::string& name);
            virtual ~Mesh() = default;

            void updateBounds(const std::vector<glm::vec3>& vertex_data, const std::vector<uint16_t>& index_data = {});
            virtual std::type_index getType() override;
            const AABB& getBounds() const;
            void addSubmesh(SubMesh& submesh);
            const std::vector<SubMesh*>& getSubmeshes() const;
            void addNode(Node& node);
            const std::vector<Node*>& getNodes() const;

        private:
            AABB m_bounds;
            std::vector<SubMesh*> m_submeshes;
            std::vector<Node*> m_nodes;
        };

    }
}
