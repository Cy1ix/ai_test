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

#include "core/render_pass.h"
#include "core/device.h"
#include "rendering/render_target.h"
#include <fmt/format.h>
#include <numeric>

namespace frame {
    namespace core {
        namespace {
            std::vector<vk::AttachmentDescription> getAttachmentDescriptions(
                const std::vector<rendering::Attachment>& attachments,
                const std::vector<rendering::LoadStoreInfo>& load_store_infos)
            {
                std::vector<vk::AttachmentDescription> attachment_descriptions;

                for (size_t i = 0; i < attachments.size(); ++i) {
                    vk::AttachmentDescription attachment{};

                    attachment.setFormat(attachments[i].format);
                    attachment.setSamples(attachments[i].samples);
                    attachment.setInitialLayout(attachments[i].initial_layout);
                    
                    attachment.setFinalLayout(
                        common::isDepthFormat(attachment.format) ?
                        vk::ImageLayout::eDepthStencilAttachmentOptimal :
                        vk::ImageLayout::eColorAttachmentOptimal);
                    
                    if (i < load_store_infos.size()) {
                        attachment.setLoadOp(load_store_infos[i].m_load_op);
                        attachment.setStoreOp(load_store_infos[i].m_store_op);
                        attachment.setStencilLoadOp(load_store_infos[i].m_load_op);
                        attachment.setStencilStoreOp(load_store_infos[i].m_store_op);
                    }

                    attachment_descriptions.push_back(attachment);
                }

                return attachment_descriptions;
            }
            
            vk::AttachmentReference getAttachmentReference(uint32_t attachment, vk::ImageLayout layout) {
                vk::AttachmentReference reference{};
                reference.setAttachment(attachment);
                reference.setLayout(layout);
                return reference;
            }
            
            void setAttachmentLayouts(
                std::vector<vk::SubpassDescription>& subpass_descriptions,
                std::vector<vk::AttachmentDescription>& attachment_descriptions)
            {
                for (auto& subpass : subpass_descriptions) {
                    for (uint32_t k = 0; k < subpass.colorAttachmentCount; ++k){
                        auto& reference = subpass.pColorAttachments[k];
                        if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                            attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                        }
                    }
                    
                    for (uint32_t k = 0; k < subpass.inputAttachmentCount; ++k) {
                        auto& reference = subpass.pInputAttachments[k];
                        if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                            attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                        }
                    }
                    
                    if (subpass.pDepthStencilAttachment) {
                        auto& reference = *subpass.pDepthStencilAttachment;
                        if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                            attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                        }
                    }
                    
                    if (subpass.pResolveAttachments) {
                        for (uint32_t k = 0; k < subpass.colorAttachmentCount; ++k) {
                            auto& reference = subpass.pResolveAttachments[k];
                            if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                            }
                        }
                    }
                }
                
                if (!subpass_descriptions.empty()) {
                    auto& subpass = subpass_descriptions.back();
                    
                    for (uint32_t k = 0; k < subpass.colorAttachmentCount; ++k) {
                        const auto& reference = subpass.pColorAttachments[k];
                        attachment_descriptions[reference.attachment].finalLayout = reference.layout;
                    }
                    
                    for (uint32_t k = 0; k < subpass.inputAttachmentCount; ++k) {
                        const auto& reference = subpass.pInputAttachments[k];
                        attachment_descriptions[reference.attachment].finalLayout = reference.layout;
                        
                        if (common::isDepthFormat(attachment_descriptions[reference.attachment].format)) {
                            subpass.pDepthStencilAttachment = nullptr;
                        }
                    }
                    
                    if (subpass.pDepthStencilAttachment) {
                        const auto& reference = *subpass.pDepthStencilAttachment;
                        attachment_descriptions[reference.attachment].finalLayout = reference.layout;
                    }
                    
                    if (subpass.pResolveAttachments) {
                        for (uint32_t k = 0; k < subpass.colorAttachmentCount; ++k) {
                            const auto& reference = subpass.pResolveAttachments[k];
                            attachment_descriptions[reference.attachment].finalLayout = reference.layout;
                        }
                    }
                }
            }
            
            std::vector<vk::SubpassDependency> getSubpassDependencies(size_t subpass_count) {
                std::vector<vk::SubpassDependency> dependencies(subpass_count > 1 ? subpass_count - 1 : 0);

                for (uint32_t i = 0; i < dependencies.size(); ++i) {
                    dependencies[i].setSrcSubpass(i);
                    dependencies[i].setDstSubpass(i + 1);
                    dependencies[i].setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
                    dependencies[i].setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader);
                    dependencies[i].setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
                    dependencies[i].setDstAccessMask(vk::AccessFlagBits::eInputAttachmentRead);
                    dependencies[i].setDependencyFlags(vk::DependencyFlagBits::eByRegion);
                }

                return dependencies;
            }

        }

        RenderPassCPP::RenderPassCPP(Device& device,
            const std::vector<rendering::Attachment>& attachments,
            const std::vector<rendering::LoadStoreInfo>& load_store_infos,
            const std::vector<SubpassInfo>& subpasses) :
            VulkanResource{VK_NULL_HANDLE, &device},
            m_subpass_count(std::max<size_t>(1, subpasses.size())),
            m_color_output_count{}
        {
            createRenderpassCPP(attachments, load_store_infos, subpasses);
        }

        RenderPassCPP::RenderPassCPP(RenderPassCPP&& other) :
            VulkanResource{ std::move(other) },
            m_subpass_count(other.m_subpass_count),
            m_color_output_count(std::move(other.m_color_output_count))
        {
        }

        RenderPassCPP::~RenderPassCPP() {
            if (hasDevice() && hasHandle()) {
                getDevice().getHandle().destroyRenderPass(getHandle());
            }
        }

        uint32_t RenderPassCPP::getColorOutputCount(uint32_t subpass_index) const {
            return m_color_output_count[subpass_index];
        }

        vk::Extent2D RenderPassCPP::getRenderAreaGranularity() const {
            return getDevice().getHandle().getRenderAreaGranularity(getHandle());
        }

        void RenderPassCPP::createRenderpassCPP(
            const std::vector<rendering::Attachment>& attachments,
            const std::vector<rendering::LoadStoreInfo>& load_store_infos,
            const std::vector<SubpassInfo>& subpasses)
        {
            auto attachment_descriptions = getAttachmentDescriptions(attachments, load_store_infos);
            
            std::vector<std::vector<vk::AttachmentReference>> input_references(m_subpass_count);
            std::vector<std::vector<vk::AttachmentReference>> color_references(m_subpass_count);
            std::vector<std::vector<vk::AttachmentReference>> depth_references(m_subpass_count);
            std::vector<std::vector<vk::AttachmentReference>> resolve_references(m_subpass_count);
            std::vector<vk::SubpassDescriptionDepthStencilResolve> depth_resolves;
            
            std::string new_debug_name{};
            const bool  needs_debug_name = getDebugName().empty();
            if (needs_debug_name) {
                new_debug_name = fmt::format("RP with {} subpasses:\n", subpasses.size());
            }
            
            for (size_t i = 0; i < subpasses.size(); ++i) {
                auto& subpass = subpasses[i];

                if (needs_debug_name) {
                    new_debug_name += fmt::format("\t[{}]: {}\n", i, subpass.debug_name);
                }
                
                for (auto output_attachment : subpass.output_attachments) {
                    auto initial_layout = attachments[output_attachment].initial_layout == vk::ImageLayout::eUndefined ?
                        vk::ImageLayout::eColorAttachmentOptimal :
                        attachments[output_attachment].initial_layout;

                    auto& description = attachment_descriptions[output_attachment];
                    if (!common::isDepthFormat(description.format)) {
                        color_references[i].push_back(getAttachmentReference(output_attachment, initial_layout));
                    }
                }
                
                for (auto input_attachment : subpass.input_attachments) {
                    bool is_depth = common::isDepthFormat(attachment_descriptions[input_attachment].format);
                    auto default_layout = is_depth ?
                        vk::ImageLayout::eDepthStencilReadOnlyOptimal :
                        vk::ImageLayout::eShaderReadOnlyOptimal;

                    auto initial_layout = attachments[input_attachment].initial_layout == vk::ImageLayout::eUndefined ?
                        default_layout :
                        attachments[input_attachment].initial_layout;

                    input_references[i].push_back(getAttachmentReference(input_attachment, initial_layout));
                }
                
                for (auto resolve_attachment : subpass.color_resolve_attachments) {
                    auto initial_layout = attachments[resolve_attachment].initial_layout == vk::ImageLayout::eUndefined ?
                        vk::ImageLayout::eColorAttachmentOptimal :
                        attachments[resolve_attachment].initial_layout;

                    resolve_references[i].push_back(getAttachmentReference(resolve_attachment, initial_layout));
                }
                
                if (!subpass.disable_depth_stencil_attachment) {
                    auto it = std::find_if(attachments.begin(), attachments.end(),
                        [](const rendering::Attachment& attachment) {
                            return common::isDepthFormat(attachment.format);
                        });

                    if (it != attachments.end()) {
                        auto depth_index = static_cast<uint32_t>(std::distance(attachments.begin(), it));
                        auto initial_layout = it->initial_layout == vk::ImageLayout::eUndefined ?
                            vk::ImageLayout::eDepthStencilAttachmentOptimal :
                            it->initial_layout;

                        depth_references[i].push_back(getAttachmentReference(depth_index, initial_layout));
                        
                        if (subpass.depth_stencil_resolve_mode != vk::ResolveModeFlagBits::eNone) {
                            auto depth_resolve_index = subpass.depth_stencil_resolve_attachment;
                            auto resolve_layout = attachments[depth_resolve_index].initial_layout == vk::ImageLayout::eUndefined ?
                                vk::ImageLayout::eDepthStencilAttachmentOptimal :
                                attachments[depth_resolve_index].initial_layout;

                            resolve_references[i].push_back(getAttachmentReference(depth_resolve_index, resolve_layout));
                        }
                    }
                }
            }
            
            std::vector<vk::SubpassDescription> subpass_descriptions;
            subpass_descriptions.reserve(m_subpass_count);

            for (size_t i = 0; i < subpasses.size(); ++i) {
                vk::SubpassDescription subpass_description{};
                subpass_description.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
                
                if (!input_references[i].empty()) {
                    subpass_description.setInputAttachmentCount(static_cast<uint32_t>(input_references[i].size()));
                    subpass_description.setPInputAttachments(input_references[i].data());
                }
                
                if (!color_references[i].empty()) {
                    subpass_description.setColorAttachmentCount(static_cast<uint32_t>(color_references[i].size()));
                    subpass_description.setPColorAttachments(color_references[i].data());
                }
                
                if (!resolve_references[i].empty() && subpass_description.colorAttachmentCount > 0) {
                    subpass_description.setPResolveAttachments(resolve_references[i].data());
                }
                
                if (!depth_references[i].empty()) {
                    subpass_description.setPDepthStencilAttachment(depth_references[i].data());
                }

                subpass_descriptions.push_back(subpass_description);
            }
            
            if (subpasses.empty()) {
                vk::SubpassDescription default_subpass{};
                default_subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

                for (uint32_t k = 0; k < static_cast<uint32_t>(attachment_descriptions.size()); ++k) {
                    if (common::isDepthFormat(attachment_descriptions[k].format)) {
                        depth_references[0].push_back(getAttachmentReference(k, vk::ImageLayout::eDepthStencilAttachmentOptimal));
                    }
                    else {
                        color_references[0].push_back(getAttachmentReference(k, vk::ImageLayout::eGeneral));
                    }
                }
                
                if (!color_references[0].empty()) {
                    default_subpass.setColorAttachmentCount(static_cast<uint32_t>(color_references[0].size()));
                    default_subpass.setPColorAttachments(color_references[0].data());
                }
                
                if (!depth_references[0].empty()) {
                    default_subpass.setPDepthStencilAttachment(depth_references[0].data());
                }

                subpass_descriptions.push_back(default_subpass);
            }
            
            setAttachmentLayouts(subpass_descriptions, attachment_descriptions);
            
            m_color_output_count.reserve(m_subpass_count);
            for (size_t i = 0; i < m_subpass_count; i++) {
                m_color_output_count.push_back(static_cast<uint32_t>(color_references[i].size()));
            }
            
            auto subpass_dependencies = getSubpassDependencies(m_subpass_count);
            
            vk::RenderPassCreateInfo create_info{};
            create_info.setAttachmentCount(static_cast<uint32_t>(attachment_descriptions.size()));
            create_info.setPAttachments(attachment_descriptions.data());
            create_info.setSubpassCount(static_cast<uint32_t>(subpass_descriptions.size()));
            create_info.setPSubpasses(subpass_descriptions.data());
            create_info.setDependencyCount(static_cast<uint32_t>(subpass_dependencies.size()));
            create_info.setPDependencies(subpass_dependencies.data());

            try {
                setHandle(getDevice().getHandle().createRenderPass(create_info));
            }
            catch (vk::SystemError& e) {
                throw std::runtime_error("Failed to create RenderPass: " + std::string(e.what()));
            }

            if (needs_debug_name) {
                setDebugName(new_debug_name);
            }
        }
    }
}
