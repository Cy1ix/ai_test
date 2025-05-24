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

#include "core/resource_replay.h"
#include "common/common.h"
#include "utils/logger.h"
#include "rendering/pipeline_state.h"
#include "core/resource_cache.h"

namespace frame {
    namespace core {
        namespace {
            inline void readSubpassInfo(std::istringstream& is, std::vector<SubpassInfo>& value) {
                std::size_t size;
                common::read(is, size);
                value.resize(size);
                for (SubpassInfo& subpass : value) {
                    common::read(is, subpass.input_attachments);
                    common::read(is, subpass.output_attachments);
                }
            }

            inline void readProcesses(std::istringstream& is, std::vector<std::string>& value) {
                std::size_t size;
                common::read(is, size);
                value.resize(size);
                for (std::string& item : value) {
                    common::read(is, item);
                }
            }
        }

        ResourceReplay::ResourceReplay() {
            m_stream_resources[ResourceType::ShaderModuleCPP] = std::bind(&ResourceReplay::createShaderModule, this, std::placeholders::_1, std::placeholders::_2);
            m_stream_resources[ResourceType::PipelineLayoutCPP] = std::bind(&ResourceReplay::createPipelineLayout, this, std::placeholders::_1, std::placeholders::_2);
            m_stream_resources[ResourceType::RenderPassCPP] = std::bind(&ResourceReplay::createRenderPass, this, std::placeholders::_1, std::placeholders::_2);
            m_stream_resources[ResourceType::GraphicsPipelineCPP] = std::bind(&ResourceReplay::createGraphicsPipeline, this, std::placeholders::_1, std::placeholders::_2);
        }

        void ResourceReplay::play(ResourceCache& resource_cache, ResourceRecord& recorder) {
            std::istringstream stream{ recorder.getStream().str() };

            while (true) {
                ResourceType resource_type;
                common::read(stream, resource_type);

                if (stream.eof()) {
                    break;
                }

                auto cmd_it = m_stream_resources.find(resource_type);

                if (cmd_it != m_stream_resources.end()) {
                    cmd_it->second(resource_cache, stream);
                }
                else {
                    LOGE("[ResourceReplay] Replay command not supported.");
                }
            }
        }

        void ResourceReplay::createShaderModule(ResourceCache& resource_cache, std::istringstream& stream) {

            vk::ShaderStageFlagBits stage{};
            std::string glsl_source;
            std::string entry_point;
            std::string preamble;
            std::vector<std::string> processes;

            common::read(stream, stage, glsl_source, entry_point, preamble);

            readProcesses(stream, processes);

            ShaderSource shader_source{};
            shader_source.setSource(std::move(glsl_source));
            ShaderVariant shader_variant(std::move(preamble), std::move(processes));

            auto& shader_module = resource_cache.requestShaderModule(stage, shader_source, shader_variant);

            m_shader_modules.push_back(&shader_module);
        }

        void ResourceReplay::createPipelineLayout(ResourceCache& resource_cache, std::istringstream& stream) {

            std::vector<size_t> shader_indices;

            common::read(stream, shader_indices);

            std::vector<ShaderModuleCPP*> shader_stages(shader_indices.size());
            std::transform(shader_indices.begin(),
                shader_indices.end(),
                shader_stages.begin(),
                [&](size_t shader_index) {
                    assert(shader_index < m_shader_modules.size());
                    return m_shader_modules[shader_index];
                });

            auto& pipeline_layout = resource_cache.requestPipelineLayout(shader_stages);

            m_pipeline_layouts.push_back(&pipeline_layout);
        }

        void ResourceReplay::createRenderPass(ResourceCache& resource_cache, std::istringstream& stream) {

            std::vector<rendering::Attachment> attachments;
            std::vector<rendering::LoadStoreInfo> load_store_infos;
            std::vector<SubpassInfo> subpasses;

            common::read(stream, attachments, load_store_infos);

            readSubpassInfo(stream, subpasses);

            auto& render_pass = resource_cache.requestRenderPass(attachments, load_store_infos, subpasses);

            m_render_passes.push_back(&render_pass);
        }

        void ResourceReplay::createGraphicsPipeline(ResourceCache& resource_cache, std::istringstream& stream) {

            size_t pipeline_layout_index{};
            size_t render_pass_index{};
            uint32_t subpass_index{};

            common::read(stream, pipeline_layout_index, render_pass_index, subpass_index);

            std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state{};
            common::read(stream, specialization_constant_state);

            rendering::VertexInputState vertex_input_state{};

            common::read(stream,
                vertex_input_state.attributes,
                vertex_input_state.bindings);

            rendering::InputAssemblyState input_assembly_state{};
            rendering::RasterizationState rasterization_state{};
            rendering::ViewportState      viewport_state{};
            rendering::MultisampleState   multisample_state{};
            rendering::DepthStencilState  depth_stencil_state{};

            common::read(stream, input_assembly_state, rasterization_state, viewport_state, multisample_state, depth_stencil_state);

            rendering::ColorBlendState color_blend_state{};

            common::read(stream, color_blend_state.logic_op, color_blend_state.logic_op_enable, color_blend_state.attachments);

            rendering::PipelineState pipeline_state{};
            assert(pipeline_layout_index < m_pipeline_layouts.size());
            pipeline_state.setPipelineLayout(*m_pipeline_layouts[pipeline_layout_index]);
            assert(render_pass_index < m_render_passes.size());
            pipeline_state.setRenderPass(*m_render_passes[render_pass_index]);

            for (auto& item : specialization_constant_state) {
                pipeline_state.setSpecializationConstant(item.first, item.second);
            }

            pipeline_state.setSubpassIndex(subpass_index);
            pipeline_state.setVertexInputState(vertex_input_state);
            pipeline_state.setInputAssemblyState(input_assembly_state);
            pipeline_state.setRasterizationState(rasterization_state);
            pipeline_state.setViewportState(viewport_state);
            pipeline_state.setMultisampleState(multisample_state);
            pipeline_state.setDepthStencilState(depth_stencil_state);
            pipeline_state.setColorBlendState(color_blend_state);

            auto& graphics_pipeline = resource_cache.requestGraphicsPipeline(pipeline_state);

            m_graphics_pipelines.push_back(&graphics_pipeline);
        }
    }
}
