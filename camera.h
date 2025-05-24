/* Copyright (c) 2019-2024, Arm Limited and Contributors
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
#include "common/helper.h"
#include "scene/component.h"

namespace frame {
    namespace scene {
        class Node;

        class Camera : public Component {
        public:
            Camera(const std::string& name);

            virtual ~Camera() = default;

            virtual std::type_index getType() override;

            virtual glm::mat4 getProjection() = 0;

            virtual void setAspectRatio(float aspect_ratio) = 0;

            glm::mat4 getView();

            void setNode(Node& node);

            Node* getNode();

            const glm::mat4 getPreRotation();

            void setPreRotation(const glm::mat4& pre_rotation);

        private:
            Node* m_node{ nullptr };
            glm::mat4 m_pre_rotation{ 1.0f };
        };
    }
}
