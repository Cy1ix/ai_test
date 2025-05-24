/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scene/components/image/image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>
#include "scene/utils.h"
#include "filesystem/filesystem.h"
#include "scene/components/image/astc.h"
#include "scene/components/image/ktx.h"
#include "scene/components/image/stb.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_format_traits.hpp>

namespace frame {
	namespace scene {
		bool isAstc(const vk::Format format) {
			return strncmp(vk::compressionScheme(format), "ASTC", 4) == 0;
		}

		Image::Image(const std::string& name, std::vector<uint8_t>&& d, std::vector<Mipmap>&& m) :
			Component{ name },
			m_data{ std::move(d) },
			m_format{ vk::Format::eR8G8B8A8Unorm },
			m_mipmaps{ std::move(m) }
		{}

		std::unique_ptr<Image> Image::load(const std::string& name, const std::string& uri,
			ContentType content_type)
		{
			std::unique_ptr<Image> image{ nullptr };
			auto data = filesystem::readAsset(uri);
			auto extension = getExtension(uri);

			if (extension == "png" || extension == "jpg" || extension == "jpeg") {
				image = std::unique_ptr<Image>(std::make_unique<Stb>(name, data, content_type).release());
			}
			else if (extension == "astc") {
				image = std::unique_ptr<Image>(std::make_unique<Astc>(name, data).release());
			}
			else if ((extension == "ktx") || (extension == "ktx2")) {
				image = std::unique_ptr<Image>(std::make_unique<Ktx>(name, data, content_type).release());
			}

			return image;
		}

		std::type_index Image::getType() {
			return typeid(Image);
		}

		void Image::clearData() {
			m_data.clear();
			m_data.shrink_to_fit();
		}

		void Image::coerceFormatToSrgb() {
			switch (m_format) {
			case vk::Format::eR8Unorm:m_format = vk::Format::eR8Srgb; break;
			case vk::Format::eR8G8Unorm:m_format = vk::Format::eR8G8Srgb; break;
			case vk::Format::eR8G8B8Unorm:m_format = vk::Format::eR8G8B8Srgb; break;
			case vk::Format::eB8G8R8Unorm:m_format = vk::Format::eB8G8R8Srgb; break;
			case vk::Format::eR8G8B8A8Unorm:m_format = vk::Format::eR8G8B8A8Srgb; break;
			case vk::Format::eB8G8R8A8Unorm:m_format = vk::Format::eB8G8R8A8Srgb; break;
			case vk::Format::eA8B8G8R8UnormPack32:m_format = vk::Format::eA8B8G8R8SrgbPack32; break;
			case vk::Format::eBc1RgbUnormBlock:m_format = vk::Format::eBc1RgbSrgbBlock; break;
			case vk::Format::eBc1RgbaUnormBlock:m_format = vk::Format::eBc1RgbaSrgbBlock; break;
			case vk::Format::eBc2UnormBlock:m_format = vk::Format::eBc2SrgbBlock; break;
			case vk::Format::eBc3UnormBlock:m_format = vk::Format::eBc3SrgbBlock; break;
			case vk::Format::eBc7UnormBlock:m_format = vk::Format::eBc7SrgbBlock; break;
			case vk::Format::eEtc2R8G8B8UnormBlock:m_format = vk::Format::eEtc2R8G8B8SrgbBlock; break;
			case vk::Format::eEtc2R8G8B8A1UnormBlock:m_format = vk::Format::eEtc2R8G8B8A1SrgbBlock; break;
			case vk::Format::eEtc2R8G8B8A8UnormBlock:m_format = vk::Format::eEtc2R8G8B8A8SrgbBlock; break;
			case vk::Format::eAstc4x4UnormBlock:m_format = vk::Format::eAstc4x4SrgbBlock; break;
			case vk::Format::eAstc5x4UnormBlock:m_format = vk::Format::eAstc5x4SrgbBlock; break;
			case vk::Format::eAstc5x5UnormBlock:m_format = vk::Format::eAstc5x5SrgbBlock; break;
			case vk::Format::eAstc6x5UnormBlock:m_format = vk::Format::eAstc6x5SrgbBlock; break;
			case vk::Format::eAstc6x6UnormBlock:m_format = vk::Format::eAstc6x6SrgbBlock; break;
			case vk::Format::eAstc8x5UnormBlock:m_format = vk::Format::eAstc8x5SrgbBlock; break;
			case vk::Format::eAstc8x6UnormBlock:m_format = vk::Format::eAstc8x6SrgbBlock; break;
			case vk::Format::eAstc8x8UnormBlock:m_format = vk::Format::eAstc8x8SrgbBlock; break;
			case vk::Format::eAstc10x5UnormBlock:m_format = vk::Format::eAstc10x5SrgbBlock; break;
			case vk::Format::eAstc10x6UnormBlock:m_format = vk::Format::eAstc10x6SrgbBlock; break;
			case vk::Format::eAstc10x8UnormBlock:m_format = vk::Format::eAstc10x8SrgbBlock; break;
			case vk::Format::eAstc10x10UnormBlock:m_format = vk::Format::eAstc10x10SrgbBlock; break;
			case vk::Format::eAstc12x10UnormBlock:m_format = vk::Format::eAstc12x10SrgbBlock; break;
			case vk::Format::eAstc12x12UnormBlock:m_format = vk::Format::eAstc12x12SrgbBlock; break;
			case vk::Format::ePvrtc12BppUnormBlockIMG:m_format = vk::Format::ePvrtc12BppSrgbBlockIMG; break;
			case vk::Format::ePvrtc14BppUnormBlockIMG:m_format = vk::Format::ePvrtc14BppSrgbBlockIMG; break;
			case vk::Format::ePvrtc22BppUnormBlockIMG:m_format = vk::Format::ePvrtc22BppSrgbBlockIMG; break;
			case vk::Format::ePvrtc24BppUnormBlockIMG:m_format = vk::Format::ePvrtc24BppSrgbBlockIMG; break;
			default: break;
			}
		}

		void Image::createVkImage(core::Device& device, vk::ImageViewType image_view_type, vk::ImageCreateFlags flags) {

			assert(!m_image && !m_image_view && "Vulkan Image already constructed");

			m_image = std::make_unique<core::ImageCPP>(
				device,
				getExtent(),
				m_format,
				vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
				VMA_MEMORY_USAGE_GPU_ONLY,
				vk::SampleCountFlagBits::e1,
				common::toU32(m_mipmaps.size()),
				m_layers,
				vk::ImageTiling::eOptimal,
				flags);
			m_image->setDebugName(getName());

			m_image_view = std::make_unique<core::ImageViewCPP>(*m_image, image_view_type);
			m_image_view->setDebugName("View on " + getName());
		}

		void Image::generateMipmaps() {

			assert(m_mipmaps.size() == 1 && "Mipmaps already generated");
			if (m_mipmaps.size() > 1) return;

			auto extent = getExtent();
			auto next_width = std::max<uint32_t>(1u, extent.width / 2);
			auto next_height = std::max<uint32_t>(1u, extent.height / 2);
			auto channels = 4;
			auto next_size = next_width * next_height * channels;

			while (true)
			{
				auto old_size = common::toU32(m_data.size());
				m_data.resize(old_size + next_size);

				auto& prev_mipmap = m_mipmaps.back();
				Mipmap next_mipmap{};
				next_mipmap.level = prev_mipmap.level + 1;
				next_mipmap.offset = old_size;
				next_mipmap.extent = vk::Extent3D(next_width, next_height, 1u);

				stbir_resize_uint8(m_data.data() + prev_mipmap.offset, prev_mipmap.extent.width, prev_mipmap.extent.height, 0,
					m_data.data() + next_mipmap.offset, next_mipmap.extent.width, next_mipmap.extent.height, 0, channels);

				m_mipmaps.emplace_back(std::move(next_mipmap));

				next_width = std::max<uint32_t>(1u, next_width / 2);
				next_height = std::max<uint32_t>(1u, next_height / 2);
				next_size = next_width * next_height * channels;

				if (next_width == 1 && next_height == 1) break;
			}
		}

		const std::vector<uint8_t>& Image::getData() const { return m_data; }

		const vk::Extent3D& Image::getExtent() const {
			assert(!m_mipmaps.empty());
			return m_mipmaps[0].extent;
		}

		vk::Format Image::getFormat() const { return m_format; }

		const uint32_t Image::getLayers() const { return m_layers; }

		const std::vector<Mipmap>& Image::getMipmaps() const { return m_mipmaps; }

		const std::vector<std::vector<vk::DeviceSize>>& Image::getOffsets() const { return m_offsets; }

		const core::ImageCPP& Image::getImage() const {
			assert(m_image && "Vulkan Image was not created");
			return *m_image;
		}

		const core::ImageViewCPP& Image::getImageView() const {
			assert(m_image_view && "Vulkan Image view was not created");
			return *m_image_view;
		}

		Mipmap& Image::getMipmap(const size_t index) {
			assert(index < m_mipmaps.size());
			return m_mipmaps[index];
		}

		std::vector<uint8_t>& Image::getMutData() { return m_data; }

		std::vector<Mipmap>& Image::getMutMipmaps() { return m_mipmaps; }

		void Image::setData(const uint8_t* raw_data, size_t size) {
			assert(m_data.empty() && "Image data already set");
			m_data = { raw_data, raw_data + size };
		}

		void Image::setDepth(const uint32_t depth) {
			assert(!m_mipmaps.empty());
			m_mipmaps[0].extent.depth = depth;
		}

		void Image::setFormat(const vk::Format f) { m_format = f; }

		void Image::setHeight(const uint32_t height) {
			assert(!m_mipmaps.empty());
			m_mipmaps[0].extent.height = height;
		}

		void Image::setLayers(uint32_t l) { m_layers = l; }

		void Image::setOffsets(const std::vector<std::vector<vk::DeviceSize>>& o) { m_offsets = o; }

		void Image::setWidth(const uint32_t width) {
			assert(!m_mipmaps.empty());
			m_mipmaps[0].extent.width = width;
		}
	}
}
