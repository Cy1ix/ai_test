/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "scene/components/mesh/sub_mesh.h"
#include "scene/components/material/material.h"

namespace frame {
    namespace scene {

        SubMesh::SubMesh(const std::string& name) :
            Component{ name }
        {}

        std::type_index SubMesh::getType() {
            return typeid(SubMesh);
        }

        void SubMesh::setAttribute(const std::string& attribute_name, const VertexAttribute& attribute) {
            m_vertex_attributes[attribute_name] = attribute;
            computeShaderVariant();
        }

        bool SubMesh::getAttribute(const std::string& attribute_name, VertexAttribute& attribute) const {
            auto attrib_it = m_vertex_attributes.find(attribute_name);

            if (attrib_it == m_vertex_attributes.end()) {
                return false;
            }

            attribute = attrib_it->second;
            return true;
        }

        void SubMesh::setMaterial(const Material& new_material) {
            m_material = &new_material;
            computeShaderVariant();
        }

        const Material* SubMesh::getMaterial() const {
            return m_material;
        }

        const core::ShaderVariant& SubMesh::getShaderVariant() const {
            return m_shader_variant;
        }

        core::ShaderVariant& SubMesh::getMutShaderVariant() {
            return m_shader_variant;
        }

        common::Buffer const& SubMesh::getIndexBuffer() const {
            return *m_index_buffer;
        }

        vk::IndexType SubMesh::getIndexType() const {
            return m_index_type;
        }

        common::Buffer const& SubMesh::getVertexBuffer(std::string const& name) const {
            return m_vertex_buffers.at(name);
        }

        void SubMesh::computeShaderVariant() {
            m_shader_variant.clear();

            if (m_material != nullptr) {
                for (auto& texture : m_material->m_textures) {
                    std::string tex_name = texture.first;
                    std::transform(tex_name.begin(), tex_name.end(), tex_name.begin(), ::toupper);

                    m_shader_variant.addDefine("HAS_" + tex_name);
                }
            }

            for (auto& attribute : m_vertex_attributes) {
                std::string attrib_name = attribute.first;
                std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::toupper);
                m_shader_variant.addDefine("HAS_" + attrib_name);
            }
        }
    }
}
