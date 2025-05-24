/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2019-2024, Sascha Willems
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

#include "scene/components/image/ktx.h"

#include <ktx.h>
#include <ktxvulkan.h>

namespace frame {
    namespace scene {
        struct CallbackData final {
            ktxTexture* texture;
            std::vector<Mipmap>* mipmaps;
        };

        static ktx_error_code_e KTX_APIENTRY optimalTilingCallback(
            int mip_level,
            int face,
            int width,
            int height,
            int depth,
            ktx_uint64_t face_lod_size,
            void* pixels,
            void* user_data)
        {
            auto* callback_data = reinterpret_cast<CallbackData*>(user_data);
            assert(static_cast<size_t>(mip_level) < callback_data->mipmaps->size() && "Not enough space in the mipmap vector");

            ktx_size_t mipmap_offset = 0;
            auto result = ktxTexture_GetImageOffset(callback_data->texture, mip_level, 0, face, &mipmap_offset);

            if (result != KTX_SUCCESS) {
                return result;
            }

            auto& mipmap = callback_data->mipmaps->at(mip_level);
            mipmap.level = mip_level;
            mipmap.offset = common::toU32(mipmap_offset);
            mipmap.extent.width = width;
            mipmap.extent.height = height;
            mipmap.extent.depth = depth;

            return KTX_SUCCESS;
        }

        Ktx::Ktx(const std::string& name, const std::vector<uint8_t>& data, ContentType content_type) :
            Image{ name }
        {
            auto data_buffer = reinterpret_cast<const ktx_uint8_t*>(data.data());
            auto data_size = static_cast<ktx_size_t>(data.size());

            ktxTexture* texture;
            auto load_ktx_result = ktxTexture_CreateFromMemory(
                data_buffer,
                data_size,
                KTX_TEXTURE_CREATE_NO_FLAGS,
                &texture);

            if (load_ktx_result != KTX_SUCCESS) {
                throw std::runtime_error{ "Error loading KTX texture: " + name };
            }

            if (texture->pData) {
                setData(texture->pData, texture->dataSize);
            }
            else {
                auto& mut_data = getMutData();
                auto size = texture->dataSize;
                mut_data.resize(size);
                auto load_data_result = ktxTexture_LoadImageData(texture, mut_data.data(), size);
                if (load_data_result != KTX_SUCCESS) {
                    throw std::runtime_error{ "Error loading KTX image data: " + name };
                }
            }

            setWidth(texture->baseWidth);
            setHeight(texture->baseHeight);
            setDepth(texture->baseDepth);
            setLayers(texture->numLayers);

            bool cubemap = false;

            if (texture->numLayers == 1 && texture->numFaces == 6) {
                cubemap = true;
                setLayers(texture->numFaces);
            }

            auto updated_format = ktxTexture_GetVkFormat(texture);
            setFormat(static_cast<vk::Format>(updated_format));

            if (texture->classId == ktxTexture1_c && content_type == Color) {
                coerceFormatToSrgb();
            }

            auto& mipmap_levels = getMutMipmaps();
            mipmap_levels.resize(texture->numLevels);

            CallbackData callback_data{};
            callback_data.texture = texture;
            callback_data.mipmaps = &mipmap_levels;

            auto result = ktxTexture_IterateLevels(texture, optimalTilingCallback, &callback_data);
            if (result != KTX_SUCCESS) {
                throw std::runtime_error("Error loading KTX texture");
            }

            if (texture->numLayers > 1 || cubemap) {
                uint32_t layer_count = cubemap ? texture->numFaces : texture->numLayers;

                std::vector<std::vector<VkDeviceSize>> offsets;
                for (uint32_t layer = 0; layer < layer_count; layer++) {
                    std::vector<VkDeviceSize> layer_offsets{};
                    for (uint32_t level = 0; level < texture->numLevels; level++) {
                        ktx_size_t offset;
                        KTX_error_code result;
                        if (cubemap) {
                            result = ktxTexture_GetImageOffset(texture, level, 0, layer, &offset);
                        }
                        else {
                            result = ktxTexture_GetImageOffset(texture, level, layer, 0, &offset);
                        }
                        layer_offsets.push_back(static_cast<VkDeviceSize>(offset));
                    }
                    offsets.push_back(layer_offsets);
                }
                setOffsets(offsets);
            }
            else {
                std::vector<std::vector<VkDeviceSize>> offsets{};
                offsets.resize(1);
                for (size_t level = 0; level < mipmap_levels.size(); level++) {
                    offsets[0].push_back(static_cast<VkDeviceSize>(mipmap_levels[level].offset));
                }
                setOffsets(offsets);
            }

            ktxTexture_Destroy(texture);
        }
    }
}
