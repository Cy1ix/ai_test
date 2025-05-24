/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "core/pipeline.h"
#include "core/device.h"
#include "core/pipeline_layout.h"
#include "core/shader_module.h"

namespace frame {
	namespace core {
        PipelineCPP::PipelineCPP(Device& device) :
            VulkanResource{ VK_NULL_HANDLE, &device }
        {}

        PipelineCPP::PipelineCPP(PipelineCPP&& other) :
            VulkanResource{ std::move(other) },
            m_state{ std::move(other.m_state) }
        {
            other.setHandle(VK_NULL_HANDLE);
        }

        PipelineCPP::~PipelineCPP() {
            if (hasHandle()) {
                getDevice().getHandle().destroyPipeline(getHandle());
            }
        }
        
        const rendering::PipelineState& PipelineCPP::getState() const {
            return m_state;
        }

        ComputePipelineCPP::ComputePipelineCPP(Device& device, vk::PipelineCache pipeline_cache, rendering::PipelineState& pipeline_state) :
            PipelineCPP{ device }
        {
            const ShaderModuleCPP* shader_module = pipeline_state.getPipelineLayout().getShaderModules().front();

            if (shader_module->getStage() != vk::ShaderStageFlagBits::eCompute) {
                throw std::runtime_error("[PipelineCPP] ERROR: Shader module stage is not compute");
            }

            vk::PipelineShaderStageCreateInfo stage{};
            stage.stage = shader_module->getStage();
            stage.pName = shader_module->getEntryPoint().c_str();

            vk::ShaderModuleCreateInfo vk_create_info{};
            vk_create_info.codeSize = shader_module->getBinary().size() * sizeof(uint32_t);
            vk_create_info.pCode = shader_module->getBinary().data();

            vk::ShaderModule shader = getDevice().getHandle().createShaderModule(vk_create_info);
            /*
            getDevice().getDebugUtils().setDebugName(
                getDevice().getHandle(),
                vk::ObjectType::eShaderModule,
                reinterpret_cast<uint64_t>(static_cast<VkShaderModule>(shader)),
                shader_module->getDebugName().c_str()
            );
            */
            stage.module = shader;

            std::vector<uint8_t> data{};
            std::vector<vk::SpecializationMapEntry> map_entries{};

            const auto& specialization_constant_state = pipeline_state.getSpecializationConstantState().getSpecializationConstantState();

            for (const auto& constant : specialization_constant_state) {
                map_entries.push_back({
                    constant.first,
                    static_cast<uint32_t>(data.size()),
                    constant.second.size()
                    });
                data.insert(data.end(), constant.second.begin(), constant.second.end());
            }

            vk::SpecializationInfo specialization_info{};
            specialization_info.mapEntryCount = static_cast<uint32_t>(map_entries.size());
            specialization_info.pMapEntries = map_entries.data();
            specialization_info.dataSize = data.size();
            specialization_info.pData = data.data();

            stage.pSpecializationInfo = &specialization_info;

            vk::ComputePipelineCreateInfo create_info{};
            create_info.layout = pipeline_state.getPipelineLayout().getHandle();
            create_info.stage = stage;

            auto result = getDevice().getHandle().createComputePipeline(pipeline_cache, create_info, nullptr);

            if(result.result != vk::Result::eSuccess) {
                LOGE("Create compute pipeline fail");
                getDevice().getHandle().destroyShaderModule(shader);
                throw std::runtime_error("[PipelineCPP] ERROR: Failed to create compute pipeline");
            }

            setHandle(result.value);

            getDevice().getHandle().destroyShaderModule(shader);

            m_state = pipeline_state;
        }

        GraphicsPipelineCPP::GraphicsPipelineCPP(Device& device, vk::PipelineCache pipeline_cache, rendering::PipelineState& pipeline_state) :
            PipelineCPP{ device }
        {
            std::vector<vk::ShaderModule> shader_modules;
            std::vector<vk::PipelineShaderStageCreateInfo> stage_create_infos;

            std::vector<uint8_t> data{};
            std::vector<vk::SpecializationMapEntry> map_entries{};

            const auto& specialization_constant_state = pipeline_state.getSpecializationConstantState().getSpecializationConstantState();

            for (const auto& constant : specialization_constant_state) {
                map_entries.push_back({
                    constant.first,
                    static_cast<uint32_t>(data.size()),
                    constant.second.size()
                    });
                data.insert(data.end(), constant.second.begin(), constant.second.end());
            }

            vk::SpecializationInfo specialization_info{};
            specialization_info.mapEntryCount = static_cast<uint32_t>(map_entries.size());
            specialization_info.pMapEntries = map_entries.data();
            specialization_info.dataSize = data.size();
            specialization_info.pData = data.data();

            for (const ShaderModuleCPP* shader_module : pipeline_state.getPipelineLayout().getShaderModules()) {

                vk::PipelineShaderStageCreateInfo stage_create_info{};
                stage_create_info.stage = shader_module->getStage();
                stage_create_info.pName = shader_module->getEntryPoint().c_str();

                vk::ShaderModuleCreateInfo vk_create_info{};
                vk_create_info.codeSize = shader_module->getBinary().size() * sizeof(uint32_t);
                vk_create_info.pCode = shader_module->getBinary().data();

                vk::ShaderModule shader = getDevice().getHandle().createShaderModule(vk_create_info);
                
                getDevice().getDebugUtils().setDebugName(
                    getDevice().getHandle(),
                    vk::ObjectType::eShaderModule,
                    reinterpret_cast<uint64_t>(static_cast<VkShaderModule>(shader)),
                    shader_module->getDebugName().c_str()
                );
                
                stage_create_info.module = shader;
                stage_create_info.pSpecializationInfo = &specialization_info;

                stage_create_infos.push_back(stage_create_info);
                shader_modules.push_back(shader);
            }

            vk::GraphicsPipelineCreateInfo create_info{};
            create_info.stageCount = static_cast<uint32_t>(stage_create_infos.size());
            create_info.pStages = stage_create_infos.data();

            vk::PipelineVertexInputStateCreateInfo vertex_input_state{};
            vertex_input_state.pVertexAttributeDescriptions = pipeline_state.getVertexInputState().attributes.data();
            vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(pipeline_state.getVertexInputState().attributes.size());
            vertex_input_state.pVertexBindingDescriptions = pipeline_state.getVertexInputState().bindings.data();
            vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(pipeline_state.getVertexInputState().bindings.size());

            vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{};
            input_assembly_state.topology = pipeline_state.getInputAssemblyState().topology;
            input_assembly_state.primitiveRestartEnable = pipeline_state.getInputAssemblyState().primitive_restart_enable;

            vk::PipelineViewportStateCreateInfo viewport_state{};
            viewport_state.viewportCount = pipeline_state.getViewportState().viewport_count;
            viewport_state.scissorCount = pipeline_state.getViewportState().scissor_count;

            vk::PipelineRasterizationStateCreateInfo rasterization_state{};
            rasterization_state.depthClampEnable = pipeline_state.getRasterizationState().depth_clamp_enable;
            rasterization_state.rasterizerDiscardEnable = pipeline_state.getRasterizationState().rasterizer_discard_enable;
            rasterization_state.polygonMode = pipeline_state.getRasterizationState().polygon_mode;
            rasterization_state.cullMode = pipeline_state.getRasterizationState().cull_mode;
            rasterization_state.frontFace = pipeline_state.getRasterizationState().front_face;
            rasterization_state.depthBiasEnable = pipeline_state.getRasterizationState().depth_bias_enable;
            rasterization_state.depthBiasClamp = 1.0f;
            rasterization_state.depthBiasSlopeFactor = 1.0f;
            rasterization_state.lineWidth = 1.0f;

            vk::PipelineMultisampleStateCreateInfo multisample_state{};
            multisample_state.sampleShadingEnable = pipeline_state.getMultisampleState().sample_shading_enable;
            multisample_state.rasterizationSamples = pipeline_state.getMultisampleState().rasterization_samples;
            multisample_state.minSampleShading = pipeline_state.getMultisampleState().min_sample_shading;
            multisample_state.alphaToCoverageEnable = pipeline_state.getMultisampleState().alpha_to_coverage_enable;
            multisample_state.alphaToOneEnable = pipeline_state.getMultisampleState().alpha_to_one_enable;

            if (pipeline_state.getMultisampleState().sample_mask) {
                multisample_state.pSampleMask = &pipeline_state.getMultisampleState().sample_mask;
            }

            vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{};
            depth_stencil_state.depthTestEnable = pipeline_state.getDepthStencilState().depth_test_enable;
            depth_stencil_state.depthWriteEnable = pipeline_state.getDepthStencilState().depth_write_enable;
            depth_stencil_state.depthCompareOp = pipeline_state.getDepthStencilState().depth_compare_op;
            depth_stencil_state.depthBoundsTestEnable = pipeline_state.getDepthStencilState().depth_bounds_test_enable;
            depth_stencil_state.stencilTestEnable = pipeline_state.getDepthStencilState().stencil_test_enable;
            depth_stencil_state.front.failOp = pipeline_state.getDepthStencilState().front.fail_op;
            depth_stencil_state.front.passOp = pipeline_state.getDepthStencilState().front.pass_op;
            depth_stencil_state.front.depthFailOp = pipeline_state.getDepthStencilState().front.depth_fail_op;
            depth_stencil_state.front.compareOp = pipeline_state.getDepthStencilState().front.compare_op;
            depth_stencil_state.front.compareMask = ~0U;
            depth_stencil_state.front.writeMask = ~0U;
            depth_stencil_state.front.reference = ~0U;
            depth_stencil_state.back.failOp = pipeline_state.getDepthStencilState().back.fail_op;
            depth_stencil_state.back.passOp = pipeline_state.getDepthStencilState().back.pass_op;
            depth_stencil_state.back.depthFailOp = pipeline_state.getDepthStencilState().back.depth_fail_op;
            depth_stencil_state.back.compareOp = pipeline_state.getDepthStencilState().back.compare_op;
            depth_stencil_state.back.compareMask = ~0U;
            depth_stencil_state.back.writeMask = ~0U;
            depth_stencil_state.back.reference = ~0U;

            vk::PipelineColorBlendStateCreateInfo color_blend_state{};
            color_blend_state.logicOpEnable = pipeline_state.getColorBlendState().logic_op_enable;
            color_blend_state.logicOp = pipeline_state.getColorBlendState().logic_op;
            color_blend_state.attachmentCount = static_cast<uint32_t>(pipeline_state.getColorBlendState().attachments.size());
            color_blend_state.pAttachments = reinterpret_cast<const vk::PipelineColorBlendAttachmentState*>(
                pipeline_state.getColorBlendState().attachments.data()
                );
            color_blend_state.blendConstants[0] = 1.0f;
            color_blend_state.blendConstants[1] = 1.0f;
            color_blend_state.blendConstants[2] = 1.0f;
            color_blend_state.blendConstants[3] = 1.0f;

            std::array<vk::DynamicState, 9> dynamic_states{
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor,
                vk::DynamicState::eLineWidth,
                vk::DynamicState::eDepthBias,
                vk::DynamicState::eBlendConstants,
                vk::DynamicState::eDepthBounds,
                vk::DynamicState::eStencilCompareMask,
                vk::DynamicState::eStencilWriteMask,
                vk::DynamicState::eStencilReference,
            };

            vk::PipelineDynamicStateCreateInfo dynamic_state{};
            dynamic_state.pDynamicStates = dynamic_states.data();
            dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());

            create_info.pVertexInputState = &vertex_input_state;
            create_info.pInputAssemblyState = &input_assembly_state;
            create_info.pViewportState = &viewport_state;
            create_info.pRasterizationState = &rasterization_state;
            create_info.pMultisampleState = &multisample_state;
            create_info.pDepthStencilState = &depth_stencil_state;
            create_info.pColorBlendState = &color_blend_state;
            create_info.pDynamicState = &dynamic_state;

            create_info.layout = pipeline_state.getPipelineLayout().getHandle();
            create_info.renderPass = pipeline_state.getRenderPass()->getHandle();
            create_info.subpass = pipeline_state.getSubpassIndex();

            auto result = getDevice().getHandle().createGraphicsPipeline(pipeline_cache, create_info);

            if(result.result != vk::Result::eSuccess) {
                LOGE("Create graphics pipeline fail");
                for (auto& shader : shader_modules) {
                    getDevice().getHandle().destroyShaderModule(shader);
                }
                throw std::runtime_error("[PipelineCPP] ERROR: Failed to create graphics pipeline");
            }

            setHandle(result.value);

            for (auto& shader : shader_modules) {
                getDevice().getHandle().destroyShaderModule(shader);
            }

            m_state = pipeline_state;
        }
	}
}
