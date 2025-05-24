/* Copyright (c) 2019-2020, Arm Limited and Contributors
 * Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/framebuffer.h"
#include "core/device.h"

namespace frame {
	namespace core {
        FramebufferCPP::FramebufferCPP(Device& device, const rendering::RenderTarget& render_target, const RenderPassCPP& render_pass) :
            VulkanResource{ VK_NULL_HANDLE, &device },
            m_extent{ render_target.getExtent() }
        {
            std::vector<vk::ImageView> attachments;

            for (const auto& view : render_target.getViews()) {
                attachments.emplace_back(view.getHandle());
            }

            vk::FramebufferCreateInfo create_info{};
            create_info.setRenderPass(render_pass.getHandle())
                .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
                .setPAttachments(attachments.data())
                .setWidth(m_extent.width)
                .setHeight(m_extent.height)
                .setLayers(1);

            try {
            	setHandle(getDevice().getHandle().createFramebuffer(create_info));
            }
            catch (const vk::SystemError& error) {
                throw std::runtime_error("Cannot create FramebufferCPP: " + std::string(error.what()));
            }
        }

        FramebufferCPP::FramebufferCPP(FramebufferCPP&& other) :
            VulkanResource{ std::move(other) },
            m_extent{ other.m_extent }
        {
            other.setHandle(VK_NULL_HANDLE);
        }

        FramebufferCPP::~FramebufferCPP() {
            if (hasHandle()) {
                getDevice().getHandle().destroyFramebuffer(getHandle());
                setHandle(VK_NULL_HANDLE);
            }
        }
        
        const vk::Extent2D& FramebufferCPP::getExtent() const {
            return m_extent;
        }
	}
}
