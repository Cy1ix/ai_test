/* Copyright (c) 2023-2024, Arm Limited and Contributors
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

#include "scene/components/camera/orthographic_camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace frame {
    namespace scene {
        OrthographicCamera::OrthographicCamera(const std::string& name) :
            Camera{ name }
        {}

        OrthographicCamera::OrthographicCamera(const std::string& name, float left, float right, float bottom, float top, float near_plane, float far_plane) :
            Camera{ name },
            m_left{ left },
            m_right{ right },
            m_top{ top },
            m_bottom{ bottom },
            m_near_plane{ near_plane },
            m_far_plane{ far_plane }
        {}

        void OrthographicCamera::setLeft(float left) {
            m_left = left;
        }

        float OrthographicCamera::getLeft() const {
            return m_left;
        }

        void OrthographicCamera::setRight(float right) {
            m_right = right;
        }

        float OrthographicCamera::getRight() const {
            return m_right;
        }

        void OrthographicCamera::setBottom(float bottom) {
            m_bottom = bottom;
        }

        float OrthographicCamera::getBottom() const {
            return m_bottom;
        }

        void OrthographicCamera::setTop(float top) {
            m_top = top;
        }

        float OrthographicCamera::getTop() const {
            return m_top;
        }

        void OrthographicCamera::setNearPlane(float near_plane) {
            m_near_plane = near_plane;
        }

        float OrthographicCamera::getNearPlane() const {
            return m_near_plane;
        }

        void OrthographicCamera::setFarPlane(float far_plane) {
            m_far_plane = far_plane;
        }

        float OrthographicCamera::getFarPlane() const {
            return m_far_plane;
        }

        glm::mat4 OrthographicCamera::getProjection() {
            return glm::ortho(m_left, m_right, m_bottom, m_top, m_far_plane, m_near_plane);
        }
    }
}
