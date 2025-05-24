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
#include <unordered_map>
#include <vector>

#include "global_common.h"
#include "scene/components/camera/camera.h"

namespace frame {
    namespace scene {
        class PerspectiveCamera : public Camera {
        public:
            PerspectiveCamera(const std::string& name);
            virtual ~PerspectiveCamera() = default;

            void setAspectRatio(float aspect_ratio) override;
            void setFieldOfView(float fov);
            float getFarPlane() const;
            void setFarPlane(float zfar);
            float getNearPlane() const;
            void setNearPlane(float znear);
            float getAspectRatio();
            float getFieldOfView();

            virtual glm::mat4 getProjection() override;

        private:
            float m_aspect_ratio{ 1.0f };

            float m_fov{ glm::radians(60.0f) };

            float m_far_plane{ 100.0 };
            float m_near_plane{ 0.1f };
        };
    }
}
