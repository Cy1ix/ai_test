/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/image.h"
#include "core/device.h"

namespace frame {
	namespace core {
		namespace {
			inline vk::ImageType findImageType(vk::Extent3D const& extent) {
				uint32_t dim_num = !!extent.width + !!extent.height + (1 < extent.depth);
				switch (dim_num) {
				case 1:
					return vk::ImageType::e1D;
				case 2:
					return vk::ImageType::e2D;
				case 3:
					return vk::ImageType::e3D;
				default:
					throw std::runtime_error("No image type found.");
					return vk::ImageType();
				}
			}
		}

		ImageCPP ImageCPPBuilder::build(Device& device) const {
			return ImageCPP(device, *this);
		}

		std::unique_ptr<ImageCPP> ImageCPPBuilder::buildUnique(Device& device) const {
			return std::make_unique<ImageCPP>(device, *this);
		}

		ImageCPP::ImageCPP(Device& device,
			const vk::Extent3D& extent,
			vk::Format format,
			vk::ImageUsageFlags image_usage,
			VmaMemoryUsage memory_usage,
			vk::SampleCountFlagBits sample_count,
			const uint32_t mip_levels,
			const uint32_t array_layers,
			vk::ImageTiling tiling,
			vk::ImageCreateFlags flags,
			uint32_t num_queue_families,
			const uint32_t* queue_families) :
			ImageCPP{ device,
				ImageCPPBuilder{extent}
				.withFormat(format)
				.withMipLevels(mip_levels)
				.withArrayLayers(array_layers)
				.withSampleCount(sample_count)
				.withTiling(tiling)
				.withFlags(flags)
				.withUsage(image_usage)
				.withQueueFamilies(num_queue_families, queue_families)
				.withVmaUsage(memory_usage)}
		{
		}

		ImageCPP::ImageCPP(Device& device, ImageCPPBuilder const& builder) :
			alloc::VmaAllocated<vk::Image>{ builder.getAllocationCreateInfo(), nullptr, &device }, m_create_info{ builder.getCreateInfo() }
		{
			getHandle() = createImage(m_create_info.operator const VkImageCreateInfo & ());
			m_subresource.arrayLayer = m_create_info.arrayLayers;
			m_subresource.mipLevel = m_create_info.mipLevels;
			if (!builder.getDebugName().empty()) {
				setDebugName(builder.getDebugName());
			}
		}

		ImageCPP::ImageCPP(Device& device,
			vk::Image handle,
			const vk::Extent3D& extent,
			vk::Format format,
			vk::ImageUsageFlags image_usage,
			bool is_external,
			vk::SampleCountFlagBits sample_count) :
			VmaAllocated{ handle, &device }
		{
			m_create_info.samples = sample_count;
			m_create_info.format = format;
			m_create_info.usage = image_usage;
			m_create_info.extent = extent;
			m_create_info.imageType = findImageType(extent);
			m_create_info.arrayLayers = 1;
			m_create_info.mipLevels = 1;
			m_subresource.mipLevel = 1;
			m_subresource.arrayLayer = 1;

			m_is_external_resource = is_external;
		}

		ImageCPP::ImageCPP(ImageCPP&& other) noexcept :
			VmaAllocated{ std::move(other) },
			m_create_info(std::exchange(other.m_create_info, {})),
			m_subresource(std::exchange(other.m_subresource, {})),
			m_views(std::exchange(other.m_views, {})),
			m_is_external_resource(std::exchange(other.m_is_external_resource, false))
		{
			for (auto& view : m_views) {
				view->setImage(*this);
			}
		}

		ImageCPP::~ImageCPP() {
			destroyImage(getHandle());
		}

		uint8_t* ImageCPP::map() {
			if (m_create_info.tiling != vk::ImageTiling::eLinear) {
				LOGW("[Image] Mapping image memory that is not linear");
			}
			return alloc::VmaAllocated<vk::Image>::map();
		}

		vk::ImageType ImageCPP::getType() const {
			return m_create_info.imageType;
		}

		const vk::Extent3D& ImageCPP::getExtent() const {
			return m_create_info.extent;
		}

		vk::Format ImageCPP::getFormat() const {
			return m_create_info.format;
		}

		vk::SampleCountFlagBits ImageCPP::getSampleCount() const {
			return m_create_info.samples;
		}

		vk::ImageUsageFlags ImageCPP::getUsage() const {
			return m_create_info.usage;
		}

		vk::ImageTiling ImageCPP::getTiling() const {
			return m_create_info.tiling;
		}

		vk::ImageSubresource ImageCPP::getSubresource() const {
			return m_subresource;
		}

		uint32_t ImageCPP::getArrayLayerCount() const {
			return m_create_info.arrayLayers;
		}

		std::unordered_set<ImageViewCPP*>& ImageCPP::getViews() {
			return m_views;
		}

		void ImageCPP::addViews(ImageViewCPP* image_view) {
			m_views.emplace(image_view);
		}

		const bool ImageCPP::isExternal() const {
			return m_is_external_resource;
		}
	}
}
