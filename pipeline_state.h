/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#pragma once

#include <vector>
#include "common/common.h"
#include "core/pipeline_layout.h"
#include "core/render_pass.h"
#include <vulkan/vulkan.hpp>

namespace frame {
    namespace core {
        class PipelineLayoutCPP;
        class RenderPassCPP;
    }

    namespace rendering {

        struct LoadStoreInfo {
            vk::AttachmentLoadOp m_load_op = vk::AttachmentLoadOp::eClear;
            vk::AttachmentStoreOp m_store_op = vk::AttachmentStoreOp::eStore;
        };

        struct StencilOpState {
            vk::StencilOp fail_op = vk::StencilOp::eReplace;
            vk::StencilOp pass_op = vk::StencilOp::eReplace;
            vk::StencilOp depth_fail_op = vk::StencilOp::eReplace;
            vk::CompareOp compare_op = vk::CompareOp::eNever;
        };

        struct ColorBlendAttachmentState {
            vk::Bool32 blend_enable = false;
            vk::BlendFactor src_color_blend_factor = vk::BlendFactor::eOne;
            vk::BlendFactor dst_color_blend_factor = vk::BlendFactor::eZero;
            vk::BlendOp color_blend_op = vk::BlendOp::eAdd;
            vk::BlendFactor src_alpha_blend_factor = vk::BlendFactor::eOne;
            vk::BlendFactor dst_alpha_blend_factor = vk::BlendFactor::eZero;
            vk::BlendOp alpha_blend_op = vk::BlendOp::eAdd;
            vk::ColorComponentFlags color_write_mask = 
                vk::ColorComponentFlagBits::eR | 
                vk::ColorComponentFlagBits::eG | 
                vk::ColorComponentFlagBits::eB | 
                vk::ColorComponentFlagBits::eA;
        };

        struct ColorBlendState {
            vk::Bool32 logic_op_enable = false;
            vk::LogicOp logic_op = vk::LogicOp::eClear;
            std::vector<ColorBlendAttachmentState> attachments;
        };

        struct DepthStencilState {
            vk::Bool32 depth_test_enable = true;
            vk::Bool32 depth_write_enable = true;
            vk::CompareOp depth_compare_op = vk::CompareOp::eGreater;
            vk::Bool32 depth_bounds_test_enable = false;
            vk::Bool32 stencil_test_enable = false;
            StencilOpState front;
            StencilOpState back;
        };

        struct InputAssemblyState {
            vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
            vk::Bool32 primitive_restart_enable = false;
        };

        struct MultisampleState {
            vk::SampleCountFlagBits rasterization_samples = vk::SampleCountFlagBits::e1;
            vk::Bool32 sample_shading_enable = false;
            float min_sample_shading = 0.0f;
            vk::SampleMask sample_mask = 0;
            vk::Bool32 alpha_to_coverage_enable = false;
            vk::Bool32 alpha_to_one_enable = false;
        };

        struct RasterizationState {
            vk::Bool32 depth_clamp_enable = false;
            vk::Bool32 rasterizer_discard_enable = false;
            vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
            vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;
            vk::FrontFace front_face = vk::FrontFace::eCounterClockwise;
            vk::Bool32 depth_bias_enable = false;
            float depthBiasConstantFactor = 0.0f;
            float depthBiasSlopeFactor = 0.0f;
            float depth_bias_clamp = 0.0f;
            float depthBiasClamp = 0.0f;
        };

        struct VertexInputState {
            std::vector<vk::VertexInputBindingDescription> bindings;
            std::vector<vk::VertexInputAttributeDescription> attributes;
        };

        struct ViewportState {
            uint32_t viewport_count = 1;
            uint32_t scissor_count = 1;
        };
        
        class SpecializationConstantState {
        public:
            void reset();
            bool isDirty() const;
            void clearDirty();

            template <class T>
            void setConstant(uint32_t constant_id, const T& data);

            void setConstant(uint32_t constant_id, const std::vector<uint8_t>& data);
            void setSpecializationConstantState(const std::map<uint32_t, std::vector<uint8_t>>& state);
            const std::map<uint32_t, std::vector<uint8_t>>& getSpecializationConstantState() const;

        private:
            bool m_dirty{ false };
            std::map<uint32_t, std::vector<uint8_t>> m_specialization_constant_state;
        };

        template <class T>
        inline void SpecializationConstantState::setConstant(uint32_t constant_id, const T& data) {
            setConstant(constant_id, common::toBytes(static_cast<uint32_t>(data)));
        }

        template <>
        inline void SpecializationConstantState::setConstant<bool>(uint32_t constant_id, const bool& data) {
            setConstant(constant_id, common::toBytes(static_cast<uint32_t>(data)));
        }

        class PipelineState {
        public:
            void reset();

            void setPipelineLayout(core::PipelineLayoutCPP& pipeline_layout);
            void setRenderPass(const core::RenderPassCPP& render_pass);
            void setSpecializationConstant(uint32_t constant_id, const std::vector<uint8_t>& data);
            void setVertexInputState(const VertexInputState& vertex_input_state);
            void setInputAssemblyState(const InputAssemblyState& input_assembly_state);
            void setRasterizationState(const RasterizationState& rasterization_state);
            void setViewportState(const ViewportState& viewport_state);
            void setMultisampleState(const MultisampleState& multisample_state);
            void setDepthStencilState(const DepthStencilState& depth_stencil_state);
            void setColorBlendState(const ColorBlendState& color_blend_state);
            void setSubpassIndex(uint32_t subpass_index);

            const core::PipelineLayoutCPP& getPipelineLayout() const;
            const core::RenderPassCPP* getRenderPass() const;
            const SpecializationConstantState& getSpecializationConstantState() const;
            const VertexInputState& getVertexInputState() const;
            const InputAssemblyState& getInputAssemblyState() const;
            const RasterizationState& getRasterizationState() const;
            const ViewportState& getViewportState() const;
            const MultisampleState& getMultisampleState() const;
            const DepthStencilState& getDepthStencilState() const;
            const ColorBlendState& getColorBlendState() const;
            uint32_t getSubpassIndex() const;

            bool isDirty() const;
            void clearDirty();

        private:
            bool m_dirty{ false };

            core::PipelineLayoutCPP* m_pipeline_layout{ nullptr };
            const core::RenderPassCPP* m_render_pass{ nullptr };

            SpecializationConstantState m_specialization_constant_state{};
            VertexInputState m_vertex_input_state{};
            InputAssemblyState m_input_assembly_state{};
            RasterizationState m_rasterization_state{};
            ViewportState m_viewport_state{};
            MultisampleState m_multisample_state{};
            DepthStencilState m_depth_stencil_state{};
            ColorBlendState m_color_blend_state{};

            uint32_t m_subpass_index{ 0U };
        };
        
        bool operator==(const ColorBlendAttachmentState& lhs, const ColorBlendAttachmentState& rhs);
        bool operator!=(const StencilOpState& lhs, const StencilOpState& rhs);
        bool operator!=(const VertexInputState& lhs, const VertexInputState& rhs);
        bool operator!=(const InputAssemblyState& lhs, const InputAssemblyState& rhs);
        bool operator!=(const RasterizationState& lhs, const RasterizationState& rhs);
        bool operator!=(const ViewportState& lhs, const ViewportState& rhs);
        bool operator!=(const MultisampleState& lhs, const MultisampleState& rhs);
        bool operator!=(const DepthStencilState& lhs, const DepthStencilState& rhs);
        bool operator!=(const ColorBlendState& lhs, const ColorBlendState& rhs);
    }
}
