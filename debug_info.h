/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <type_traits>

#include "global_common.h"
#include "common/helper.h"
#include "common/strings.h"

namespace frame {
    namespace common {
        namespace field {
            struct Base {
                std::string m_label;

                Base(const std::string& label) :
                    m_label{ label }
                {
                }

                virtual ~Base() = default;

                virtual const std::string toString() = 0;
            };

            template <typename T>
            struct Static : public Base {
                T m_value;

                Static(const std::string& label, const T& value) :
                    Base(label),
                    m_value{ value }
                {
                }

                virtual ~Static() = default;

                const std::string toString() override {
                    return common::toString(m_value);
                }
            };

            template <typename T>
            struct Dynamic : public Base {
                T& m_value;

                Dynamic(const std::string& label, T& value) :
                    Base(label),
                    m_value{ value }
                {
                }

                virtual ~Dynamic() = default;

                const std::string toString() override {
                    return common::toString(m_value);
                }
            };

            template <typename T>
            struct Vector final : public Static<T> {
                T m_x, m_y, m_z;

                Vector(const std::string& label, const glm::vec3& vec) :
                    Vector(label, vec.x, vec.y, vec.z)
                {
                }

                Vector(const std::string& label, T x, T y, T z) :
                    Static<T>(label, x),
                    m_x{ x },
                    m_y{ y },
                    m_z{ z }
                {}

                virtual ~Vector() = default;

                const std::string toString() override {
                    return std::string("x: " + common::toString(m_x) + " " + "y: " + common::toString(m_y) + " " + "z: " + common::toString(m_z));
                }
            };

            template <typename T>
            struct MinMax final : public Dynamic<T> {
                T m_min, m_max;

                MinMax(const std::string& label, T& value) :
                    Dynamic<T>(label, value),
                    m_min{ value },
                    m_max{ value }
                {
                    static_assert(std::is_arithmetic<T>::value, "MinMax must be templated to a numeric type.");
                }

                virtual ~MinMax() = default;

                const std::string toString() override {
                    if (this->m_value > m_max) {
                        m_max = this->m_value;
                    }
                    if (this->m_value < m_min) {
                        m_min = this->m_value;
                    }
                    if (m_min == 0) {
                        m_min = this->m_value;
                    }

                    return std::string("current: " + common::toString(this->m_value) + " min: " + common::toString(m_min) + " max: " + common::toString(m_max));
                }
            };
        }

        class DebugInfo {
        public:
            const std::vector<std::unique_ptr<field::Base>>& getFields() const {
                return m_fields;
            }

            float getLongestLabel() const {
                float column_width = 0.0f;
                for (auto& field : m_fields) {
                    const std::string& label = field->m_label;

                    if (label.size() > column_width) {
                        column_width = static_cast<float>(label.size());
                    }
                }
                return column_width;
            }

            template <template <typename> class C, typename T, typename... A>
            void insert(const std::string& label, A &&...args) {
                static_assert(std::is_base_of<field::Base, C<T>>::value, "C is not a type of field::Base.");

                for (auto& field : m_fields)
                {
                    if (field->m_label == label)
                    {
                        if (dynamic_cast<typename field::template Static<T> *>(field.get()))
                        {
                            field = std::make_unique<C<T>>(label, std::forward<A>(args)...);
                        }
                        return;
                    }
                }

                auto field = std::make_unique<C<T>>(label, std::forward<A>(args)...);
                m_fields.push_back(std::move(field));
            }

        private:
            std::vector<std::unique_ptr<field::Base>> m_fields;
        };
    }
}
