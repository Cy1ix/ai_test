/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "global_common.h"
#include "scene/component.h"

namespace frame {
    namespace scene {
        class Texture;

        enum class AlphaMode {
            Opaque,
            Mask,
            Blend
        };

        class Material : public Component {
        public:
            Material(const std::string& name);
            Material(Material&& other) = default;
            virtual ~Material() = default;

            virtual std::type_index getType() override;

            std::unordered_map<std::string, Texture*> const& getTextures() const;

            std::unordered_map<std::string, Texture*> m_textures;

            glm::vec3 m_emissive{ 0.0f, 0.0f, 0.0f };

            bool m_double_sided{ false };

            float m_alpha_cutoff{ 0.5f };

            AlphaMode m_alpha_mode{ AlphaMode::Opaque };
        };
    }
}
