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

#include "scene/components/camera/perspective_camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace frame {
    namespace scene {
        PerspectiveCamera::PerspectiveCamera(const std::string& name) :
            Camera{ name }
        {}

        void PerspectiveCamera::setFieldOfView(float new_fov) {
            m_fov = new_fov;
        }

        float PerspectiveCamera::getFarPlane() const {
            return m_far_plane;
        }

        void PerspectiveCamera::setFarPlane(float zfar) {
            m_far_plane = zfar;
        }

        float PerspectiveCamera::getNearPlane() const {
            return m_near_plane;
        }

        void PerspectiveCamera::setNearPlane(float znear) {
            m_near_plane = znear;
        }

        void PerspectiveCamera::setAspectRatio(float new_aspect_ratio) {
            m_aspect_ratio = new_aspect_ratio;
        }

        float PerspectiveCamera::getFieldOfView() {
            return m_fov;
        }

        float PerspectiveCamera::getAspectRatio() {
            return m_aspect_ratio;
        }

        glm::mat4 PerspectiveCamera::getProjection() {
            return glm::perspective(m_fov, m_aspect_ratio, m_far_plane, m_near_plane);
        }
    }
}
