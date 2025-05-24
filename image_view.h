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

#pragma once

#include "core/vulkan_resource.h"

namespace frame {
	namespace core {

		class ImageCPP;
		class Device;

		class ImageViewCPP : public VulkanResource<vk::ImageView> {
		public:
			ImageViewCPP(ImageCPP& image,
				vk::ImageViewType view_type,
				vk::Format format = vk::Format::eUndefined,
				uint32_t mip_level = 0,
				uint32_t level_count = 0,
				uint32_t array_layer = 0,
				uint32_t layer_count = 0);

			ImageViewCPP(ImageViewCPP&) = delete;
			ImageViewCPP(ImageViewCPP&& other);

			ImageViewCPP& operator=(const ImageViewCPP&) = delete;
			ImageViewCPP& operator=(ImageViewCPP&&) = delete;

			~ImageViewCPP();

			vk::Format getFormat() const;
			ImageCPP const& getImage() const;
			void setImage(ImageCPP& image);
			vk::ImageSubresourceLayers getSubresourceLayers() const;
			vk::ImageSubresourceRange getSubresourceRange() const;

		private:
			ImageCPP* m_image = nullptr;
			vk::Format m_format;
			vk::ImageSubresourceRange m_subresource_range;
		};
	}
}
