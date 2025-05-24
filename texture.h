/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "scene/component.h"
#include "scene/components/sampler.h"

namespace frame {
    namespace scene {
        class Image;

        class Texture : public Component {
        public:
            Texture(const std::string& name);
            Texture(Texture&& other) = default;
            virtual ~Texture() = default;

            virtual std::type_index getType() override;

            void setImage(Image& image);
            Image* getImage();

            void setSampler(Sampler& sampler);
            Sampler* getSampler();

        private:
            Image* m_image{ nullptr };
            Sampler* m_sampler{ nullptr };
        };
    }
}
