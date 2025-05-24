/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "scene/components/transform.h"
#include "scene/scripts/script.h"

using TransformAnimFn = std::function<void(frame::scene::Transform&, float)>;

namespace frame {
    namespace scene {
        class NodeAnimation : public NodeScript {
        public:
            NodeAnimation(Node& node, TransformAnimFn animation_fn);

            virtual ~NodeAnimation() = default;

            virtual void update(float delta_time) override;

            void setAnimation(TransformAnimFn handle);

            void clearAnimation();

        private:
            TransformAnimFn m_animation_fn{};
        };
    }
}
