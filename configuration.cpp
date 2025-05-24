/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "platform/configuration.h"

namespace frame {
	namespace platform {
        BoolSetting::BoolSetting(bool& handle, bool value) :
            m_handle{ handle },
            m_value{ value }
        {
        }

        void BoolSetting::apply() {
            m_handle = m_value;
        }

        std::type_index BoolSetting::getType() {
            return typeid(BoolSetting);
        }

        IntSetting::IntSetting(int& handle, int value) :
            m_handle{ handle },
            m_value{ value }
        {
        }

        void IntSetting::apply() {
            m_handle = m_value;
        }

        std::type_index IntSetting::getType() {
            return typeid(IntSetting);
        }

        EmptySetting::EmptySetting() {}

        void EmptySetting::apply() {}

        std::type_index EmptySetting::getType() {
            return typeid(EmptySetting);
        }

        void Configuration::apply() {
            for (auto& pair : m_current_configuration->second) {
                for (auto setting : pair.second) {
                    setting->apply();
                }
            }
        }

        bool Configuration::next() {
            if (m_configs.empty()) {
                return false;
            }

            m_current_configuration++;

            if (m_current_configuration == m_configs.end()) {
                return false;
            }

            return true;
        }

        void Configuration::reset() {
            m_current_configuration = m_configs.begin();
        }

        void Configuration::insertSetting(uint32_t config_index, std::unique_ptr<Setting> setting) {
            m_settings.push_back(std::move(setting));
            m_configs[config_index][m_settings.back()->getType()].push_back(m_settings.back().get());
        }
	}
}
