/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/image_view.h"
#include "core/image.h"
#include "common/common.h"
#include "core/device.h"
#include "vulkan/vulkan_format_traits.hpp"

namespace frame {
	namespace core {
		ImageViewCPP::ImageViewCPP(ImageCPP& image,
			vk::ImageViewType view_type,
			vk::Format format,
			uint32_t mip_level,
			uint32_t level_count,
			uint32_t array_layer,
			uint32_t layer_count) :
			VulkanResource{VK_NULL_HANDLE, &image.getDevice()},
			m_image(&image), m_format(format)
		{
			if (m_format == vk::Format::eUndefined) {
				m_format = m_image->getFormat();
			}

			m_subresource_range =
				vk::ImageSubresourceRange(
					[this]() -> vk::ImageAspectFlags {
						if (common::isDepthFormat(m_format)) {
							if (common::isDepthStencilFormat(m_format)) {
								return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
							}
							return vk::ImageAspectFlagBits::eDepth;
						}
						return vk::ImageAspectFlagBits::eColor;
					}(),
					mip_level,
					level_count == 0 ? m_image->getSubresource().mipLevel : level_count,
					array_layer,
					layer_count == 0 ? m_image->getSubresource().arrayLayer : layer_count);

			vk::ImageViewCreateInfo image_view_create_info({}, m_image->getHandle(), view_type, m_format, {}, m_subresource_range);

			setHandle(this->getDevice().getHandle().createImageView(image_view_create_info));

			m_image->getViews().emplace(this);
		}

		ImageViewCPP::ImageViewCPP(ImageViewCPP&& other) :
			VulkanResource(nullptr, &(other.getDevice())),
			m_image(other.m_image),
			m_format(other.m_format),
			m_subresource_range(other.m_subresource_range)
		{
			setHandle(other.getHandle());
			auto& views = m_image->getViews();

			views.erase(&other);
			views.emplace(this);

			other.setHandle(nullptr);
		}

		ImageViewCPP::~ImageViewCPP() {
			m_image = nullptr;
			if (hasHandle()) {
				getDevice().getHandle().destroyImageView(getHandle());
			}
		}

		vk::Format ImageViewCPP::getFormat() const { return m_format; }

		const ImageCPP& ImageViewCPP::getImage() const {
			assert(m_image && "[ImageViewCPP] ASSERT: ImageCPP view is referring an invalid image");
			return *m_image;
		}

		void ImageViewCPP::setImage(ImageCPP& img) { m_image = &img; }

		vk::ImageSubresourceLayers ImageViewCPP::getSubresourceLayers() const {
			return vk::ImageSubresourceLayers(
				m_subresource_range.aspectMask,
				m_subresource_range.baseMipLevel,
				m_subresource_range.baseArrayLayer,
				m_subresource_range.layerCount);
		}

		vk::ImageSubresourceRange ImageViewCPP::getSubresourceRange() const { return m_subresource_range; }

	}
}