/* Copyright (c) 2019, Sascha Willems
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

#include "scene/frustum.h"
#include <cmath>

namespace frame {
	namespace scene {
		void Frustum::update(const glm::mat4& matrix) {
			m_planes[LEFT].x = matrix[0].w + matrix[0].x;
			m_planes[LEFT].y = matrix[1].w + matrix[1].x;
			m_planes[LEFT].z = matrix[2].w + matrix[2].x;
			m_planes[LEFT].w = matrix[3].w + matrix[3].x;

			m_planes[RIGHT].x = matrix[0].w - matrix[0].x;
			m_planes[RIGHT].y = matrix[1].w - matrix[1].x;
			m_planes[RIGHT].z = matrix[2].w - matrix[2].x;
			m_planes[RIGHT].w = matrix[3].w - matrix[3].x;

			m_planes[TOP].x = matrix[0].w - matrix[0].y;
			m_planes[TOP].y = matrix[1].w - matrix[1].y;
			m_planes[TOP].z = matrix[2].w - matrix[2].y;
			m_planes[TOP].w = matrix[3].w - matrix[3].y;

			m_planes[BOTTOM].x = matrix[0].w + matrix[0].y;
			m_planes[BOTTOM].y = matrix[1].w + matrix[1].y;
			m_planes[BOTTOM].z = matrix[2].w + matrix[2].y;
			m_planes[BOTTOM].w = matrix[3].w + matrix[3].y;

			m_planes[BACK].x = matrix[0].w + matrix[0].z;
			m_planes[BACK].y = matrix[1].w + matrix[1].z;
			m_planes[BACK].z = matrix[2].w + matrix[2].z;
			m_planes[BACK].w = matrix[3].w + matrix[3].z;

			m_planes[FRONT].x = matrix[0].w - matrix[0].z;
			m_planes[FRONT].y = matrix[1].w - matrix[1].z;
			m_planes[FRONT].z = matrix[2].w - matrix[2].z;
			m_planes[FRONT].w = matrix[3].w - matrix[3].z;
			
			for (size_t i = 0; i < m_planes.size(); i++) {
				float length = std::sqrt(m_planes[i].x * m_planes[i].x + m_planes[i].y * m_planes[i].y + m_planes[i].z * m_planes[i].z);
				m_planes[i] /= length;
			}
		}

		bool Frustum::checkSphere(glm::vec3 pos, float radius) {
			for (size_t i = 0; i < m_planes.size(); i++) {
				if ((m_planes[i].x * pos.x) + (m_planes[i].y * pos.y) + (m_planes[i].z * pos.z) + m_planes[i].w <= -radius) {
					return false;
				}
			}
			return true;
		}

		const std::array<glm::vec4, 6>& Frustum::getPlanes() const {
			return m_planes;
		}
	}
}
