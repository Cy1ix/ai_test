/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "common/object_builder.h"
#include "common/allocator.h"
#include "core/vulkan_resource.h"
#include "core/image_view.h"
#include <unordered_set>

namespace frame {
    namespace core {
        class Device;
        class ImageViewCPP;
        class ImageCPP;
		
		struct ImageCPPBuilder : public alloc::ObjectBuilder<ImageCPPBuilder, vk::ImageCreateInfo> {
		private:
			using Parent = ObjectBuilder<ImageCPPBuilder, vk::ImageCreateInfo>;
		public:
			ImageCPPBuilder(vk::Extent3D const& extent) :
				Parent(vk::ImageCreateInfo{ {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, extent, 1, 1 })
			{}

			ImageCPPBuilder(vk::Extent2D const& extent) :
				ImageCPPBuilder(vk::Extent3D{ extent.width, extent.height, 1 })
			{}

			ImageCPPBuilder(uint32_t width, uint32_t height = 1, uint32_t depth = 1) :
				ImageCPPBuilder(vk::Extent3D{ width, height, depth })
			{}

			ImageCPPBuilder& withFormat(vk::Format format) {
				m_create_info.format = format;
				return *this;
			}

			ImageCPPBuilder& withImageCPPType(vk::ImageType type) {
				m_create_info.imageType = type;
				return *this;
			}

			ImageCPPBuilder& withArrayLayers(uint32_t layers) {
				m_create_info.arrayLayers = layers;
				return *this;
			}

			ImageCPPBuilder& withMipLevels(uint32_t levels) {
				m_create_info.mipLevels = levels;
				return *this;
			}

			ImageCPPBuilder& withSampleCount(vk::SampleCountFlagBits sample_count) {
				m_create_info.samples = sample_count;
				return *this;
			}

			ImageCPPBuilder& withTiling(vk::ImageTiling tiling) {
				m_create_info.tiling = tiling;
				return *this;
			}

			ImageCPPBuilder& withUsage(vk::ImageUsageFlags usage) {
				m_create_info.usage = usage;
				return *this;
			}

			ImageCPPBuilder& withFlags(vk::ImageCreateFlags flags) {
				m_create_info.flags = flags;
				return *this;
			}
			
			ImageCPP build(Device& device) const;
			
			std::unique_ptr<ImageCPP> buildUnique(Device& device) const;
		};

        class ImageCPP : public alloc::VmaAllocated<vk::Image> {
        public:
			ImageCPP(Device& device,
				vk::Image handle,
				const vk::Extent3D& extent,
				vk::Format format,
				vk::ImageUsageFlags image_usage,
				bool is_external = false,
                vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

            ImageCPP(Device& device,
                const vk::Extent3D& extent,
                vk::Format format,
                vk::ImageUsageFlags image_usage,
                VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO,
                vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1,
                uint32_t mip_levels = 1,
                uint32_t array_layers = 1,
                vk::ImageTiling tiling = vk::ImageTiling::eOptimal,
                vk::ImageCreateFlags flags = {},
                uint32_t num_queue_families = 0,
                const uint32_t* queue_families = nullptr);

            ImageCPP(Device& device, const ImageCPPBuilder& builder);

            ImageCPP(const ImageCPP&) = delete;
            ImageCPP(ImageCPP&& other) noexcept;
            ImageCPP& operator=(const ImageCPP&) = delete;
            ImageCPP& operator=(ImageCPP&&) = delete;

			~ImageCPP();

            uint8_t* map();

            vk::ImageType getType() const;
            const vk::Extent3D& getExtent() const;
            vk::Format getFormat() const;
            vk::SampleCountFlagBits getSampleCount() const;
            vk::ImageUsageFlags getUsage() const;
            vk::ImageTiling getTiling() const;
            vk::ImageSubresource getSubresource() const;
            uint32_t getArrayLayerCount() const;
            std::unordered_set<ImageViewCPP*>& getViews();
			void addViews(ImageViewCPP* image_view);
			const bool isExternal() const;

        private:
            vk::ImageCreateInfo m_create_info;
            vk::ImageSubresource m_subresource;
            std::unordered_set<ImageViewCPP*> m_views;
			bool m_is_external_resource = false;
        };
    }
}