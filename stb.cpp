/* Copyright (c) 2019-2022, Arm Limited and Contributors
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

#include "scene/components/image/stb.h"

//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace frame {
    namespace scene {
        Stb::Stb(const std::string& name, const std::vector<uint8_t>& data, ContentType content_type) :
            Image{ name }
        {
            int width;
            int height;
            int comp;
            int req_comp = 4;  // Always request RGBA

            auto data_buffer = reinterpret_cast<const stbi_uc*>(data.data());
            auto data_size = static_cast<int>(data.size());

            auto raw_data = stbi_load_from_memory(data_buffer, data_size, &width, &height, &comp, req_comp);

            if (!raw_data) {
                throw std::runtime_error{ "Failed to load " + name + ": " + stbi_failure_reason() };
            }

            setData(raw_data, width * height * req_comp);
            stbi_image_free(raw_data);

            setFormat(content_type == Color ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm);
            setWidth(common::toU32(width));
            setHeight(common::toU32(height));
            setDepth(1u);
        }
    }
}