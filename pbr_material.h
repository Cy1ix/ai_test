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
#include <unordered_map>
#include <vector>

#include "global_common.h"
#include "scene/components/material/material.h"

namespace frame {
    namespace scene {
        class PBRMaterial : public Material {
        public:
            PBRMaterial(const std::string& name);
            virtual ~PBRMaterial() = default;
            virtual std::type_index getType() override;

            glm::vec4 m_color{ 0.0f, 0.0f, 0.0f, 0.0f };
            float m_metallic{ 0.0f };
            float m_roughness{ 0.0f };
        };
    }
}
