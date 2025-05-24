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

#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>
#include "common/buffer.h"
#include "core/shader_module.h"
#include "scene/component.h"

namespace frame {
    namespace scene {
        class Material;

        struct VertexAttribute {
            vk::Format format = vk::Format::eUndefined;
            std::uint32_t stride = 0;
            std::uint32_t offset = 0;
        };

        class SubMesh : public Component {
        public:
            SubMesh(const std::string& name = {});
            virtual ~SubMesh() = default;

            virtual std::type_index getType() override;

            void setAttribute(const std::string& attribute_name, const VertexAttribute& attribute);
            bool getAttribute(const std::string& attribute_name, VertexAttribute& attribute) const;

            void setMaterial(const Material& material);
            const Material* getMaterial() const;

            const core::ShaderVariant& getShaderVariant() const;
            core::ShaderVariant& getMutShaderVariant();

            common::Buffer const& getIndexBuffer() const;
            vk::IndexType getIndexType() const;
            common::Buffer const& getVertexBuffer(std::string const& name) const;

            vk::IndexType m_index_type{};
            std::uint32_t m_index_offset = 0;
            std::uint32_t m_vertices_count = 0;
            std::uint32_t m_vertex_indices = 0;

            std::unordered_map<std::string, common::Buffer> m_vertex_buffers;
            std::unique_ptr<common::Buffer> m_index_buffer;

        private:
            void computeShaderVariant();

            std::unordered_map<std::string, VertexAttribute> m_vertex_attributes;
            const Material* m_material{ nullptr };
            core::ShaderVariant m_shader_variant;
        };
    }
}
