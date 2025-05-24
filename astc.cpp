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

#include "scene/components/image/astc.h"

#include <mutex>

#include "common/profiling.h"

#include "global_common.h"
#if defined(_WIN32) || defined(_WIN64)
	#undef IGNORE
#endif
#include <astcenc.h>

#define MAGIC_FILE_CONSTANT 0x5CA1AB13

namespace frame {
    namespace scene {
        BlockDim toBlockDim(vk::Format format) {
            switch (format) {
            case vk::Format::eAstc4x4UnormBlock:
            case vk::Format::eAstc4x4SrgbBlock:
                return BlockDim{ 4, 4, 1 };
            case vk::Format::eAstc5x4UnormBlock:
            case vk::Format::eAstc5x4SrgbBlock:
                return BlockDim{ 5, 4, 1 };
            case vk::Format::eAstc5x5UnormBlock:
            case vk::Format::eAstc5x5SrgbBlock:
                return BlockDim{ 5, 5, 1 };
            case vk::Format::eAstc6x5UnormBlock:
            case vk::Format::eAstc6x5SrgbBlock:
                return BlockDim{ 6, 5, 1 };
            case vk::Format::eAstc6x6UnormBlock:
            case vk::Format::eAstc6x6SrgbBlock:
                return BlockDim{ 6, 6, 1 };
            case vk::Format::eAstc8x5UnormBlock:
            case vk::Format::eAstc8x5SrgbBlock:
                return BlockDim{ 8, 5, 1 };
            case vk::Format::eAstc8x6UnormBlock:
            case vk::Format::eAstc8x6SrgbBlock:
                return BlockDim{ 8, 6, 1 };
            case vk::Format::eAstc8x8UnormBlock:
            case vk::Format::eAstc8x8SrgbBlock:
                return BlockDim{ 8, 8, 1 };
            case vk::Format::eAstc10x5UnormBlock:
            case vk::Format::eAstc10x5SrgbBlock:
                return BlockDim{ 10, 5, 1 };
            case vk::Format::eAstc10x6UnormBlock:
            case vk::Format::eAstc10x6SrgbBlock:
                return BlockDim{ 10, 6, 1 };
            case vk::Format::eAstc10x8UnormBlock:
            case vk::Format::eAstc10x8SrgbBlock:
                return BlockDim{ 10, 8, 1 };
            case vk::Format::eAstc10x10UnormBlock:
            case vk::Format::eAstc10x10SrgbBlock:
                return BlockDim{ 10, 10, 1 };
            case vk::Format::eAstc12x10UnormBlock:
            case vk::Format::eAstc12x10SrgbBlock:
                return BlockDim{ 12, 10, 1 };
            case vk::Format::eAstc12x12UnormBlock:
            case vk::Format::eAstc12x12SrgbBlock:
                return BlockDim{ 12, 12, 1 };
            default:
                throw vk::LogicError("Invalid astc format");
            }
        }

        struct AstcHeader {
            uint8_t magic[4];
            uint8_t blockdim_x;
            uint8_t blockdim_y;
            uint8_t blockdim_z;
            uint8_t xsize[3];
            uint8_t ysize[3];
            uint8_t zsize[3];
        };

        void Astc::init() {
            // Empty initialization (possibly for future use)
        }

        void Astc::decode(BlockDim blockdim, vk::Extent3D extent, const uint8_t* compressed_data, uint32_t compressed_size) {
            PROFILE_SCOPE("Decode ASTC Image");

            astcenc_swizzle swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

            astcenc_config astc_config;
            auto astc_result = astcenc_config_init(
                ASTCENC_PRF_LDR_SRGB,
                blockdim.x,
                blockdim.y,
                blockdim.z,
                ASTCENC_PRE_FAST,
                ASTCENC_FLG_DECOMPRESS_ONLY,
                &astc_config);
            if (astc_result != ASTCENC_SUCCESS) {
                throw std::runtime_error{ "Error initializing astc" };
            }
            if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
                throw std::runtime_error{ "Error reading astc: invalid size" };
            }

            astcenc_context* astc_context;
            astcenc_context_alloc(&astc_config, 1, &astc_context);

            astcenc_image decoded{};
            decoded.dim_x = extent.width;
            decoded.dim_y = extent.height;
            decoded.dim_z = extent.depth;
            decoded.data_type = ASTCENC_TYPE_U8;

            auto uncompressed_size = decoded.dim_x * decoded.dim_y * decoded.dim_z * 4;
            auto& decoded_data = getMutData();
            decoded_data.resize(uncompressed_size);
            void* data_ptr = static_cast<void*>(decoded_data.data());
            decoded.data = &data_ptr;

            astc_result = astcenc_decompress_image(astc_context, compressed_data, compressed_size, &decoded, &swizzle, 0);
            if (astc_result != ASTCENC_SUCCESS) {
                throw std::runtime_error("Error decoding astc");
            }

            astcenc_context_free(astc_context);

            setFormat(vk::Format::eR8G8B8A8Srgb);
            setWidth(decoded.dim_x);
            setHeight(decoded.dim_y);
            setDepth(decoded.dim_z);
        }

        Astc::Astc(const Image& image) :
            Image{ image.getName() }
        {
            init();

            auto mip_it = std::find_if(image.getMipmaps().begin(), image.getMipmaps().end(),
                [](auto& mip) { return mip.level == 0; });
            assert(mip_it != image.getMipmaps().end() && "Mip #0 not found");

            const auto blockdim = toBlockDim(image.getFormat());
            const auto& extent = mip_it->extent;
            auto size = extent.width * extent.height * extent.depth * 4;
            const uint8_t* data_ptr = image.getData().data() + mip_it->offset;
            decode(blockdim, mip_it->extent, data_ptr, size);
        }

        Astc::Astc(const std::string& name, const std::vector<uint8_t>& data) :
            Image{ name }
        {
            init();

            if (data.size() < sizeof(AstcHeader))
            {
                throw std::runtime_error{ "Error reading astc: invalid memory" };
            }

            AstcHeader header{};
            std::memcpy(&header, data.data(), sizeof(AstcHeader));

            uint32_t magic_val = header.magic[0] +
                256 * static_cast<uint32_t>(header.magic[1]) +
                65536 * static_cast<uint32_t>(header.magic[2]) +
                16777216 * static_cast<uint32_t>(header.magic[3]);
            if (magic_val != MAGIC_FILE_CONSTANT)
            {
                throw std::runtime_error{ "Error reading astc: invalid magic" };
            }

            BlockDim blockdim = {
                header.blockdim_x,
                header.blockdim_y,
                header.blockdim_z
            };

            vk::Extent3D extent = {
            	static_cast<uint32_t>(header.xsize[0] + 256 * header.xsize[1] + 65536 * header.xsize[2]),
            	static_cast<uint32_t>(header.ysize[0] + 256 * header.ysize[1] + 65536 * header.ysize[2]),
            	static_cast<uint32_t>(header.zsize[0] + 256 * header.zsize[1] + 65536 * header.zsize[2])
            };

            decode(blockdim, extent, data.data() + sizeof(AstcHeader),
                common::toU32(data.size() - sizeof(AstcHeader)));
        }
    }
}
