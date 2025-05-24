/* Copyright (c) 2019-2021, Arm Limited and Contributors
 * Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/pipeline_state.h"

namespace frame {
    namespace rendering {

        bool operator==(const ColorBlendAttachmentState& lhs, const ColorBlendAttachmentState& rhs) {
            return std::tie(lhs.alpha_blend_op, lhs.blend_enable, lhs.color_blend_op, lhs.color_write_mask,
                lhs.dst_alpha_blend_factor, lhs.dst_color_blend_factor, lhs.src_alpha_blend_factor,
                lhs.src_color_blend_factor) ==
                std::tie(rhs.alpha_blend_op, rhs.blend_enable, rhs.color_blend_op, rhs.color_write_mask,
                    rhs.dst_alpha_blend_factor, rhs.dst_color_blend_factor, rhs.src_alpha_blend_factor,
                    rhs.src_color_blend_factor);
        }

        bool operator!=(const StencilOpState& lhs, const StencilOpState& rhs) {
            return std::tie(lhs.compare_op, lhs.depth_fail_op, lhs.fail_op, lhs.pass_op) !=
                std::tie(rhs.compare_op, rhs.depth_fail_op, rhs.fail_op, rhs.pass_op);
        }

        bool operator!=(const VertexInputState& lhs, const VertexInputState& rhs) {
            return lhs.attributes != rhs.attributes || lhs.bindings != rhs.bindings;
        }

        bool operator!=(const InputAssemblyState& lhs, const InputAssemblyState& rhs) {
            return std::tie(lhs.primitive_restart_enable, lhs.topology) !=
                std::tie(rhs.primitive_restart_enable, rhs.topology);
        }

        bool operator!=(const RasterizationState& lhs, const RasterizationState& rhs) {
            return std::tie(lhs.cull_mode, lhs.depth_bias_enable, lhs.depth_clamp_enable, lhs.front_face,
                lhs.polygon_mode, lhs.rasterizer_discard_enable) !=
                std::tie(rhs.cull_mode, rhs.depth_bias_enable, rhs.depth_clamp_enable, rhs.front_face,
                    rhs.polygon_mode, rhs.rasterizer_discard_enable);
        }

        bool operator!=(const ViewportState& lhs, const ViewportState& rhs) {
            return lhs.viewport_count != rhs.viewport_count || lhs.scissor_count != rhs.scissor_count;
        }

        bool operator!=(const MultisampleState& lhs, const MultisampleState& rhs) {
            return std::tie(lhs.alpha_to_coverage_enable, lhs.alpha_to_one_enable, lhs.min_sample_shading,
                lhs.rasterization_samples, lhs.sample_mask, lhs.sample_shading_enable) !=
                std::tie(rhs.alpha_to_coverage_enable, rhs.alpha_to_one_enable, rhs.min_sample_shading,
                    rhs.rasterization_samples, rhs.sample_mask, rhs.sample_shading_enable);
        }

        bool operator!=(const DepthStencilState& lhs, const DepthStencilState& rhs) {
            return std::tie(lhs.depth_bounds_test_enable, lhs.depth_compare_op, lhs.depth_test_enable,
                lhs.depth_write_enable, lhs.stencil_test_enable) !=
                std::tie(rhs.depth_bounds_test_enable, rhs.depth_compare_op, rhs.depth_test_enable,
                    rhs.depth_write_enable, rhs.stencil_test_enable) ||
                lhs.back != rhs.back || lhs.front != rhs.front;
        }

        bool operator!=(const ColorBlendState& lhs, const ColorBlendState& rhs) {
            return std::tie(lhs.logic_op, lhs.logic_op_enable) != std::tie(rhs.logic_op, rhs.logic_op_enable) ||
                lhs.attachments.size() != rhs.attachments.size() ||
                !std::equal(lhs.attachments.begin(), lhs.attachments.end(), rhs.attachments.begin(),
                    [](const ColorBlendAttachmentState& lhs, const ColorBlendAttachmentState& rhs) {
                        return lhs == rhs;
                    });
        }

        void SpecializationConstantState::reset() {
            if (m_dirty) {
                m_specialization_constant_state.clear();
            }
            m_dirty = false;
        }

        bool SpecializationConstantState::isDirty() const {
            return m_dirty;
        }

        void SpecializationConstantState::clearDirty() {
            m_dirty = false;
        }

        void SpecializationConstantState::setConstant(uint32_t constant_id, const std::vector<uint8_t>& data) {
            auto existing_data = m_specialization_constant_state.find(constant_id);

            if (existing_data != m_specialization_constant_state.end() && existing_data->second == data) {
                return;
            }

            m_dirty = true;
            m_specialization_constant_state[constant_id] = data;
        }

        void SpecializationConstantState::setSpecializationConstantState(const std::map<uint32_t, std::vector<uint8_t>>& state) {
            m_specialization_constant_state = state;
        }

        const std::map<uint32_t, std::vector<uint8_t>>& SpecializationConstantState::getSpecializationConstantState() const {
            return m_specialization_constant_state;
        }

        void PipelineState::reset() {
            clearDirty();

            m_pipeline_layout = nullptr;
            m_render_pass = nullptr;

            m_specialization_constant_state.reset();
            m_vertex_input_state = {};
            m_input_assembly_state = {};
            m_rasterization_state = {};
            m_viewport_state = {};
            m_multisample_state = {};
            m_depth_stencil_state = {};
            m_color_blend_state = {};
            m_subpass_index = 0U;
        }

        void PipelineState::setPipelineLayout(core::PipelineLayoutCPP& pipeline_layout) {
            if (m_pipeline_layout) {
                if (m_pipeline_layout->getHandle() != pipeline_layout.getHandle()) {
                    m_pipeline_layout = &pipeline_layout;
                    m_dirty = true;
                }
            }
            else {
                m_pipeline_layout = &pipeline_layout;
                m_dirty = true;
            }
        }

        void PipelineState::setRenderPass(const core::RenderPassCPP& render_pass) {
            if (m_render_pass) {
                if (m_render_pass->getHandle() != render_pass.getHandle()) {
                    m_render_pass = &render_pass;
                    m_dirty = true;
                }
            }
            else {
                m_render_pass = &render_pass;
                m_dirty = true;
            }
        }

        void PipelineState::setSpecializationConstant(uint32_t constant_id, const std::vector<uint8_t>& data) {
            m_specialization_constant_state.setConstant(constant_id, data);

            if (m_specialization_constant_state.isDirty()) {
                m_dirty = true;
            }
        }

        void PipelineState::setVertexInputState(const VertexInputState& vertex_input_state) {
            if (m_vertex_input_state != vertex_input_state) {
                m_vertex_input_state = vertex_input_state;
                m_dirty = true;
            }
        }

        void PipelineState::setInputAssemblyState(const InputAssemblyState& input_assembly_state) {
            if (m_input_assembly_state != input_assembly_state) {
                m_input_assembly_state = input_assembly_state;
                m_dirty = true;
            }
        }

        void PipelineState::setRasterizationState(const RasterizationState& rasterization_state) {
            if (m_rasterization_state != rasterization_state) {
                m_rasterization_state = rasterization_state;
                m_dirty = true;
            }
        }

        void PipelineState::setViewportState(const ViewportState& viewport_state) {
            if (m_viewport_state != viewport_state) {
                m_viewport_state = viewport_state;
                m_dirty = true;
            }
        }

        void PipelineState::setMultisampleState(const MultisampleState& multisample_state) {
            if (m_multisample_state != multisample_state) {
                m_multisample_state = multisample_state;
                m_dirty = true;
            }
        }

        void PipelineState::setDepthStencilState(const DepthStencilState& depth_stencil_state) {
            if (m_depth_stencil_state != depth_stencil_state) {
                m_depth_stencil_state = depth_stencil_state;
                m_dirty = true;
            }
        }

        void PipelineState::setColorBlendState(const ColorBlendState& color_blend_state) {
            if (m_color_blend_state != color_blend_state) {
                m_color_blend_state = color_blend_state;
                m_dirty = true;
            }
        }

        void PipelineState::setSubpassIndex(uint32_t subpass_index) {
            if (m_subpass_index != subpass_index) {
                m_subpass_index = subpass_index;
                m_dirty = true;
            }
        }

        const core::PipelineLayoutCPP& PipelineState::getPipelineLayout() const {
            assert(m_pipeline_layout && "[PipelineState] ASSERT: PipelineCPP layout is not set");
            return *m_pipeline_layout;
        }

        const core::RenderPassCPP* PipelineState::getRenderPass() const {
            return m_render_pass;
        }

        const SpecializationConstantState& PipelineState::getSpecializationConstantState() const {
            return m_specialization_constant_state;
        }

        const VertexInputState& PipelineState::getVertexInputState() const {
            return m_vertex_input_state;
        }

        const InputAssemblyState& PipelineState::getInputAssemblyState() const {
            return m_input_assembly_state;
        }

        const RasterizationState& PipelineState::getRasterizationState() const {
            return m_rasterization_state;
        }

        const ViewportState& PipelineState::getViewportState() const {
            return m_viewport_state;
        }

        const MultisampleState& PipelineState::getMultisampleState() const {
            return m_multisample_state;
        }

        const DepthStencilState& PipelineState::getDepthStencilState() const {
            return m_depth_stencil_state;
        }

        const ColorBlendState& PipelineState::getColorBlendState() const {
            return m_color_blend_state;
        }

        uint32_t PipelineState::getSubpassIndex() const {
            return m_subpass_index;
        }

        bool PipelineState::isDirty() const {
            return m_dirty || m_specialization_constant_state.isDirty();
        }

        void PipelineState::clearDirty() {
            m_dirty = false;
            m_specialization_constant_state.clearDirty();
        }
    }
}
