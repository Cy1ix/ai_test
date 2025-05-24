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

#include "core/resource_record.h"
#include "core/pipeline.h"
#include "core/pipeline_layout.h"
#include "core/render_pass.h"
#include "core/shader_module.h"
#include "core/resource_cache.h"

namespace frame {
    namespace core {
        namespace {
            inline void writeSubpassInfo(std::ostringstream& os, const std::vector<core::SubpassInfo>& value) {
                common::write(os, value.size());
                for (const core::SubpassInfo& item : value) {
                    common::write(os, item.input_attachments);
                    common::write(os, item.output_attachments);
                }
            }

            inline void writeProcesses(std::ostringstream& os, const std::vector<std::string>& value) {
                common::write(os, value.size());
                for (const std::string& item : value) {
                    common::write(os, item);
                }
            }
        }

        void ResourceRecord::setData(const std::vector<uint8_t>& data) {
            m_stream.str(std::string{ data.begin(), data.end() });
        }

        std::vector<uint8_t> ResourceRecord::getData() {
            std::string str = m_stream.str();
            return std::vector<uint8_t>{str.begin(), str.end()};
        }

        const std::ostringstream& ResourceRecord::getStream() {
            return m_stream;
        }

        size_t ResourceRecord::registerShaderModule(vk::ShaderStageFlagBits stage,
            const core::ShaderSource& glsl_source,
            const std::string& entry_point,
            const core::ShaderVariant& shader_variant)
        {
            m_shader_module_indices.push_back(m_shader_module_indices.size());

            common::write(m_stream, ResourceType::ShaderModuleCPP,
                static_cast<vk::ShaderStageFlagBits>(stage),
                glsl_source.getSource(),
                entry_point,
                shader_variant.getPreamble());

            writeProcesses(m_stream, shader_variant.getProcesses());

            return m_shader_module_indices.back();
        }

        size_t ResourceRecord::registerPipelineLayout(const std::vector<core::ShaderModuleCPP*>& shader_modules) {
            m_pipeline_layout_indices.push_back(m_pipeline_layout_indices.size());

            std::vector<size_t> shader_indices(shader_modules.size());
            std::transform(shader_modules.begin(), shader_modules.end(), shader_indices.begin(),
                [this](core::ShaderModuleCPP* shader_module) {
                    return m_shader_module_to_index.at(shader_module);
                });

            common::write(m_stream, ResourceType::PipelineLayoutCPP, shader_indices);

            return m_pipeline_layout_indices.back();
        }

        size_t ResourceRecord::registerRenderPass(const std::vector<rendering::Attachment>& attachments,
            const std::vector<rendering::LoadStoreInfo>& load_store_infos,
            const std::vector<core::SubpassInfo>& subpasses)
    	{
            m_render_pass_indices.push_back(m_render_pass_indices.size());

            common::write(m_stream, ResourceType::RenderPassCPP, attachments, load_store_infos);

            writeSubpassInfo(m_stream, subpasses);

            return m_render_pass_indices.back();
        }

        size_t ResourceRecord::registerGraphicsPipeline(vk::PipelineCache pipeline_cache,
            rendering::PipelineState& pipeline_state)
        {
            m_graphics_pipeline_indices.push_back(m_graphics_pipeline_indices.size());

            auto& pipeline_layout = pipeline_state.getPipelineLayout();
            auto render_pass = pipeline_state.getRenderPass();

            auto pipeline_layout_it = m_pipeline_layout_to_index.find(&pipeline_layout);
            if (pipeline_layout_it == m_pipeline_layout_to_index.end()) {
                throw std::runtime_error("PipelineLayout not registered in m_pipeline_layout_to_index");
            }
            
            auto render_pass_it = m_render_pass_to_index.find(render_pass);
            if (render_pass_it == m_render_pass_to_index.end()) {
                throw std::runtime_error("RenderPass not registered in m_render_pass_to_index");
            }

            common::write(m_stream,
                ResourceType::GraphicsPipelineCPP,
                m_pipeline_layout_to_index.at(&pipeline_layout),
                m_render_pass_to_index.at(render_pass),
                pipeline_state.getSubpassIndex());

            auto& specialization_constant_state = pipeline_state.getSpecializationConstantState().getSpecializationConstantState();

            common::write(m_stream, specialization_constant_state);

            auto& vertex_input_state = pipeline_state.getVertexInputState();

            common::write(m_stream, vertex_input_state.attributes, vertex_input_state.bindings);

            common::write(m_stream,
                pipeline_state.getInputAssemblyState(),
                pipeline_state.getRasterizationState(),
                pipeline_state.getViewportState(),
                pipeline_state.getMultisampleState(),
                pipeline_state.getDepthStencilState());

            auto& color_blend_state = pipeline_state.getColorBlendState();

            common::write(m_stream,
                color_blend_state.logic_op,
                color_blend_state.logic_op_enable,
                color_blend_state.attachments);

            return m_graphics_pipeline_indices.back();
        }

        void ResourceRecord::setShaderModule(size_t index, const core::ShaderModuleCPP& shader_module) {
            m_shader_module_to_index[&shader_module] = index;
        }

        void ResourceRecord::setPipelineLayout(size_t index, const core::PipelineLayoutCPP& pipeline_layout) {
            m_pipeline_layout_to_index[&pipeline_layout] = index;
        }

        void ResourceRecord::setRenderPass(size_t index, const core::RenderPassCPP& render_pass) {
            m_render_pass_to_index[&render_pass] = index;
        }

        void ResourceRecord::setGraphicsPipeline(size_t index, const core::GraphicsPipelineCPP& graphics_pipeline) {
            m_graphics_pipeline_to_index[&graphics_pipeline] = index;
        }
    }
}
