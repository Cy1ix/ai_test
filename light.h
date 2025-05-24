/* Copyright (c) 2018-2024, Arm Limited and Contributors
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
#include <vector>

#include "global_common.h"

#include "core/shader_module.h"
#include "scene/component.h"

namespace frame {
    namespace scene {

        class Node;

        enum LightType {
            Directional = 0,
            Point = 1,
            Spot = 2,

            Max
        };

        struct LightProperties {
            glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
            glm::vec3 color{ 1.0f, 1.0f, 1.0f };
            float intensity{ 1.0f };
            float range{ 0.0f };
            float inner_cone_angle{ 0.0f };
            float outer_cone_angle{ 0.0f };
        };

        class Light : public Component {
        public:
            Light(const std::string& name);
            Light(Light&& other) = default;
            virtual ~Light() override = default;

            virtual std::type_index getType() override;

            void setNode(Node& node);
            Node* getNode();

            void setLightType(const LightType& type);
            const LightType& getLightType();

            void setProperties(const LightProperties& properties);
            const LightProperties& getProperties();

            void setDirection(const glm::vec3& d);
            void setColor(const glm::vec3& c);
            void setIntensity(const float& i);
            void setRange(const float& r);
            void setInnerConeAngle(const float& ica);
            void setOuterConeAngle(const float& oca);
        private:
            Node* m_node{ nullptr };
            LightType m_light_type{ Directional };
            LightProperties m_properties;
        };

        inline Light::Light(const std::string& name) : Component{ name } {}

        inline std::type_index Light::getType() {
            return typeid(Light);
        }

        inline void Light::setNode(Node& node) {
            m_node = &node;
        }

        inline Node* Light::getNode() {
            return m_node;
        }

        inline void Light::setLightType(const LightType& type) {
            m_light_type = type;
        }

        inline const LightType& Light::getLightType() {
            return m_light_type;
        }

        inline void Light::setProperties(const LightProperties& properties) {
            m_properties = properties;
        }

        inline const LightProperties& Light::getProperties() {
            return m_properties;
        }

        inline void Light::setDirection(const glm::vec3& d) { m_properties.direction = d; }
        inline void Light::setColor(const glm::vec3& c) { m_properties.color = c; }
        inline void Light::setIntensity(const float& i) { m_properties.intensity = i; }
        inline void Light::setRange(const float& r) { m_properties.range = r; }
        inline void Light::setInnerConeAngle(const float& ica) { m_properties.inner_cone_angle = ica; }
        inline void Light::setOuterConeAngle(const float& oca) { m_properties.outer_cone_angle = oca; }
    }
}
