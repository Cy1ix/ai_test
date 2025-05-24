/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/subpass/forward_subpass.h"

#include "common/common.h"
#include "rendering/subpass.h"
#include "rendering/render_context.h"
#include "scene/components/camera/camera.h"
#include "scene/components/image/image.h"
#include "scene/components/mesh/mesh.h"
#include "scene/components/mesh/sub_mesh.h"
#include "scene/components/material/material.h"
#include "scene/components/material/pbr_material.h"
#include "scene/components/texture.h"
#include "scene/node.h"
#include "scene/scene.h"

namespace frame {
    namespace rendering {
        namespace subpass {
            ForwardSubpass::ForwardSubpass(rendering::RenderContext& render_context,
                core::ShaderSource&& vertex_shader,
                core::ShaderSource&& fragment_shader,
                scene::Scene& scene,
                scene::Camera& camera) :
                GeometrySubpass{ render_context,
                    std::forward<core::ShaderSource>(vertex_shader),
                    std::forward<core::ShaderSource>(fragment_shader),
                    scene,
                    camera }
            {
            }

            void ForwardSubpass::prepare() {

                auto& device = getRenderContext().getDevice();

                for (auto& mesh : m_meshes) {

                    for (auto& sub_mesh : mesh->getSubmeshes()) {

                        auto& variant = sub_mesh->getMutShaderVariant();

                        variant.addDefinitions({ "MAX_LIGHT_COUNT " + std::to_string(MAX_FORWARD_LIGHT_COUNT) });
                        variant.addDefinitions(light_type_definitions);

                        auto& vert_module = device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eVertex, getVertexShader(), variant);
                        auto& frag_module = device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eFragment, getFragmentShader(), variant);
                    }
                }
            }

            void ForwardSubpass::draw(core::CommandBuffer& command_buffer) {

                allocateLights<ForwardLights>(m_scene.getComponents<scene::Light>(), MAX_FORWARD_LIGHT_COUNT);
                command_buffer.bindLighting(getLightingState(), 0, 4);

                GeometrySubpass::draw(command_buffer);
            }
        }
    }
}
