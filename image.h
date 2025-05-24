/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "core/device.h"
#include "scene/component.h"
#include <vulkan/vulkan.hpp>

namespace frame {
	namespace scene {
		bool isAstc(vk::Format format);
		
		struct Mipmap {
			uint32_t level = 0;
			uint32_t offset = 0;
			vk::Extent3D extent = { 0, 0, 0 };
		};

		class Image : public Component {
		public:
			Image(const std::string& name, std::vector<uint8_t>&& data = {}, std::vector<Mipmap>&& mipmaps = { {} });
			virtual ~Image() = default;

		public:
			enum ContentType {
				Unknown,
				Color,
				Other
			};

			static std::unique_ptr<Image> load(const std::string& name, const std::string& uri, ContentType content_type);
			
			virtual std::type_index getType() override;

			void clearData();
			void coerceFormatToSrgb();
			void createVkImage(core::Device& device, vk::ImageViewType image_view_type = vk::ImageViewType::e2D, vk::ImageCreateFlags flags = {});
			void generateMipmaps();
			const std::vector<uint8_t>& getData() const;
			const vk::Extent3D& getExtent() const;
			vk::Format getFormat() const;
			const uint32_t getLayers() const;
			const std::vector<Mipmap>& getMipmaps() const;
			const std::vector<std::vector<vk::DeviceSize>>& getOffsets() const;
			const core::ImageCPP& getImage() const;
			const core::ImageViewCPP& getImageView() const;

		protected:
			Mipmap& getMipmap(size_t index);
			std::vector<uint8_t>& getMutData();
			std::vector<Mipmap>& getMutMipmaps();
			void setData(const uint8_t* raw_data, size_t size);
			void setDepth(uint32_t depth);
			void setFormat(vk::Format format);
			void setHeight(uint32_t height);
			void setLayers(uint32_t layers);
			void setOffsets(const std::vector<std::vector<vk::DeviceSize>>& offsets);
			void setWidth(uint32_t width);

		private:
			std::vector<uint8_t> m_data;
			vk::Format m_format = vk::Format::eUndefined;
			uint32_t m_layers = 1;
			std::vector<Mipmap> m_mipmaps{ {} };
			std::vector<std::vector<vk::DeviceSize>> m_offsets;
			std::unique_ptr<core::ImageCPP> m_image;
			std::unique_ptr<core::ImageViewCPP> m_image_view;
		};
	}
}
