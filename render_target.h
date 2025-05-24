/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <functional>
#include <memory>
#include "core/image.h"

namespace frame {
    namespace core {
        class Device;
    }

    namespace rendering {
        struct Attachment {
            Attachment() = default;

            Attachment(vk::Format format, vk::SampleCountFlagBits samples, vk::ImageUsageFlags usage) :
                format{ format },
                samples{ samples },
                usage{ usage }
            {}
            
            vk::Format format = vk::Format::eUndefined;
            vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
            vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
            vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
        };
        
        class RenderTarget {
        public:
            using CreateFunc = std::function<std::unique_ptr<RenderTarget>(core::ImageCPP&&)>;
            
            static const CreateFunc CREATE_FUNC;
            static const CreateFunc DEFAULT_CREATE_FUNC;

            RenderTarget(std::vector<core::ImageCPP>&& images);
            RenderTarget(std::vector<core::ImageViewCPP>&& image_views);

            RenderTarget(const RenderTarget&) = delete;
            RenderTarget(RenderTarget&&) = delete;
            RenderTarget& operator=(const RenderTarget& other) noexcept = delete;
            RenderTarget& operator=(RenderTarget&& other) noexcept = delete;

            const vk::Extent2D& getExtent() const;
            const std::vector<core::ImageCPP>& getImages() const;
            const std::vector<core::ImageViewCPP>& getViews() const;
            std::vector<core::ImageCPP>& getImage();
            std::vector<core::ImageViewCPP>& getView();
            const std::vector<Attachment>& getAttachments() const;
            
            void setInputAttachments(std::vector<uint32_t>& input);
            const std::vector<uint32_t>& getInputAttachments() const;
            
            void setOutputAttachments(std::vector<uint32_t>& output);
            const std::vector<uint32_t>& getOutputAttachments() const;

            void setLayout(uint32_t attachment, vk::ImageLayout layout);
            vk::ImageLayout getLayout(uint32_t attachment) const;

            const core::ImageViewCPP& getDepthView() const;
            const core::ImageViewCPP& getImageView() const;
            const core::ImageViewCPP& getAlbedoView() const;
        	const core::ImageViewCPP& getNormalView() const;
        	const core::ImageViewCPP& getMaterialView() const;
            const core::ImageViewCPP& getPositionView() const;
            const core::ImageViewCPP& getEmissiveView() const;
        private:
            core::Device const& m_device;
            vk::Extent2D m_extent;
            std::vector<core::ImageCPP> m_images;
            std::vector<core::ImageViewCPP> m_views;
            std::vector<Attachment> m_attachments;
            std::vector<uint32_t> m_input_attachments = {};
            std::vector<uint32_t> m_output_attachments = { 0 };
        };
    }
}