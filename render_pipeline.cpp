/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/render_pipeline.h"

#include "scene/components/camera/camera.h"
#include "scene/components/image/image.h"
#include "scene/components/material/material.h"
#include "scene/components/mesh/mesh.h"
#include "scene/components/material/pbr_material.h"
#include "scene/components/sampler.h"
#include "scene/components/mesh/sub_mesh.h"
#include "scene/components/texture.h"
#include "scene/node.h"

namespace frame {
    namespace rendering {
        RenderPipeline::RenderPipeline(std::vector<std::unique_ptr<Subpass>>&& subpasses) :
            m_subpasses{ std::move(subpasses) }
        {
            prepare();
            
            m_load_store[0].m_load_op = vk::AttachmentLoadOp::eClear;
            m_load_store[0].m_store_op = vk::AttachmentStoreOp::eStore;
            
            m_load_store[1].m_load_op = vk::AttachmentLoadOp::eClear;
            m_load_store[1].m_store_op = vk::AttachmentStoreOp::eDontCare;
            
            m_clear_value[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_clear_value[1].depthStencil = vk::ClearDepthStencilValue{ 0.0f, ~0U };
        }

        void RenderPipeline::prepare() {
            for (auto& subpass : m_subpasses) {
                subpass->prepare();
            }
        }

        void RenderPipeline::addSubpass(std::unique_ptr<Subpass>&& subpass) {
            subpass->prepare();
            m_subpasses.emplace_back(std::move(subpass));
        }

        void RenderPipeline::addForwardSubpass(std::unique_ptr<subpass::ForwardSubpass>&& subpass) {
            addSubpass(std::move(subpass));
        }

        std::vector<std::unique_ptr<Subpass>>& RenderPipeline::getSubpasses() {
            return m_subpasses;
        }

        const std::vector<LoadStoreInfo>& RenderPipeline::getLoadStore() const {
            return m_load_store;
        }

        void RenderPipeline::setLoadStore(const std::vector<LoadStoreInfo>& load_store) {
            m_load_store = load_store;
        }

        const std::vector<vk::ClearValue>& RenderPipeline::getClearValue() const {
            return m_clear_value;
        }

        void RenderPipeline::setClearValue(const std::vector<vk::ClearValue>& clear_values) {
            m_clear_value = clear_values;
        }

        void RenderPipeline::draw(core::CommandBuffer& command_buffer, RenderTarget& render_target, vk::SubpassContents contents) {

            assert(!m_subpasses.empty() && "Render pipeline should contain at least one sub-pass");
            
            while (m_clear_value.size() < render_target.getAttachments().size()) {
                m_clear_value.push_back(vk::ClearValue{ vk::ClearColorValue().setFloat32({0.0f, 0.0f, 0.0f, 1.0f}) });
            }

            for (size_t i = 0; i < m_subpasses.size(); ++i) {

                m_active_subpass_index = i;
                auto& subpass = m_subpasses[i];

                subpass->updateRenderTargetAttachments(render_target);

                if (i == 0) {
                    command_buffer.beginRenderPass(render_target, m_load_store, m_clear_value, m_subpasses, contents);
                }
                else {
                    command_buffer.nextSubpass();
                }

                if (subpass->getDebugName().empty()) {
                    subpass->setDebugName(fmt::format("RP subpass #{}", i));
                }
                core::ScopedDebugLabel subpass_debug_label{ command_buffer, subpass->getDebugName().c_str() };

                subpass->draw(command_buffer);
            }

            m_active_subpass_index = 0;
        }

        std::unique_ptr<Subpass>& RenderPipeline::getActiveSubpass() {
            return m_subpasses[m_active_subpass_index];
        }
    }
}
