/* Copyright (c) 2020-2024, Arm Limited and Contributors
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

#include "global_common.h"
#include "scene/components/camera/camera.h"

namespace frame {
    namespace scene {
        class OrthographicCamera : public Camera {
        public:
            OrthographicCamera(const std::string& name);
            OrthographicCamera(const std::string& name, float left, float right, float bottom, float top, float near_plane, float far_plane);
            virtual ~OrthographicCamera() = default;

            void setLeft(float left);
            float getLeft() const;

            void setRight(float right);
            float getRight() const;

            void setBottom(float bottom);
            float getBottom() const;

            void setTop(float top);
            float getTop() const;

            void setNearPlane(float near_plane);
            float getNearPlane() const;

            void setFarPlane(float far_plane);
            float getFarPlane() const;

            virtual glm::mat4 getProjection() override;

        private:
            float m_left{ -1.0f };
            float m_right{ 1.0f };
            float m_bottom{ -1.0f };
            float m_top{ 1.0f };
            float m_near_plane{ 0.0f };
            float m_far_plane{ 1.0f };
        };
    }
}
