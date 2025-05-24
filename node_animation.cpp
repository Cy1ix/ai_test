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

#include "node_animation.h"

#include "global_common.h"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include "scene/components/camera/perspective_camera.h"
#include "scene/components/transform.h"
#include "scene/node.h"

namespace frame {
    namespace scene {
        NodeAnimation::NodeAnimation(Node& node, TransformAnimFn animation_fn) :
            NodeScript{ node, "" },
            m_animation_fn{ animation_fn }
        {}

        void NodeAnimation::update(float delta_time) {
            if (m_animation_fn) {
                m_animation_fn(getNode().getComponent<Transform>(), delta_time);
            }
        }

        void NodeAnimation::setAnimation(TransformAnimFn handle) {
            m_animation_fn = handle;
        }

        void NodeAnimation::clearAnimation() {
            m_animation_fn = {};
        }
    }
}
