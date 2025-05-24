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

#pragma once

#include "common/helper.h"
#include "common/common.h"
#include "core/render_pass.h"
#include "rendering/render_target.h"
#include <vulkan/vulkan.hpp>

namespace frame {
	namespace rendering {
		class RenderTarget;
	}

    namespace core {
        class Device;
        class RenderPassCPP;
        class FramebufferCPP : public VulkanResource<vk::Framebuffer> {
        public:
            FramebufferCPP(Device& device, const rendering::RenderTarget& render_target, const RenderPassCPP& render_pass);

            FramebufferCPP(const FramebufferCPP&) = delete;
            FramebufferCPP& operator=(const FramebufferCPP&) = delete;
            FramebufferCPP(FramebufferCPP&& other);
            FramebufferCPP& operator=(FramebufferCPP&&) = delete;

            ~FramebufferCPP();

            const vk::Extent2D& getExtent() const;

        private:
            vk::Extent2D m_extent{};
        };
    }
}
