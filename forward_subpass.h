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

#pragma once

#include "common/buffer_pool.h"
#include "rendering/subpass/geometry_subpass.h"
#include "rendering/render_context.h"

#define MAX_FORWARD_LIGHT_COUNT 8

namespace frame {
    namespace scene {
        class Scene;
        class Node;
        class Mesh;
        class SubMesh;
        class Camera;
    }

    namespace rendering {

        struct alignas(16) ForwardLights {
            Light directional_lights[MAX_FORWARD_LIGHT_COUNT];
            Light point_lights[MAX_FORWARD_LIGHT_COUNT];
            Light spot_lights[MAX_FORWARD_LIGHT_COUNT];
        };

        namespace subpass {
            class ForwardSubpass : public GeometrySubpass {
            public:
                ForwardSubpass(RenderContext& render_context,
                    core::ShaderSource&& vertex_shader,
                    core::ShaderSource&& fragment_shader,
                    scene::Scene& scene,
                    scene::Camera& camera);

                virtual ~ForwardSubpass() = default;

                virtual void prepare() override;

                virtual void draw(core::CommandBuffer& command_buffer) override;
            };
        }
    }
}
