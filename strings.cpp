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

#include "common/strings.h"

#include <fmt/format.h>

#include "core/shader_module.h"
#include "scene/components/material/material.h"

namespace frame {
	namespace common {
		std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
			if (str.empty()) {
				return {};
			}

			std::vector<std::string> out;

			std::string buffer = str;
			size_t      last_found_pos = 0;
			size_t      pos = 0;
			while ((pos = buffer.find(delimiter)) != std::string::npos) {
				out.push_back(buffer.substr(0, pos));
				buffer.erase(0, pos + delimiter.length());
				last_found_pos = last_found_pos + pos + delimiter.length();
			}

			if (last_found_pos == str.size()) {
				out.push_back("");
			}

			return out;
		}

		std::string join(const std::vector<std::string>& str, const std::string& separator) {
			std::stringstream out;
			for (auto it = str.begin(); it != str.end(); it++) {
				out << *it;
				if (it != str.end() - 1) {
					out << separator;
				}
			}
			return out.str();
		}

		const std::string toString(vk::Format format) {
			switch (format) {
			case vk::Format::eR4G4UnormPack8:
				return "VK_FORMAT_R4G4_UNORM_PACK8";
			case vk::Format::eR4G4B4A4UnormPack16:
				return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
			case vk::Format::eB4G4R4A4UnormPack16:
				return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
			case vk::Format::eR5G6B5UnormPack16:
				return "VK_FORMAT_R5G6B5_UNORM_PACK16";
			case vk::Format::eB5G6R5UnormPack16:
				return "VK_FORMAT_B5G6R5_UNORM_PACK16";
			case vk::Format::eR5G5B5A1UnormPack16:
				return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
			case vk::Format::eB5G5R5A1UnormPack16:
				return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
			case vk::Format::eA1R5G5B5UnormPack16:
				return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
			case vk::Format::eR8Unorm:
				return "VK_FORMAT_R8_UNORM";
			case vk::Format::eR8Snorm:
				return "VK_FORMAT_R8_SNORM";
			case vk::Format::eR8Uscaled:
				return "VK_FORMAT_R8_USCALED";
			case vk::Format::eR8Sscaled:
				return "VK_FORMAT_R8_SSCALED";
			case vk::Format::eR8Uint:
				return "VK_FORMAT_R8_UINT";
			case vk::Format::eR8Sint:
				return "VK_FORMAT_R8_SINT";
			case vk::Format::eR8Srgb:
				return "VK_FORMAT_R8_SRGB";
			case vk::Format::eR8G8Unorm:
				return "VK_FORMAT_R8G8_UNORM";
			case vk::Format::eR8G8Snorm:
				return "VK_FORMAT_R8G8_SNORM";
			case vk::Format::eR8G8Uscaled:
				return "VK_FORMAT_R8G8_USCALED";
			case vk::Format::eR8G8Sscaled:
				return "VK_FORMAT_R8G8_SSCALED";
			case vk::Format::eR8G8Uint:
				return "VK_FORMAT_R8G8_UINT";
			case vk::Format::eR8G8Sint:
				return "VK_FORMAT_R8G8_SINT";
			case vk::Format::eR8G8Srgb:
				return "VK_FORMAT_R8G8_SRGB";
			case vk::Format::eR8G8B8Unorm:
				return "VK_FORMAT_R8G8B8_UNORM";
			case vk::Format::eR8G8B8Snorm:
				return "VK_FORMAT_R8G8B8_SNORM";
			case vk::Format::eR8G8B8Uscaled:
				return "VK_FORMAT_R8G8B8_USCALED";
			case vk::Format::eR8G8B8Sscaled:
				return "VK_FORMAT_R8G8B8_SSCALED";
			case vk::Format::eR8G8B8Uint:
				return "VK_FORMAT_R8G8B8_UINT";
			case vk::Format::eR8G8B8Sint:
				return "VK_FORMAT_R8G8B8_SINT";
			case vk::Format::eR8G8B8Srgb:
				return "VK_FORMAT_R8G8B8_SRGB";
			case vk::Format::eB8G8R8Unorm:
				return "VK_FORMAT_B8G8R8_UNORM";
			case vk::Format::eB8G8R8Snorm:
				return "VK_FORMAT_B8G8R8_SNORM";
			case vk::Format::eB8G8R8Uscaled:
				return "VK_FORMAT_B8G8R8_USCALED";
			case vk::Format::eB8G8R8Sscaled:
				return "VK_FORMAT_B8G8R8_SSCALED";
			case vk::Format::eB8G8R8Uint:
				return "VK_FORMAT_B8G8R8_UINT";
			case vk::Format::eB8G8R8Sint:
				return "VK_FORMAT_B8G8R8_SINT";
			case vk::Format::eB8G8R8Srgb:
				return "VK_FORMAT_B8G8R8_SRGB";
			case vk::Format::eR8G8B8A8Unorm:
				return "VK_FORMAT_R8G8B8A8_UNORM";
			case vk::Format::eR8G8B8A8Snorm:
				return "VK_FORMAT_R8G8B8A8_SNORM";
			case vk::Format::eR8G8B8A8Uscaled:
				return "VK_FORMAT_R8G8B8A8_USCALED";
			case vk::Format::eR8G8B8A8Sscaled:
				return "VK_FORMAT_R8G8B8A8_SSCALED";
			case vk::Format::eR8G8B8A8Uint:
				return "VK_FORMAT_R8G8B8A8_UINT";
			case vk::Format::eR8G8B8A8Sint:
				return "VK_FORMAT_R8G8B8A8_SINT";
			case vk::Format::eR8G8B8A8Srgb:
				return "VK_FORMAT_R8G8B8A8_SRGB";
			case vk::Format::eB8G8R8A8Unorm:
				return "VK_FORMAT_B8G8R8A8_UNORM";
			case vk::Format::eB8G8R8A8Snorm:
				return "VK_FORMAT_B8G8R8A8_SNORM";
			case vk::Format::eB8G8R8A8Uscaled:
				return "VK_FORMAT_B8G8R8A8_USCALED";
			case vk::Format::eB8G8R8A8Sscaled:
				return "VK_FORMAT_B8G8R8A8_SSCALED";
			case vk::Format::eB8G8R8A8Uint:
				return "VK_FORMAT_B8G8R8A8_UINT";
			case vk::Format::eB8G8R8A8Sint:
				return "VK_FORMAT_B8G8R8A8_SINT";
			case vk::Format::eB8G8R8A8Srgb:
				return "VK_FORMAT_B8G8R8A8_SRGB";
			case vk::Format::eA8B8G8R8UnormPack32:
				return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
			case vk::Format::eA8B8G8R8SnormPack32:
				return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
			case vk::Format::eA8B8G8R8UscaledPack32:
				return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
			case vk::Format::eA8B8G8R8SscaledPack32:
				return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
			case vk::Format::eA8B8G8R8UintPack32:
				return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
			case vk::Format::eA8B8G8R8SintPack32:
				return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
			case vk::Format::eA8B8G8R8SrgbPack32:
				return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
			case vk::Format::eA2R10G10B10UnormPack32:
				return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
			case vk::Format::eA2R10G10B10SnormPack32:
				return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
			case vk::Format::eA2R10G10B10UscaledPack32:
				return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
			case vk::Format::eA2R10G10B10SscaledPack32:
				return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
			case vk::Format::eA2R10G10B10UintPack32:
				return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
			case vk::Format::eA2R10G10B10SintPack32:
				return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
			case vk::Format::eA2B10G10R10UnormPack32:
				return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
			case vk::Format::eA2B10G10R10SnormPack32:
				return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
			case vk::Format::eA2B10G10R10UscaledPack32:
				return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
			case vk::Format::eA2B10G10R10SscaledPack32:
				return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
			case vk::Format::eA2B10G10R10UintPack32:
				return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
			case vk::Format::eA2B10G10R10SintPack32:
				return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
			case vk::Format::eR16Unorm:
				return "VK_FORMAT_R16_UNORM";
			case vk::Format::eR16Snorm:
				return "VK_FORMAT_R16_SNORM";
			case vk::Format::eR16Uscaled:
				return "VK_FORMAT_R16_USCALED";
			case vk::Format::eR16Sscaled:
				return "VK_FORMAT_R16_SSCALED";
			case vk::Format::eR16Uint:
				return "VK_FORMAT_R16_UINT";
			case vk::Format::eR16Sint:
				return "VK_FORMAT_R16_SINT";
			case vk::Format::eR16Sfloat:
				return "VK_FORMAT_R16_SFLOAT";
			case vk::Format::eR16G16Unorm:
				return "VK_FORMAT_R16G16_UNORM";
			case vk::Format::eR16G16Snorm:
				return "VK_FORMAT_R16G16_SNORM";
			case vk::Format::eR16G16Uscaled:
				return "VK_FORMAT_R16G16_USCALED";
			case vk::Format::eR16G16Sscaled:
				return "VK_FORMAT_R16G16_SSCALED";
			case vk::Format::eR16G16Uint:
				return "VK_FORMAT_R16G16_UINT";
			case vk::Format::eR16G16Sint:
				return "VK_FORMAT_R16G16_SINT";
			case vk::Format::eR16G16Sfloat:
				return "VK_FORMAT_R16G16_SFLOAT";
			case vk::Format::eR16G16B16Unorm:
				return "VK_FORMAT_R16G16B16_UNORM";
			case vk::Format::eR16G16B16Snorm:
				return "VK_FORMAT_R16G16B16_SNORM";
			case vk::Format::eR16G16B16Uscaled:
				return "VK_FORMAT_R16G16B16_USCALED";
			case vk::Format::eR16G16B16Sscaled:
				return "VK_FORMAT_R16G16B16_SSCALED";
			case vk::Format::eR16G16B16Uint:
				return "VK_FORMAT_R16G16B16_UINT";
			case vk::Format::eR16G16B16Sint:
				return "VK_FORMAT_R16G16B16_SINT";
			case vk::Format::eR16G16B16Sfloat:
				return "VK_FORMAT_R16G16B16_SFLOAT";
			case vk::Format::eR16G16B16A16Unorm:
				return "VK_FORMAT_R16G16B16A16_UNORM";
			case vk::Format::eR16G16B16A16Snorm:
				return "VK_FORMAT_R16G16B16A16_SNORM";
			case vk::Format::eR16G16B16A16Uscaled:
				return "VK_FORMAT_R16G16B16A16_USCALED";
			case vk::Format::eR16G16B16A16Sscaled:
				return "VK_FORMAT_R16G16B16A16_SSCALED";
			case vk::Format::eR16G16B16A16Uint:
				return "VK_FORMAT_R16G16B16A16_UINT";
			case vk::Format::eR16G16B16A16Sint:
				return "VK_FORMAT_R16G16B16A16_SINT";
			case vk::Format::eR16G16B16A16Sfloat:
				return "VK_FORMAT_R16G16B16A16_SFLOAT";
			case vk::Format::eR32Uint:
				return "VK_FORMAT_R32_UINT";
			case vk::Format::eR32Sint:
				return "VK_FORMAT_R32_SINT";
			case vk::Format::eR32Sfloat:
				return "VK_FORMAT_R32_SFLOAT";
			case vk::Format::eR32G32Uint:
				return "VK_FORMAT_R32G32_UINT";
			case vk::Format::eR32G32Sint:
				return "VK_FORMAT_R32G32_SINT";
			case vk::Format::eR32G32Sfloat:
				return "VK_FORMAT_R32G32_SFLOAT";
			case vk::Format::eR32G32B32Uint:
				return "VK_FORMAT_R32G32B32_UINT";
			case vk::Format::eR32G32B32Sint:
				return "VK_FORMAT_R32G32B32_SINT";
			case vk::Format::eR32G32B32Sfloat:
				return "VK_FORMAT_R32G32B32_SFLOAT";
			case vk::Format::eR32G32B32A32Uint:
				return "VK_FORMAT_R32G32B32A32_UINT";
			case vk::Format::eR32G32B32A32Sint:
				return "VK_FORMAT_R32G32B32A32_SINT";
			case vk::Format::eR32G32B32A32Sfloat:
				return "VK_FORMAT_R32G32B32A32_SFLOAT";
			case vk::Format::eR64Uint:
				return "VK_FORMAT_R64_UINT";
			case vk::Format::eR64Sint:
				return "VK_FORMAT_R64_SINT";
			case vk::Format::eR64Sfloat:
				return "VK_FORMAT_R64_SFLOAT";
			case vk::Format::eR64G64Uint:
				return "VK_FORMAT_R64G64_UINT";
			case vk::Format::eR64G64Sint:
				return "VK_FORMAT_R64G64_SINT";
			case vk::Format::eR64G64Sfloat:
				return "VK_FORMAT_R64G64_SFLOAT";
			case vk::Format::eR64G64B64Uint:
				return "VK_FORMAT_R64G64B64_UINT";
			case vk::Format::eR64G64B64Sint:
				return "VK_FORMAT_R64G64B64_SINT";
			case vk::Format::eR64G64B64Sfloat:
				return "VK_FORMAT_R64G64B64_SFLOAT";
			case vk::Format::eR64G64B64A64Uint:
				return "VK_FORMAT_R64G64B64A64_UINT";
			case vk::Format::eR64G64B64A64Sint:
				return "VK_FORMAT_R64G64B64A64_SINT";
			case vk::Format::eR64G64B64A64Sfloat:
				return "VK_FORMAT_R64G64B64A64_SFLOAT";
			case vk::Format::eB10G11R11UfloatPack32:
				return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
			case vk::Format::eE5B9G9R9UfloatPack32:
				return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
			case vk::Format::eD16Unorm:
				return "VK_FORMAT_D16_UNORM";
			case vk::Format::eX8D24UnormPack32:
				return "VK_FORMAT_X8_D24_UNORM_PACK32";
			case vk::Format::eD32Sfloat:
				return "VK_FORMAT_D32_SFLOAT";
			case vk::Format::eS8Uint:
				return "VK_FORMAT_S8_UINT";
			case vk::Format::eD16UnormS8Uint:
				return "VK_FORMAT_D16_UNORM_S8_UINT";
			case vk::Format::eD24UnormS8Uint:
				return "VK_FORMAT_D24_UNORM_S8_UINT";
			case vk::Format::eD32SfloatS8Uint:
				return "VK_FORMAT_D32_SFLOAT_S8_UINT";
			case vk::Format::eBc1RgbUnormBlock:
				return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
			case vk::Format::eBc1RgbSrgbBlock:
				return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
			case vk::Format::eBc1RgbaUnormBlock:
				return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
			case vk::Format::eBc1RgbaSrgbBlock:
				return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
			case vk::Format::eBc2UnormBlock:
				return "VK_FORMAT_BC2_UNORM_BLOCK";
			case vk::Format::eBc2SrgbBlock:
				return "VK_FORMAT_BC2_SRGB_BLOCK";
			case vk::Format::eBc3UnormBlock:
				return "VK_FORMAT_BC3_UNORM_BLOCK";
			case vk::Format::eBc3SrgbBlock:
				return "VK_FORMAT_BC3_SRGB_BLOCK";
			case vk::Format::eBc4UnormBlock:
				return "VK_FORMAT_BC4_UNORM_BLOCK";
			case vk::Format::eBc4SnormBlock:
				return "VK_FORMAT_BC4_SNORM_BLOCK";
			case vk::Format::eBc5UnormBlock:
				return "VK_FORMAT_BC5_UNORM_BLOCK";
			case vk::Format::eBc5SnormBlock:
				return "VK_FORMAT_BC5_SNORM_BLOCK";
			case vk::Format::eBc6HUfloatBlock:
				return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
			case vk::Format::eBc6HSfloatBlock:
				return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
			case vk::Format::eBc7UnormBlock:
				return "VK_FORMAT_BC7_UNORM_BLOCK";
			case vk::Format::eBc7SrgbBlock:
				return "VK_FORMAT_BC7_SRGB_BLOCK";
			case vk::Format::eEtc2R8G8B8UnormBlock:
				return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
			case vk::Format::eEtc2R8G8B8SrgbBlock:
				return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
			case vk::Format::eEtc2R8G8B8A1UnormBlock:
				return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOK ";
			case vk::Format::eEtc2R8G8B8A1SrgbBlock:
				return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
			case vk::Format::eEtc2R8G8B8A8UnormBlock:
				return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
			case vk::Format::eEtc2R8G8B8A8SrgbBlock:
				return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
			case vk::Format::eEacR11UnormBlock:
				return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
			case vk::Format::eEacR11SnormBlock:
				return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
			case vk::Format::eEacR11G11UnormBlock:
				return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
			case vk::Format::eEacR11G11SnormBlock:
				return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
			case vk::Format::eAstc4x4UnormBlock:
				return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
			case vk::Format::eAstc4x4SrgbBlock:
				return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
			case vk::Format::eAstc5x4UnormBlock:
				return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
			case vk::Format::eAstc5x4SrgbBlock:
				return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
			case vk::Format::eAstc5x5UnormBlock:
				return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
			case vk::Format::eAstc5x5SrgbBlock:
				return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
			case vk::Format::eAstc6x5UnormBlock:
				return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
			case vk::Format::eAstc6x5SrgbBlock:
				return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
			case vk::Format::eAstc6x6UnormBlock:
				return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
			case vk::Format::eAstc6x6SrgbBlock:
				return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
			case vk::Format::eAstc8x5UnormBlock:
				return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
			case vk::Format::eAstc8x5SrgbBlock:
				return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
			case vk::Format::eAstc8x6UnormBlock:
				return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
			case vk::Format::eAstc8x6SrgbBlock:
				return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
			case vk::Format::eAstc8x8UnormBlock:
				return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
			case vk::Format::eAstc8x8SrgbBlock:
				return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
			case vk::Format::eAstc10x5UnormBlock:
				return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
			case vk::Format::eAstc10x5SrgbBlock:
				return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
			case vk::Format::eAstc10x6UnormBlock:
				return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
			case vk::Format::eAstc10x6SrgbBlock:
				return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
			case vk::Format::eAstc10x8UnormBlock:
				return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
			case vk::Format::eAstc10x8SrgbBlock:
				return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
			case vk::Format::eAstc10x10UnormBlock:
				return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
			case vk::Format::eAstc10x10SrgbBlock:
				return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
			case vk::Format::eAstc12x10UnormBlock:
				return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
			case vk::Format::eAstc12x10SrgbBlock:
				return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
			case vk::Format::eAstc12x12UnormBlock:
				return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
			case vk::Format::eAstc12x12SrgbBlock:
				return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
			case vk::Format::eG8B8G8R8422Unorm:
				return "VK_FORMAT_G8B8G8R8_422_UNORM";
			case vk::Format::eB8G8R8G8422Unorm:
				return "VK_FORMAT_B8G8R8G8_422_UNORM";
			case vk::Format::eG8B8R83Plane420Unorm:
				return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
			case vk::Format::eG8B8R82Plane420Unorm:
				return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
			case vk::Format::eG8B8R83Plane422Unorm:
				return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
			case vk::Format::eG8B8R82Plane422Unorm:
				return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
			case vk::Format::eG8B8R83Plane444Unorm:
				return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
			case vk::Format::eR10X6UnormPack16:
				return "VK_FORMAT_R10X6_UNORM_PACK16";
			case vk::Format::eR10X6G10X6Unorm2Pack16:
				return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
			case vk::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16:
				return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
			case vk::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16:
				return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
			case vk::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16:
				return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
			case vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16:
				return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
			case vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16:
				return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
			case vk::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16:
				return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
			case vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16:
				return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
			case vk::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16:
				return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
			case vk::Format::eR12X4UnormPack16:
				return "VK_FORMAT_R12X4_UNORM_PACK16";
			case vk::Format::eR12X4G12X4Unorm2Pack16:
				return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
			case vk::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16:
				return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
			case vk::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16:
				return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
			case vk::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16:
				return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
			case vk::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16:
				return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
			case vk::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16:
				return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
			case vk::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16:
				return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
			case vk::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16:
				return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
			case vk::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16:
				return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
			case vk::Format::eG16B16G16R16422Unorm:
				return "VK_FORMAT_G16B16G16R16_422_UNORM";
			case vk::Format::eB16G16R16G16422Unorm:
				return "VK_FORMAT_B16G16R16G16_422_UNORM";
			case vk::Format::eG16B16R163Plane420Unorm:
				return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
			case vk::Format::eG16B16R162Plane420Unorm:
				return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
			case vk::Format::eG16B16R163Plane422Unorm:
				return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
			case vk::Format::eG16B16R162Plane422Unorm:
				return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
			case vk::Format::eG16B16R163Plane444Unorm:
				return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
			case vk::Format::eG8B8R82Plane444Unorm:
				return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
			case vk::Format::eG10X6B10X6R10X62Plane444Unorm3Pack16:
				return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
			case vk::Format::eG12X4B12X4R12X42Plane444Unorm3Pack16:
				return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
			case vk::Format::eG16B16R162Plane444Unorm:
				return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
			case vk::Format::eA4R4G4B4UnormPack16:
				return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
			case vk::Format::eA4B4G4R4UnormPack16:
				return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
			case vk::Format::eAstc4x4SfloatBlock:
				return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
			case vk::Format::eAstc5x4SfloatBlock:
				return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
			case vk::Format::eAstc5x5SfloatBlock:
				return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
			case vk::Format::eAstc6x5SfloatBlock:
				return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
			case vk::Format::eAstc6x6SfloatBlock:
				return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
			case vk::Format::eAstc8x5SfloatBlock:
				return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
			case vk::Format::eAstc8x6SfloatBlock:
				return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
			case vk::Format::eAstc8x8SfloatBlock:
				return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
			case vk::Format::eAstc10x5SfloatBlock:
				return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
			case vk::Format::eAstc10x6SfloatBlock:
				return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
			case vk::Format::eAstc10x8SfloatBlock:
				return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
			case vk::Format::eAstc10x10SfloatBlock:
				return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
			case vk::Format::eAstc12x10SfloatBlock:
				return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
			case vk::Format::eAstc12x12SfloatBlock:
				return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
			case vk::Format::ePvrtc12BppUnormBlockIMG:
				return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
			case vk::Format::ePvrtc14BppUnormBlockIMG:
				return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
			case vk::Format::ePvrtc22BppUnormBlockIMG:
				return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
			case vk::Format::ePvrtc24BppUnormBlockIMG:
				return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
			case vk::Format::ePvrtc12BppSrgbBlockIMG:
				return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
			case vk::Format::ePvrtc14BppSrgbBlockIMG:
				return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
			case vk::Format::ePvrtc22BppSrgbBlockIMG:
				return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
			case vk::Format::ePvrtc24BppSrgbBlockIMG:
				return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
			case vk::Format::eR16G16S105NV:
				return "VK_FORMAT_R16G16_S10_5_NV";
			case vk::Format::eA1B5G5R5UnormPack16KHR:
				return "VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR";
			case vk::Format::eA8UnormKHR:
				return "VK_FORMAT_A8_UNORM_KHR";
			case vk::Format::eUndefined:
				return "VK_FORMAT_UNDEFINED";
			default:
				return "VK_FORMAT_INVALID";
			}
		}

		const std::string toString(vk::PresentModeKHR present_mode) {
			switch (present_mode) {
			case vk::PresentModeKHR::eMailbox:
				return "VK_PRESENT_MODE_MAILBOX_KHR";
			case vk::PresentModeKHR::eImmediate:
				return "VK_PRESENT_MODE_IMMEDIATE_KHR";
			case vk::PresentModeKHR::eFifo:
				return "VK_PRESENT_MODE_FIFO_KHR";
			case vk::PresentModeKHR::eFifoRelaxed:
				return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
			case vk::PresentModeKHR::eSharedContinuousRefresh:
				return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
			case vk::PresentModeKHR::eSharedDemandRefresh:
				return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
			default:
				return "UNKNOWN_PRESENT_MODE";
			}
		}

		const std::string toString(vk::Result result) {
			switch (result) {

#define STR(r)   \
		case vk::Result::e##r: \
			return #r
				STR(NotReady);
				STR(Timeout);
				STR(EventSet);
				STR(EventReset);
				STR(Incomplete);
				STR(ErrorOutOfHostMemory);
				STR(ErrorOutOfDeviceMemory);
				STR(ErrorInitializationFailed);
				STR(ErrorDeviceLost);
				STR(ErrorMemoryMapFailed);
				STR(ErrorLayerNotPresent);
				STR(ErrorExtensionNotPresent);
				STR(ErrorFeatureNotPresent);
				STR(ErrorIncompatibleDriver);
				STR(ErrorTooManyObjects);
				STR(ErrorFormatNotSupported);
				STR(ErrorFragmentedPool);
				STR(ErrorUnknown);
				STR(ErrorOutOfPoolMemory);
				STR(ErrorInvalidExternalHandle);
				STR(ErrorFragmentation);
				STR(ErrorInvalidOpaqueCaptureAddress);
				STR(PipelineCompileRequired);
				STR(ErrorSurfaceLostKHR);
				STR(ErrorNativeWindowInUseKHR);
				STR(SuboptimalKHR);
				STR(ErrorOutOfDateKHR);
				STR(ErrorIncompatibleDisplayKHR);
				STR(ErrorValidationFailedEXT);
				STR(ErrorInvalidShaderNV);
#undef STR
			default:
				return "UNKNOWN_ERROR";
			}
		}

		const std::string toString(vk::SurfaceTransformFlagBitsKHR transform_flag) {
			switch (transform_flag) {
			case vk::SurfaceTransformFlagBitsKHR::eIdentity:
				return "SURFACE_TRANSFORM_IDENTITY";
			case vk::SurfaceTransformFlagBitsKHR::eRotate90:
				return "SURFACE_TRANSFORM_ROTATE_90";
			case vk::SurfaceTransformFlagBitsKHR::eRotate180:
				return "SURFACE_TRANSFORM_ROTATE_180";
			case vk::SurfaceTransformFlagBitsKHR::eRotate270:
				return "SURFACE_TRANSFORM_ROTATE_270";
			case vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror:
				return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR";
			case vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90:
				return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90";
			case vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180:
				return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180";
			case vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270:
				return "SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270";
			case vk::SurfaceTransformFlagBitsKHR::eInherit:
				return "SURFACE_TRANSFORM_INHERIT";
			default:
				return "[Unknown transform flag]";
			}
		}

		const std::string toString(vk::SurfaceFormatKHR surface_format) {
			std::string surface_format_string = toString(surface_format.format) + ", ";

			switch (surface_format.colorSpace) {
			case vk::ColorSpaceKHR::eSrgbNonlinear:
				surface_format_string += "VK_COLORSPACE_SRGB_NONLINEAR_KHR";
				break;
			default:
				surface_format_string += "UNKNOWN COLOR SPACE";
			}
			return surface_format_string;
		}

		const std::string toString(vk::CompositeAlphaFlagBitsKHR composite_alpha) {
			switch (composite_alpha) {
			case vk::CompositeAlphaFlagBitsKHR::eOpaque:
				return "VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR";
			case vk::CompositeAlphaFlagBitsKHR::ePreMultiplied:
				return "VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR";
			case vk::CompositeAlphaFlagBitsKHR::ePostMultiplied:
				return "VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR";
			case vk::CompositeAlphaFlagBitsKHR::eInherit:
				return "VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR";
			default:
				return "UNKNOWN COMPOSITE ALPHA FLAG";
			}
		}

		const std::string toString(vk::ImageUsageFlagBits image_usage) {
			switch (image_usage) {
			case vk::ImageUsageFlagBits::eTransferSrc:
				return "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
			case vk::ImageUsageFlagBits::eTransferDst:
				return "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
			case vk::ImageUsageFlagBits::eSampled:
				return "VK_IMAGE_USAGE_SAMPLED_BIT";
			case vk::ImageUsageFlagBits::eStorage:
				return "VK_IMAGE_USAGE_STORAGE_BIT";
			case vk::ImageUsageFlagBits::eColorAttachment:
				return "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
			case vk::ImageUsageFlagBits::eDepthStencilAttachment:
				return "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
			case vk::ImageUsageFlagBits::eTransientAttachment:
				return "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
			case vk::ImageUsageFlagBits::eInputAttachment:
				return "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
			default:
				return "UNKNOWN IMAGE USAGE FLAG";
			}
		}

		const std::string toString(vk::Extent2D extent) {
			return fmt::format("{}x{}", extent.width, extent.height);
		}

		const std::string toString(vk::SampleCountFlagBits flags) {
			std::string result{ "" };
			bool append = false;
			if (flags & vk::SampleCountFlagBits::e1) {
				result += "1";
				append = true;
			}
			if (flags & vk::SampleCountFlagBits::e2) {
				result += append ? "/" : "";
				result += "2";
				append = true;
			}
			if (flags & vk::SampleCountFlagBits::e4) {
				result += append ? "/" : "";
				result += "4";
				append = true;
			}
			if (flags & vk::SampleCountFlagBits::e8) {
				result += append ? "/" : "";
				result += "8";
				append = true;
			}
			if (flags & vk::SampleCountFlagBits::e16) {
				result += append ? "/" : "";
				result += "16";
				append = true;
			}
			if (flags & vk::SampleCountFlagBits::e32) {
				result += append ? "/" : "";
				result += "32";
				append = true;
			}
			if (flags & vk::SampleCountFlagBits::e64) {
				result += append ? "/" : "";
				result += "64";
			}
			return result;
		}

		const std::string toString(vk::PhysicalDeviceType type) {
			switch (type) {
			case vk::PhysicalDeviceType::eOther:
				return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
			case vk::PhysicalDeviceType::eIntegratedGpu:
				return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
			case vk::PhysicalDeviceType::eDiscreteGpu:
				return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
			case vk::PhysicalDeviceType::eVirtualGpu:
				return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
			case vk::PhysicalDeviceType::eCpu:
				return "VK_PHYSICAL_DEVICE_TYPE_CPU";
			default:
				return "UNKNOWN_DEVICE_TYPE";
			}
		}

		const std::string toString(vk::ImageTiling tiling) {
			switch (tiling) {
			case vk::ImageTiling::eOptimal:
				return "VK_IMAGE_TILING_OPTIMAL";
			case vk::ImageTiling::eLinear:
				return "VK_IMAGE_TILING_LINEAR";
			default:
				return "UNKOWN_TILING_METHOD";
			}
		}

		const std::string toString(vk::ImageType type) {
			switch (type) {
			case vk::ImageType::e1D:
				return "VK_IMAGE_TYPE_1D";
			case vk::ImageType::e2D:
				return "VK_IMAGE_TYPE_2D";
			case vk::ImageType::e3D:
				return "VK_IMAGE_TYPE_3D";
			default:
				return "UNKOWN_IMAGE_TYPE";
			}
		}

		const std::string toString(vk::BlendFactor blend) {
			switch (blend) {
			case vk::BlendFactor::eZero:
				return "VK_BLEND_FACTOR_ZERO";
			case vk::BlendFactor::eOne:
				return "VK_BLEND_FACTOR_ONE";
			case vk::BlendFactor::eSrcColor:
				return "VK_BLEND_FACTOR_SRC_COLOR";
			case vk::BlendFactor::eOneMinusSrcColor:
				return "VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR";
			case vk::BlendFactor::eDstColor:
				return "VK_BLEND_FACTOR_DST_COLOR";
			case vk::BlendFactor::eOneMinusDstColor:
				return "VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR";
			case vk::BlendFactor::eSrcAlpha:
				return "VK_BLEND_FACTOR_SRC_ALPHA";
			case vk::BlendFactor::eOneMinusSrcAlpha:
				return "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA";
			case vk::BlendFactor::eDstAlpha:
				return "VK_BLEND_FACTOR_DST_ALPHA";
			case vk::BlendFactor::eOneMinusDstAlpha:
				return "VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA";
			case vk::BlendFactor::eConstantColor:
				return "VK_BLEND_FACTOR_CONSTANT_COLOR";
			case vk::BlendFactor::eOneMinusConstantColor:
				return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR";
			case vk::BlendFactor::eConstantAlpha:
				return "VK_BLEND_FACTOR_CONSTANT_ALPHA";
			case vk::BlendFactor::eOneMinusConstantAlpha:
				return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA";
			case vk::BlendFactor::eSrcAlphaSaturate:
				return "VK_BLEND_FACTOR_SRC_ALPHA_SATURATE";
			case vk::BlendFactor::eSrc1Color:
				return "VK_BLEND_FACTOR_SRC1_COLOR";
			case vk::BlendFactor::eOneMinusSrc1Color:
				return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR";
			case vk::BlendFactor::eSrc1Alpha:
				return "VK_BLEND_FACTOR_SRC1_ALPHA";
			case vk::BlendFactor::eOneMinusSrc1Alpha:
				return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA";
			default:
				return "Unknown Blend Factor";
			}
		}

		const std::string toString(vk::VertexInputRate rate) {
			switch (rate) {
			case vk::VertexInputRate::eVertex:
				return "VK_VERTEX_INPUT_RATE_VERTEX";
			case vk::VertexInputRate::eInstance:
				return "VK_VERTEX_INPUT_RATE_INSTANCE";
			default:
				return "Unknown Rate";
			}
		}

		const std::string toString(vk::Bool32 state) {
			if (state == VK_TRUE) {
				return "true";
			}
			return "false";
		}

		const std::string toString(vk::PrimitiveTopology topology) {
			switch (topology) {
			case vk::PrimitiveTopology::ePointList:
				return "VK_PRIMITIVE_TOPOLOGY_POINT_LIST";
			case vk::PrimitiveTopology::eLineList:
				return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST";
			case vk::PrimitiveTopology::eLineStrip:
				return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP";
			case vk::PrimitiveTopology::eTriangleList:
				return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
			case vk::PrimitiveTopology::eTriangleStrip:
				return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP";
			case vk::PrimitiveTopology::eTriangleFan:
				return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN";
			case vk::PrimitiveTopology::eLineListWithAdjacency:
				return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY";
			case vk::PrimitiveTopology::eLineStripWithAdjacency:
				return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY";
			case vk::PrimitiveTopology::eTriangleListWithAdjacency:
				return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY";
			case vk::PrimitiveTopology::eTriangleStripWithAdjacency:
				return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY";
			case vk::PrimitiveTopology::ePatchList:
				return "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST";
			default:
				return "UNKOWN TOPOLOGY";
			}
		}

		const std::string toString(vk::FrontFace face) {
			switch (face) {
			case vk::FrontFace::eCounterClockwise:
				return "VK_FRONT_FACE_COUNTER_CLOCKWISE";
			case vk::FrontFace::eClockwise:
				return "VK_FRONT_FACE_CLOCKWISE";
			default:
				return "UNKOWN";
			}
		}

		const std::string toString(vk::PolygonMode mode) {
			switch (mode) {
			case vk::PolygonMode::eFill:
				return "VK_POLYGON_MODE_FILL";
			case vk::PolygonMode::eLine:
				return "VK_POLYGON_MODE_LINE";
			case vk::PolygonMode::ePoint:
				return "VK_POLYGON_MODE_POINT";
			default:
				return "UNKOWN";
			}
		}

		const std::string toString(vk::CompareOp operation) {
			switch (operation) {
			case vk::CompareOp::eNever:
				return "NEVER";
			case vk::CompareOp::eLess:
				return "LESS";
			case vk::CompareOp::eEqual:
				return "EQUAL";
			case vk::CompareOp::eLessOrEqual:
				return "LESS_OR_EQUAL";
			case vk::CompareOp::eGreater:
				return "GREATER";
			case vk::CompareOp::eNotEqual:
				return "NOT_EQUAL";
			case vk::CompareOp::eGreaterOrEqual:
				return "GREATER_OR_EQUAL";
			case vk::CompareOp::eAlways:
				return "ALWAYS";
			default:
				return "Unkown";
			}
		}

		const std::string toString(vk::StencilOp operation) {
			switch (operation) {
			case vk::StencilOp::eKeep:
				return "KEEP";
			case vk::StencilOp::eZero:
				return "ZERO";
			case vk::StencilOp::eReplace:
				return "REPLACE";
			case vk::StencilOp::eIncrementAndClamp:
				return "INCREMENT_AND_CLAMP";
			case vk::StencilOp::eDecrementAndClamp:
				return "DECREMENT_AND_CLAMP";
			case vk::StencilOp::eInvert:
				return "INVERT";
			case vk::StencilOp::eIncrementAndWrap:
				return "INCREMENT_AND_WRAP";
			case vk::StencilOp::eDecrementAndWrap:
				return "DECREMENT_AND_WRAP";
			default:
				return "Unkown";
			}
		}

		const std::string toString(vk::LogicOp operation) {
			switch (operation) {
			case vk::LogicOp::eClear:
				return "CLEAR";
			case vk::LogicOp::eAnd:
				return "AND";
			case vk::LogicOp::eAndReverse:
				return "AND_REVERSE";
			case vk::LogicOp::eCopy:
				return "COPY";
			case vk::LogicOp::eAndInverted:
				return "AND_INVERTED";
			case vk::LogicOp::eNoOp:
				return "NO_OP";
			case vk::LogicOp::eXor:
				return "XOR";
			case vk::LogicOp::eOr:
				return "OR";
			case vk::LogicOp::eNor:
				return "NOR";
			case vk::LogicOp::eEquivalent:
				return "EQUIVALENT";
			case vk::LogicOp::eInvert:
				return "INVERT";
			case vk::LogicOp::eOrReverse:
				return "OR_REVERSE";
			case vk::LogicOp::eCopyInverted:
				return "COPY_INVERTED";
			case vk::LogicOp::eOrInverted:
				return "OR_INVERTED";
			case vk::LogicOp::eNand:
				return "NAND";
			case vk::LogicOp::eSet:
				return "SET";
			default:
				return "Unkown";
			}
		}

		const std::string toString(vk::BlendOp operation) {
			switch (operation) {
			case vk::BlendOp::eAdd:
				return "ADD";
			case vk::BlendOp::eSubtract:
				return "SUBTRACT";
			case vk::BlendOp::eReverseSubtract:
				return "REVERSE_SUBTRACT";
			case vk::BlendOp::eMin:
				return "MIN";
			case vk::BlendOp::eMax:
				return "MAX";
			default:
				return "Unkown";
			}
		}

		const std::string toString(scene::AlphaMode mode) {
			if (mode == scene::AlphaMode::Blend) {
				return "Blend";
			}
			else if (mode == scene::AlphaMode::Mask) {
				return "Mask";
			}
			else if (mode == scene::AlphaMode::Opaque) {
				return "Opaque";
			}
			return "Unkown";
		}

		const std::string toString(bool flag) {
			return flag ? "true" : "false";
		}

		const std::string toString(core::ShaderResourceType type) {
			switch (type) {
			case core::ShaderResourceType::Input:
				return "Input";
			case core::ShaderResourceType::InputAttachment:
				return "InputAttachment";
			case core::ShaderResourceType::Output:
				return "Output";
			case core::ShaderResourceType::ImageCPP:
				return "Image";
			case core::ShaderResourceType::ImageCPPSampler:
				return "ImageSampler";
			case core::ShaderResourceType::ImageCPPStorage:
				return "ImageStorage";
			case core::ShaderResourceType::Sampler:
				return "Sampler";
			case core::ShaderResourceType::BufferUniform:
				return "BufferUniform";
			case core::ShaderResourceType::BufferStorage:
				return "BufferStorage";
			case core::ShaderResourceType::PushConstant:
				return "PushConstant";
			case core::ShaderResourceType::SpecializationConstant:
				return "SpecializationConstant";
			default:
				return "Unkown Type";
			}
		}

		const std::string bufferUsageToString(vk::BufferUsageFlags flags) {
			return toString<vk::BufferUsageFlagBits>(static_cast<uint32_t>(flags),
				{ {vk::BufferUsageFlagBits::eTransferSrc, "VK_BUFFER_USAGE_TRANSFER_SRC_BIT"},
				  {vk::BufferUsageFlagBits::eTransferDst, "VK_BUFFER_USAGE_TRANSFER_DST_BIT"},
				  {vk::BufferUsageFlagBits::eUniformTexelBuffer, "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT"},
				  {vk::BufferUsageFlagBits::eStorageTexelBuffer, "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT"},
				  {vk::BufferUsageFlagBits::eUniformBuffer, "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT"},
				  {vk::BufferUsageFlagBits::eStorageBuffer, "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT"},
				  {vk::BufferUsageFlagBits::eIndexBuffer, "VK_BUFFER_USAGE_INDEX_BUFFER_BIT"},
				  {vk::BufferUsageFlagBits::eVertexBuffer, "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT"},
				  {vk::BufferUsageFlagBits::eIndirectBuffer, "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT"},
				  {vk::BufferUsageFlagBits::eShaderDeviceAddress, "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT"} });
		}

		const std::string shaderStageToString(vk::ShaderStageFlags flags) {
			return toString<vk::ShaderStageFlagBits>(static_cast<uint32_t>(flags),
				{ {vk::ShaderStageFlagBits::eTessellationControl, "TESSELLATION_CONTROL"},
				  {vk::ShaderStageFlagBits::eTessellationEvaluation, "TESSELLATION_EVALUATION"},
				  {vk::ShaderStageFlagBits::eGeometry, "GEOMETRY"},
				  {vk::ShaderStageFlagBits::eVertex, "VERTEX"},
				  {vk::ShaderStageFlagBits::eFragment, "FRAGMENT"},
				  {vk::ShaderStageFlagBits::eCompute, "COMPUTE"},
				  {vk::ShaderStageFlagBits::eAllGraphics, "ALL GRAPHICS"} });
		}

		const std::string imageUsageToString(vk::ImageUsageFlags flags) {
			return toString<vk::ImageUsageFlagBits>(static_cast<uint32_t>(flags),
				{ {vk::ImageUsageFlagBits::eTransferSrc, "VK_IMAGE_USAGE_TRANSFER_SRC_BIT"},
				  {vk::ImageUsageFlagBits::eTransferDst, "VK_IMAGE_USAGE_TRANSFER_DST_BIT"},
				  {vk::ImageUsageFlagBits::eSampled, "VK_IMAGE_USAGE_SAMPLED_BIT"},
				  {vk::ImageUsageFlagBits::eStorage, "VK_IMAGE_USAGE_STORAGE_BIT"},
				  {vk::ImageUsageFlagBits::eColorAttachment, "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT"},
				  {vk::ImageUsageFlagBits::eDepthStencilAttachment, "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT"},
				  {vk::ImageUsageFlagBits::eTransientAttachment, "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT"},
				  {vk::ImageUsageFlagBits::eInputAttachment, "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT"} });
		}

		const std::string imageAspectToString(vk::ImageAspectFlags flags) {
			return toString<vk::ImageAspectFlagBits>(static_cast<uint32_t>(flags),
				{ {vk::ImageAspectFlagBits::eColor, "VK_IMAGE_ASPECT_COLOR_BIT"},
				  {vk::ImageAspectFlagBits::eDepth, "VK_IMAGE_ASPECT_DEPTH_BIT"},
				  {vk::ImageAspectFlagBits::eStencil, "VK_IMAGE_ASPECT_STENCIL_BIT"},
				  {vk::ImageAspectFlagBits::eMetadata, "VK_IMAGE_ASPECT_METADATA_BIT"},
				  {vk::ImageAspectFlagBits::ePlane0, "VK_IMAGE_ASPECT_PLANE_0_BIT"},
				  {vk::ImageAspectFlagBits::ePlane1, "VK_IMAGE_ASPECT_PLANE_1_BIT"},
				  {vk::ImageAspectFlagBits::ePlane2, "VK_IMAGE_ASPECT_PLANE_2_BIT"} });
		}

		const std::string cullModeToString(vk::CullModeFlags flags) {
			return toString<vk::CullModeFlagBits>(static_cast<uint32_t>(flags),
				{ {vk::CullModeFlagBits::eNone, "VK_CULL_MODE_NONE"},
				  {vk::CullModeFlagBits::eFront, "VK_CULL_MODE_FRONT_BIT"},
				  {vk::CullModeFlagBits::eBack, "VK_CULL_MODE_BACK_BIT"},
				  {vk::CullModeFlagBits::eFrontAndBack, "VK_CULL_MODE_FRONT_AND_BACK"} });
		}

		const std::string colorComponentToString(vk::ColorComponentFlags flags) {
			return toString<vk::ColorComponentFlagBits>(static_cast<uint32_t>(flags),
				{ {vk::ColorComponentFlagBits::eR, "R"},
				  {vk::ColorComponentFlagBits::eG, "G"},
				  {vk::ColorComponentFlagBits::eB, "B"},
				  {vk::ColorComponentFlagBits::eA, "A"} });
		}

		const std::string imageCompressionFlagsToString(vk::ImageCompressionFlagsEXT flags) {
			if (flags == static_cast<vk::ImageCompressionFlagsEXT>(0)) {
				return "VK_IMAGE_COMPRESSION_DEFAULT_EXT";
			}

			return toString<vk::ImageCompressionFlagBitsEXT>(static_cast<uint32_t>(flags),
				{ {vk::ImageCompressionFlagBitsEXT::eFixedRateDefault, "VK_IMAGE_COMPRESSION_FIXED_RATE_DEFAULT_EXT"},
				  {vk::ImageCompressionFlagBitsEXT::eFixedRateExplicit, "VK_IMAGE_COMPRESSION_FIXED_RATE_EXPLICIT_EXT"},
				  {vk::ImageCompressionFlagBitsEXT::eDisabled, "VK_IMAGE_COMPRESSION_DISABLED_EXT"} });
		}

		const std::string imageCompressionFixedRateFlagsToString(vk::ImageCompressionFixedRateFlagsEXT flags) {
			if (flags == static_cast<vk::ImageCompressionFixedRateFlagsEXT>(0)) {
				return "VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT";
			}

			return toString<vk::ImageCompressionFixedRateFlagBitsEXT>(static_cast<uint32_t>(flags),
				{ {vk::ImageCompressionFixedRateFlagBitsEXT::e1Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_1BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e2Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_2BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e3Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_3BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e4Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_4BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e5Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_5BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e6Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_6BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e7Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_7BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e8Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_8BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e9Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_9BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e10Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_10BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e11Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_11BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e12Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_12BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e13Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_13BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e14Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_14BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e15Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_15BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e16Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_16BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e17Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_17BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e18Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_18BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e19Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_19BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e20Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_20BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e21Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_21BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e22Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_22BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e23Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_23BPC_BIT_EXT"},
				  {vk::ImageCompressionFixedRateFlagBitsEXT::e24Bpc, "VK_IMAGE_COMPRESSION_FIXED_RATE_24BPC_BIT_EXT"} });
		}

		std::vector<std::string> split(const std::string& input, char delim) {
			std::vector<std::string> tokens;

			std::stringstream sstream(input);
			std::string       token;
			while (std::getline(sstream, token, delim)) {
				tokens.push_back(token);
			}

			return tokens;
		}
	}
}
