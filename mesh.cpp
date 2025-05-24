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

#include "scene/components/mesh/mesh.h"

namespace frame {
    namespace scene {

        Mesh::Mesh(const std::string& name) :
            Component{ name }
        {}

        void Mesh::updateBounds(const std::vector<glm::vec3>& vertex_data, const std::vector<uint16_t>& index_data) {
            m_bounds.update(vertex_data, index_data);
        }

        std::type_index Mesh::getType() {
            return typeid(Mesh);
        }

        const AABB& Mesh::getBounds() const {
            return m_bounds;
        }

        void Mesh::addSubmesh(SubMesh& submesh) {
            m_submeshes.push_back(&submesh);
        }

        const std::vector<SubMesh*>& Mesh::getSubmeshes() const {
            return reinterpret_cast<std::vector<SubMesh*> const&>(m_submeshes);
        }

        void Mesh::addNode(Node& node){
            m_nodes.push_back(&node);
        }

        const std::vector<Node*>& Mesh::getNodes() const {
            return m_nodes;
        }
    }
}
