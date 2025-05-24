/* Copyright (c) 2019-2024, Arm Limited and Contributors
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
#include "core/vulkan_resource.h"
#include <vulkan/vulkan.hpp>

namespace frame {
	
    namespace rendering {
        struct LoadStoreInfo;
        struct Attachment;
    }

    namespace core {
        class Device;

        struct SubpassInfo {
            std::vector<uint32_t> input_attachments;
            std::vector<uint32_t> output_attachments;
            std::vector<uint32_t> color_resolve_attachments;
            bool disable_depth_stencil_attachment{ false };
            uint32_t depth_stencil_resolve_attachment{ 0 };
            vk::ResolveModeFlagBits depth_stencil_resolve_mode{ vk::ResolveModeFlagBits::eNone };
            std::string debug_name{};
        };

        class RenderPassCPP : public VulkanResource<vk::RenderPass> {
        public:
            RenderPassCPP(Device& device,
                const std::vector<rendering::Attachment>& attachments,
                const std::vector<rendering::LoadStoreInfo>& load_store_infos,
                const std::vector<SubpassInfo>& subpasses);

            RenderPassCPP(const RenderPassCPP&) = delete;
            RenderPassCPP(RenderPassCPP&& other);
            ~RenderPassCPP();

            RenderPassCPP& operator=(const RenderPassCPP&) = delete;
            RenderPassCPP& operator=(RenderPassCPP&&) = delete;
            
            uint32_t getColorOutputCount(uint32_t subpass_index) const;
            
            vk::Extent2D getRenderAreaGranularity() const;

        private:
            size_t m_subpass_count;
            std::vector<uint32_t> m_color_output_count;
            
            void createRenderpassCPP(const std::vector<rendering::Attachment>& attachments,
                const std::vector<rendering::LoadStoreInfo>& load_store_infos,
                const std::vector<SubpassInfo>& subpasses);
        };
    }
}