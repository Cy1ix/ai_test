/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include <algorithm>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

namespace frame {
    namespace scene {

        class Component {
        public:
            Component() = default;
            Component(const std::string& name);
            Component(Component&& other) = default;
            virtual ~Component() = default;

            const std::string& getName() const;
            virtual std::type_index getType() = 0;

        private:
            std::string m_name;
        };

        inline Component::Component(const std::string& name) : m_name{ name } {}

        inline const std::string& Component::getName() const {
            return m_name;
        }
    }
}
