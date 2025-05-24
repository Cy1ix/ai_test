/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2018-2021, Arm Limited and Contributors
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
#include <functional>
#include <vulkan/vulkan.hpp>

#ifdef MemoryBarrier
#undef MemoryBarrier
#endif
#include <vulkan/vulkan_hash.hpp>

namespace frame {
    namespace common {
        template <class T>
        inline void hashCombineResource(size_t& seed, const T& value) {
            seed ^= std::hash<T>{}(value)+0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    }
}

namespace std {
    template <typename Key, typename Value>
    struct hash<std::map<Key, Value>>
    {
        size_t operator()(std::map<Key, Value> const& bindings) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, bindings.size());
            for (auto const& binding : bindings)
            {
                frame::common::hashCombineResource(result, binding.first);
                frame::common::hashCombineResource(result, binding.second);
            }
            return result;
        }
    };

    template <typename T>
    struct hash<std::vector<T>>
    {
        size_t operator()(std::vector<T> const& values) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, values.size());
            for (auto const& value : values)
            {
                frame::common::hashCombineResource(result, value);
            }
            return result;
        }
    };

    template <typename T>
    struct hash<frame::core::VulkanResource<T>>
    {
        size_t operator()(const frame::core::VulkanResource<T>& vulkan_resource) const
        {
            return std::hash<T>{}(vulkan_resource.getHandle());
        }
    };

    template <>
    struct hash<frame::core::ShaderResource>
    {
        size_t operator()(frame::core::ShaderResource const& shader_resource) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, shader_resource.stages);
            frame::common::hashCombineResource(result, shader_resource.type);
            frame::common::hashCombineResource(result, shader_resource.mode);
            frame::common::hashCombineResource(result, shader_resource.set);
            frame::common::hashCombineResource(result, shader_resource.binding);
            frame::common::hashCombineResource(result, shader_resource.location);
            frame::common::hashCombineResource(result, shader_resource.input_attachment_index);
            frame::common::hashCombineResource(result, shader_resource.vec_size);
            frame::common::hashCombineResource(result, shader_resource.columns);
            frame::common::hashCombineResource(result, shader_resource.array_size);
            frame::common::hashCombineResource(result, shader_resource.offset);
            frame::common::hashCombineResource(result, shader_resource.size);
            frame::common::hashCombineResource(result, shader_resource.constant_id);
            frame::common::hashCombineResource(result, shader_resource.qualifiers);
            frame::common::hashCombineResource(result, shader_resource.name);
            return result;
        }
    };

    template <>
    struct hash<frame::core::SubpassInfo>
    {
        size_t operator()(frame::core::SubpassInfo const& subpass_info) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, subpass_info.input_attachments);
            frame::common::hashCombineResource(result, subpass_info.output_attachments);
            frame::common::hashCombineResource(result, subpass_info.color_resolve_attachments);
            frame::common::hashCombineResource(result, subpass_info.disable_depth_stencil_attachment);
            frame::common::hashCombineResource(result, subpass_info.depth_stencil_resolve_attachment);
            frame::common::hashCombineResource(result, subpass_info.depth_stencil_resolve_mode);
            frame::common::hashCombineResource(result, subpass_info.debug_name);
            return result;
        }
    };

    template <>
    struct hash<frame::core::ImageCPP>
    {
        size_t operator()(const frame::core::ImageCPP& image) const
        {
            size_t result = 0;
            if(!image.isExternal()) {
	            frame::common::hashCombineResource(result, image.getMemory());
            }
            frame::common::hashCombineResource(result, image.getType());
            frame::common::hashCombineResource(result, image.getExtent());
            frame::common::hashCombineResource(result, image.getFormat());
            frame::common::hashCombineResource(result, image.getUsage());
            frame::common::hashCombineResource(result, image.getSampleCount());
            frame::common::hashCombineResource(result, image.getTiling());
            frame::common::hashCombineResource(result, image.getSubresource());
            frame::common::hashCombineResource(result, image.getArrayLayerCount());
            return result;
        }
    };

    template <>
    struct hash<frame::core::ImageViewCPP>
    {
        size_t operator()(const frame::core::ImageViewCPP& image_view) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, image_view.getHandle());
            frame::common::hashCombineResource(result, image_view.getImage());
            frame::common::hashCombineResource(result, image_view.getFormat());
            frame::common::hashCombineResource(result, image_view.getSubresourceRange());
            return result;
        }
    };

    template <>
    struct hash<frame::core::ShaderModuleCPP>
    {
        std::size_t operator()(const frame::core::ShaderModuleCPP& shader_module) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, shader_module.getId());
            return result;
        }
    };

    template <>
    struct hash<frame::core::DescriptorSetLayoutCPP>
    {
        std::size_t operator()(const frame::core::DescriptorSetLayoutCPP& descriptor_set_layout) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, descriptor_set_layout.getHandle());
            return result;
        }
    };

    template <>
    struct hash<frame::core::DescriptorSetCPP>
    {
        size_t operator()(frame::core::DescriptorSetCPP& descriptor_set) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, descriptor_set.getHandle());
            frame::common::hashCombineResource(result, descriptor_set.getLayout());
            frame::common::hashCombineResource(result, descriptor_set.getBufferInfos());
            frame::common::hashCombineResource(result, descriptor_set.getImageInfos());
            return result;
        }
    };

    template <>
    struct hash<frame::core::DescriptorPoolCPP>
    {
        std::size_t operator()(const frame::core::DescriptorPoolCPP& descriptor_pool) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, descriptor_pool.getDescriptorSetLayout());
            return result;
        }
    };

    template <>
    struct hash<frame::core::PipelineCPP> {
        std::size_t operator()(const frame::core::PipelineCPP& pipeline) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, pipeline.getHandle());
            return result;
        }
    };

    template <>
    struct hash<frame::core::PipelineLayoutCPP>
    {
        std::size_t operator()(const frame::core::PipelineLayoutCPP& pipeline_layout) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, pipeline_layout.getHandle());
            return result;
        }
    };

    template <>
    struct hash<frame::core::RenderPassCPP>
    {
        std::size_t operator()(const frame::core::RenderPassCPP& render_pass) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, render_pass.getHandle());
            return result;
        }
    };

    template <>
    struct hash<frame::core::ShaderSource>
    {
        std::size_t operator()(const frame::core::ShaderSource& shader_source) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, shader_source.getId());
            return result;
        }
    };

    template <>
    struct hash<frame::core::ShaderVariant>
    {
        std::size_t operator()(const frame::core::ShaderVariant& shader_variant) const
        {
            std::size_t result = 0;
            frame::common::hashCombineResource(result, shader_variant.getId());
            return result;
        }
    };

    template <>
    struct hash<frame::rendering::LoadStoreInfo>
    {
        size_t operator()(frame::rendering::LoadStoreInfo const& lsi) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, lsi.m_load_op);
            frame::common::hashCombineResource(result, lsi.m_store_op);
            return result;
        }
    };

    template <>
    struct hash<frame::rendering::RenderTarget>
    {
        size_t operator()(const frame::rendering::RenderTarget& render_target) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, render_target.getExtent());
            for (auto const& view : render_target.getViews())
            {
                frame::common::hashCombineResource(result, view);
            }
            for (auto const& attachment : render_target.getAttachments())
            {
                frame::common::hashCombineResource(result, attachment);
            }
            for (auto const& input : render_target.getInputAttachments())
            {
                frame::common::hashCombineResource(result, input);
            }
            for (auto const& output : render_target.getOutputAttachments())
            {
                frame::common::hashCombineResource(result, output);
            }
            return result;
        }
    };

    template <>
    struct hash<frame::rendering::PipelineState>
    {
        std::size_t operator()(const frame::rendering::PipelineState& pipeline_state) const
        {
            std::size_t result = 0;

            frame::common::hashCombineResource(result, pipeline_state.getPipelineLayout().getHandle());

            if (auto render_pass = pipeline_state.getRenderPass())
            {
                frame::common::hashCombineResource(result, render_pass->getHandle());
            }

            frame::common::hashCombineResource(result, pipeline_state.getSpecializationConstantState());

            frame::common::hashCombineResource(result, pipeline_state.getSubpassIndex());

            for (auto shader_module : pipeline_state.getPipelineLayout().getShaderModules())
            {
                frame::common::hashCombineResource(result, shader_module->getId());
            }

            for (auto& attribute : pipeline_state.getVertexInputState().attributes)
            {
                frame::common::hashCombineResource(result, attribute);
            }

            for (auto& binding : pipeline_state.getVertexInputState().bindings)
            {
                frame::common::hashCombineResource(result, binding);
            }

            frame::common::hashCombineResource(result, pipeline_state.getInputAssemblyState().primitive_restart_enable);
            frame::common::hashCombineResource(result, pipeline_state.getInputAssemblyState().topology);

            frame::common::hashCombineResource(result, pipeline_state.getViewportState().viewport_count);
            frame::common::hashCombineResource(result, pipeline_state.getViewportState().scissor_count);

            frame::common::hashCombineResource(result, pipeline_state.getRasterizationState().cull_mode);
            frame::common::hashCombineResource(result, pipeline_state.getRasterizationState().depth_bias_enable);
            frame::common::hashCombineResource(result, pipeline_state.getRasterizationState().depth_clamp_enable);
            frame::common::hashCombineResource(result, pipeline_state.getRasterizationState().front_face);
            frame::common::hashCombineResource(result, pipeline_state.getRasterizationState().polygon_mode);
            frame::common::hashCombineResource(result, pipeline_state.getRasterizationState().rasterizer_discard_enable);

            frame::common::hashCombineResource(result, pipeline_state.getMultisampleState().alpha_to_coverage_enable);
            frame::common::hashCombineResource(result, pipeline_state.getMultisampleState().alpha_to_one_enable);
            frame::common::hashCombineResource(result, pipeline_state.getMultisampleState().min_sample_shading);
            frame::common::hashCombineResource(result, pipeline_state.getMultisampleState().rasterization_samples);
            frame::common::hashCombineResource(result, pipeline_state.getMultisampleState().sample_shading_enable);
            frame::common::hashCombineResource(result, pipeline_state.getMultisampleState().sample_mask);

            frame::common::hashCombineResource(result, pipeline_state.getDepthStencilState().back);
            frame::common::hashCombineResource(result, pipeline_state.getDepthStencilState().depth_bounds_test_enable);
            frame::common::hashCombineResource(result, pipeline_state.getDepthStencilState().depth_compare_op);
            frame::common::hashCombineResource(result, pipeline_state.getDepthStencilState().depth_test_enable);
            frame::common::hashCombineResource(result, pipeline_state.getDepthStencilState().depth_write_enable);
            frame::common::hashCombineResource(result, pipeline_state.getDepthStencilState().front);
            frame::common::hashCombineResource(result, pipeline_state.getDepthStencilState().stencil_test_enable);

            frame::common::hashCombineResource(result, pipeline_state.getColorBlendState().logic_op);
            frame::common::hashCombineResource(result, pipeline_state.getColorBlendState().logic_op_enable);

            for (auto& attachment : pipeline_state.getColorBlendState().attachments)
            {
                frame::common::hashCombineResource(result, attachment);
            }

            return result;
        }
    };

    template <>
    struct hash<frame::rendering::SpecializationConstantState>
    {
        std::size_t operator()(const frame::rendering::SpecializationConstantState& specialization_constant_state) const
        {
            std::size_t result = 0;

            for (auto constants : specialization_constant_state.getSpecializationConstantState())
            {
                frame::common::hashCombineResource(result, constants.first);
                for (const auto data : constants.second)
                {
                    frame::common::hashCombineResource(result, data);
                }
            }

            return result;
        }
    };

    template <>
    struct hash<frame::rendering::StencilOpState>
    {
        std::size_t operator()(const frame::rendering::StencilOpState& stencil) const
        {
            std::size_t result = 0;

            frame::common::hashCombineResource(result, stencil.compare_op);
            frame::common::hashCombineResource(result, stencil.depth_fail_op);
            frame::common::hashCombineResource(result, stencil.fail_op);
            frame::common::hashCombineResource(result, stencil.pass_op);

            return result;
        }
    };

    template <>
    struct hash<frame::rendering::ColorBlendAttachmentState>
    {
        std::size_t operator()(const frame::rendering::ColorBlendAttachmentState& color_blend_attachment) const
        {
            std::size_t result = 0;

            frame::common::hashCombineResource(result, color_blend_attachment.alpha_blend_op);
            frame::common::hashCombineResource(result, color_blend_attachment.blend_enable);
            frame::common::hashCombineResource(result, color_blend_attachment.color_blend_op);
            frame::common::hashCombineResource(result, color_blend_attachment.color_write_mask);
            frame::common::hashCombineResource(result, color_blend_attachment.dst_alpha_blend_factor);
            frame::common::hashCombineResource(result, color_blend_attachment.dst_color_blend_factor);
            frame::common::hashCombineResource(result, color_blend_attachment.src_alpha_blend_factor);
            frame::common::hashCombineResource(result, color_blend_attachment.src_color_blend_factor);

            return result;
        }
    };

    template <>
    struct hash<frame::rendering::Attachment>
    {
        size_t operator()(const frame::rendering::Attachment& attachment) const
        {
            size_t result = 0;
            frame::common::hashCombineResource(result, attachment.format);
            frame::common::hashCombineResource(result, attachment.samples);
            frame::common::hashCombineResource(result, attachment.usage);
            frame::common::hashCombineResource(result, attachment.initial_layout);
            return result;
        }
    };
}


namespace frame {
    namespace common {

        template <typename T>
        inline void hashParam(size_t& seed, const T& value)
        {
            hashCombineResource(seed, value);
        }

        template <>
        inline void hashParam(size_t&/*seed*/, const vk::PipelineCache&/*value*/)
        {
            // Pipeline cache doesn't contribute to the hash
        }

        template <typename T, typename... Args>
        inline void hashParam(size_t& seed, const T& first_arg, const Args &...args)
        {
            hashParam(seed, first_arg);
            hashParam(seed, args...);
        }

        template <class T, class... A>
        struct RecordHelper
        {
            size_t record(core::ResourceRecord&/*recorder*/, A &.../*args*/)
            {
                return 0;
            }

            void index(core::ResourceRecord&/*recorder*/, size_t /*index*/, T&/*resource*/)
            {
            }
        };

        template <class... A>
        struct RecordHelper<core::ShaderModuleCPP, A...>
        {
            size_t record(core::ResourceRecord& recorder, A &...args)
            {
                return recorder.registerShaderModule(args...);
            }

            void index(core::ResourceRecord& recorder, size_t index, core::ShaderModuleCPP& shader_module)
            {
                recorder.setShaderModule(index, shader_module);
            }
        };

        template <class... A>
        struct RecordHelper<core::PipelineLayoutCPP, A...>
        {
            size_t record(core::ResourceRecord& recorder, A &...args)
            {
                return recorder.registerPipelineLayout(args...);
            }

            void index(core::ResourceRecord& recorder, size_t index, core::PipelineLayoutCPP& pipeline_layout)
            {
                recorder.setPipelineLayout(index, pipeline_layout);
            }
        };

        template <class... A>
        struct RecordHelper<core::RenderPassCPP, A...>
        {
            size_t record(core::ResourceRecord& recorder, A &...args)
            {
                return recorder.registerRenderPass(args...);
            }

            void index(core::ResourceRecord& recorder, size_t index, core::RenderPassCPP& render_pass)
            {
                recorder.setRenderPass(index, render_pass);
            }
        };

        template <class... A>
        struct RecordHelper<core::GraphicsPipelineCPP, A...>
        {
            size_t record(core::ResourceRecord& recorder, A &...args)
            {
                return recorder.registerGraphicsPipeline(args...);
            }

            void index(core::ResourceRecord& recorder, size_t index, core::GraphicsPipelineCPP& graphics_pipeline)
            {
                recorder.setGraphicsPipeline(index, graphics_pipeline);
            }
        };

        template <class T, class... A>
        T& requestResources(core::Device& device, core::ResourceRecord* recorder, std::unordered_map<size_t, T>& resources, A &...args)
        {
            RecordHelper<T, A...> record_helper;

            size_t hash{ 0U };
            hashParam(hash, args...);

            auto res_it = resources.find(hash);

            if (res_it != resources.end())
            {
                return res_it->second;
            }

            const char* res_type = typeid(T).name();
            size_t res_id = resources.size();

            LOGD("Building #{} cache object ({})", res_id, res_type);
#ifndef VK_DEBUG
            try {
#endif
                T resource(device, args...);

                auto res_ins_it = resources.emplace(hash, std::move(resource));

                if (!res_ins_it.second)
                {
                    throw std::runtime_error{ std::string{"Insertion error for #"} + std::to_string(res_id) + " cache object (" + res_type + ")" };
                }

                res_it = res_ins_it.first;

                if (recorder)
                {
                    size_t index = record_helper.record(*recorder, args...);
                    record_helper.index(*recorder, index, res_it->second);
                }
#ifndef VK_DEBUG
            }
            catch (const std::exception& e) {
                LOGE("Creation error for #{} cache object ({})", res_id, res_type);
                throw e;
            }
#endif
            return res_it->second;
        }
    }
}

#ifndef MemoryBarrier
#define MemoryBarrier __faststorefence
#endif
