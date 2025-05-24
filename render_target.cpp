/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/render_target.h"
#include "core/physical_device.h"
#include "core/device.h"
#include "core/image_view.h"

namespace frame {
    namespace rendering {
        const RenderTarget::CreateFunc RenderTarget::DEFAULT_CREATE_FUNC = [](core::ImageCPP&& image) -> std::unique_ptr<RenderTarget> {

            std::vector<core::ImageCPP> images;

            images.push_back(std::move(image));

            return std::make_unique<RenderTarget>(std::move(images));

        };

        const RenderTarget::CreateFunc RenderTarget::CREATE_FUNC = [](core::ImageCPP&& image) -> std::unique_ptr<RenderTarget>
        {
            std::vector<core::ImageCPP> images;
            auto& device = image.getDevice();
            auto extent = image.getExtent();

            // if the image parameter is depth format, it is used for shadow mapping, render target contains only depth image
            if (common::isDepthFormat(image.getFormat())) {
                images.push_back(std::move(image));
                return std::make_unique<RenderTarget>(std::move(images));
            }
            else {
                // depth image
                vk::Format depth_format = common::getSuitableDepthFormat(device.getPhysicalDevice().getHandle());
                core::ImageCPP depth_image{
                    device,
                    extent,
                    depth_format,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                    VMA_MEMORY_USAGE_GPU_ONLY
                };
                images.push_back(std::move(depth_image));

                // original image
                images.emplace_back(std::move(image));

                // albedo image
                core::ImageCPP albedo_image{
                    device,
                    extent,
                    vk::Format::eR8G8B8A8Unorm,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment,
                    VMA_MEMORY_USAGE_GPU_ONLY
                };
                images.push_back(std::move(albedo_image));

                // normal image
                core::ImageCPP normal_image{
                    device,
                    extent,
                    vk::Format::eR16G16B16A16Sfloat,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment,
                    VMA_MEMORY_USAGE_GPU_ONLY
                };
                images.push_back(std::move(normal_image));

                // material image
                core::ImageCPP material_image{
                    device,
                    extent,
                    vk::Format::eR8G8B8A8Unorm,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment,
                    VMA_MEMORY_USAGE_GPU_ONLY
                };
                images.push_back(std::move(material_image));

                // position image
                core::ImageCPP position_image{
                    device,
                    extent,
                    vk::Format::eR16G16B16A16Sfloat,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment,
                    VMA_MEMORY_USAGE_GPU_ONLY
                };
                images.push_back(std::move(position_image));

                // emissive image
                core::ImageCPP emissive_image{
                    device,
                    extent,
                    vk::Format::eR8G8B8A8Unorm,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment,
                    VMA_MEMORY_USAGE_GPU_ONLY
                };
                images.push_back(std::move(emissive_image));

            }

            return std::make_unique<RenderTarget>(std::move(images));
        };
        
        RenderTarget::RenderTarget(std::vector<core::ImageCPP>&& images_) :
            m_device{ images_.back().getDevice() },
            m_images{ std::move(images_) }
        {
            assert(!m_images.empty() && "[RenderTarget] ASSERT: Should specify at least 1 image");
            
            auto it = std::find_if(m_images.begin(), m_images.end(),
                [](core::ImageCPP const& image) { return image.getType() != vk::ImageType::e2D; });

            if (it != m_images.end()) {
                throw std::runtime_error("[RenderTarget] ERROR: ImageCPP type is not 2D");
            }

            m_extent.width = m_images.front().getExtent().width;
            m_extent.height = m_images.front().getExtent().height;
            
            it = std::find_if(std::next(m_images.begin()), m_images.end(),
                [this](core::ImageCPP const& image) {
                    return (m_extent.width != image.getExtent().width) ||
                        (m_extent.height != image.getExtent().height);
                });

            if (it != m_images.end()) {
                throw std::runtime_error("[RenderTarget] ERROR: Extent size is not unique");
            }

            for (auto& image : m_images) {
                m_views.emplace_back(image, vk::ImageViewType::e2D);
                m_attachments.emplace_back(image.getFormat(), image.getSampleCount(), image.getUsage());
            }
        }

        RenderTarget::RenderTarget(std::vector<core::ImageViewCPP>&& image_views) :
            m_device{ image_views.back().getImage().getDevice() },
            m_views{ std::move(image_views) }
        {
            assert(!m_views.empty() && "[RenderTarget] ASSERT: Should specify at least 1 image view");

            const uint32_t mip_level = m_views.front().getSubresourceRange().baseMipLevel;
            m_extent.width = m_views.front().getImage().getExtent().width >> mip_level;
            m_extent.height = m_views.front().getImage().getExtent().height >> mip_level;
            
            auto it = std::find_if(std::next(m_views.begin()), m_views.end(),
                [this](core::ImageViewCPP const& image_view) {
                    const uint32_t mip_level = image_view.getSubresourceRange().baseMipLevel;
                    return (m_extent.width != image_view.getImage().getExtent().width >> mip_level) ||
                        (m_extent.height != image_view.getImage().getExtent().height >> mip_level);
                });
            if (it != m_views.end()) {
                throw std::runtime_error("[RenderTarget] ERROR: Extent size is not unique");
            }

            for (auto& view : m_views) {
                const auto& image = view.getImage();
                m_attachments.emplace_back(Attachment{ image.getFormat(), image.getSampleCount(), image.getUsage() });
            }
        }

        const vk::Extent2D& RenderTarget::getExtent() const {
            return m_extent;
        }

        const std::vector<core::ImageCPP>& RenderTarget::getImages() const {
            return m_images;
        }

        std::vector<core::ImageCPP>& RenderTarget::getImage() {
            return m_images;
        }

        const std::vector<core::ImageViewCPP>& RenderTarget::getViews() const {
            return m_views;
        }
        
        std::vector<core::ImageViewCPP>& RenderTarget::getView() {
            return m_views;
        }

        const std::vector<Attachment>& RenderTarget::getAttachments() const {
            return m_attachments;
        }

        void RenderTarget::setInputAttachments(std::vector<uint32_t>& input) {
            m_input_attachments = input;
        }

        const std::vector<uint32_t>& RenderTarget::getInputAttachments() const {
            return m_input_attachments;
        }

        void RenderTarget::setOutputAttachments(std::vector<uint32_t>& output) {
            m_output_attachments = output;
        }

        const std::vector<uint32_t>& RenderTarget::getOutputAttachments() const {
            return m_output_attachments;
        }

        void RenderTarget::setLayout(uint32_t attachment, vk::ImageLayout layout) {
            m_attachments[attachment].initial_layout = layout;
        }
        
        vk::ImageLayout RenderTarget::getLayout(uint32_t attachment) const {
            return m_attachments[attachment].initial_layout;
        }

        const core::ImageViewCPP& RenderTarget::getDepthView() const {
            if(m_views.size() > 0) {
	            return m_views[0];
            }
            else {
                LOGE("Current render target has no depth view");
                throw std::runtime_error("Views array out of range");
            }
        }

        const core::ImageViewCPP& RenderTarget::getImageView() const {
            if (m_views.size() > 1) {
                return m_views[1];
            }
            else {
                LOGE("Current render target has no image view");
                throw std::runtime_error("Views array out of range");
            }
        }

        const core::ImageViewCPP& RenderTarget::getAlbedoView() const {
            if (m_views.size() > 2) {
                return m_views[2];
            }
            else {
                LOGE("Current render target has no albedo view");
                throw std::runtime_error("Views array out of range");
            }
        }

        const core::ImageViewCPP& RenderTarget::getNormalView() const {
            if (m_views.size() > 3) {
                return m_views[3];
            }
            else {
                LOGE("Current render target has no normal view");
                throw std::runtime_error("Views array out of range");
            }
        }

        const core::ImageViewCPP& RenderTarget::getMaterialView() const {
            if (m_views.size() > 4) {
                return m_views[4];
            }
            else {
                LOGE("Current render target has no material view");
                throw std::runtime_error("Views array out of range");
            }
        }

        const core::ImageViewCPP& RenderTarget::getPositionView() const {
            if (m_views.size() > 5) {
                return m_views[5];
            }
            else {
                LOGE("Current render target has no position view");
                throw std::runtime_error("Views array out of range");
            }
        }

        const core::ImageViewCPP& RenderTarget::getEmissiveView() const {
            if (m_views.size() > 6) {
                return m_views[6];
            }
            else {
                LOGE("Current render target has no emissive view");
                throw std::runtime_error("Views array out of range");
            }
        }
    }
}
