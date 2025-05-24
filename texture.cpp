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

#include "texture.h"
#include "scene/components/image/image.h"

namespace frame
{
    namespace scene
    {

        Texture::Texture(const std::string& name) :
            Component{ name }
        {
        }

        std::type_index Texture::getType()
        {
            return typeid(Texture);
        }

        void Texture::setImage(Image& image)
        {
            m_image = &image;
        }

        Image* Texture::getImage()
        {
            return m_image;
        }

        void Texture::setSampler(Sampler& sampler)
        {
            m_sampler = &sampler;
        }

        Sampler* Texture::getSampler()
        {
            assert(m_sampler && "Texture has no sampler");
            return m_sampler;
        }

    } // namespace scene
} // namespace frame