/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2019-2024, Sascha Willems
 * Copyright (c) 2024, Mobica Limited
 * Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "vma/vk_mem_alloc.h"
#include <vulkan/vulkan.hpp>

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000

template <class T>
using ShaderStageMap = std::map<vk::ShaderStageFlagBits, T>;

template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

namespace frame {

    namespace rendering {
        struct LoadStoreInfo;
    }

    namespace common {

        enum class ShaderSourceLanguage {
            GLSL,
            HLSL,
            SPV,
        };

        enum class ShadingLanguage {
            GLSL,
            HLSL,
        };

        struct BufferMemoryBarrier {
            vk::PipelineStageFlags m_src_stage_mask{ vk::PipelineStageFlagBits::eBottomOfPipe };
            vk::PipelineStageFlags m_dst_stage_mask{ vk::PipelineStageFlagBits::eTopOfPipe };
            vk::AccessFlags m_src_access_mask{ 0 };
            vk::AccessFlags m_dst_access_mask{ 0 };
        };

        struct ImageMemoryBarrier {
            vk::PipelineStageFlags m_src_stage_mask{ vk::PipelineStageFlagBits::eBottomOfPipe };
            vk::PipelineStageFlags m_dst_stage_mask{ vk::PipelineStageFlagBits::eTopOfPipe };
            vk::AccessFlags m_src_access_mask{ 0 };
            vk::AccessFlags m_dst_access_mask{ 0 };
            vk::ImageLayout m_old_layout{ vk::ImageLayout::eUndefined };
            vk::ImageLayout m_new_layout{ vk::ImageLayout::eUndefined };
            uint32_t m_old_queue_family{ VK_QUEUE_FAMILY_IGNORED };
            uint32_t m_new_queue_family{ VK_QUEUE_FAMILY_IGNORED };
        };

        vk::Extent3D to3D(vk::Extent2D extent);

        bool isDepthOnlyFormat(vk::Format format);

        bool isDepthStencilFormat(vk::Format format);

        bool isDepthFormat(vk::Format format);

        vk::Format getSuitableDepthFormat(vk::PhysicalDevice physical_device,
            bool depth_only = false,
            const std::vector<vk::Format>& depth_format_priority_list = {
                vk::Format::eD24UnormS8Uint,
                vk::Format::eD32Sfloat,
                vk::Format::eD16Unorm });

        vk::Format chooseBlendableFormat(vk::PhysicalDevice physical_device,
            const std::vector<vk::Format>& format_priority_list);

        vk::ShaderStageFlagBits findShaderStage(const std::string& ext);

        void makeFiltersValid(vk::PhysicalDevice physical_device,
            vk::Format format,
            vk::Filter* filter,
            vk::SamplerMipmapMode* mipmap_mode = nullptr);

        bool isDynamicBufferDescriptorType(vk::DescriptorType descriptor_type);

        bool isBufferDescriptorType(vk::DescriptorType descriptor_type);

        int32_t getBitsPerPixel(vk::Format format);

        vk::ShaderModule loadShader(const std::string& filename,
            vk::Device device,
            vk::ShaderStageFlagBits stage,
            ShaderSourceLanguage src_language = ShaderSourceLanguage::GLSL);

        vk::AccessFlags getAccessFlags(vk::ImageLayout layout);

        vk::PipelineStageFlags getPipelineStageFlags(vk::ImageLayout layout);

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            vk::Image image,
            vk::PipelineStageFlags src_stage_mask,
            vk::PipelineStageFlags dst_stage_mask,
            vk::AccessFlags src_access_mask,
            vk::AccessFlags dst_access_mask,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            const vk::ImageSubresourceRange& subresource_range);

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            vk::Image image,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            const vk::ImageSubresourceRange& subresource_range);

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            vk::Image image,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout);

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            const std::vector<std::pair<vk::Image, vk::ImageSubresourceRange>>& images_and_ranges,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout);

        std::vector<vk::ImageCompressionFixedRateFlagBitsEXT> fixedRateCompressionFlagsToVector(
            vk::ImageCompressionFixedRateFlagsEXT flags);

        vk::ImageCompressionPropertiesEXT querySupportedFixedRateCompression(
            vk::PhysicalDevice gpu,
            const vk::ImageCreateInfo& create_info);

        vk::ImageCompressionPropertiesEXT queryAppliedCompression(
            vk::Device device,
            vk::Image image);

        vk::SurfaceFormatKHR selectSurfaceFormat(
            vk::PhysicalDevice gpu,
            vk::SurfaceKHR surface,
            const std::vector<vk::Format>& preferred_formats = {
                vk::Format::eR8G8B8A8Srgb,
                vk::Format::eB8G8R8A8Srgb,
                vk::Format::eA8B8G8R8SrgbPack32 });

        vk::CommandBuffer allocateCommandBuffer(
            vk::Device device,
            vk::CommandPool command_pool,
            vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

        vk::DescriptorSet allocateDescriptorSet(
            vk::Device device,
            vk::DescriptorPool descriptor_pool,
            vk::DescriptorSetLayout descriptor_set_layout);

        vk::Framebuffer createFramebuffer(
            vk::Device device,
            vk::RenderPass render_pass,
            const std::vector<vk::ImageView>& attachments,
            const vk::Extent2D& extent);

        vk::Pipeline createGraphicsPipeline(
            vk::Device device,
            vk::PipelineCache pipeline_cache,
            const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages,
            const vk::PipelineVertexInputStateCreateInfo& vertex_input_state,
            vk::PrimitiveTopology primitive_topology,
            uint32_t patch_control_points,
            vk::PolygonMode polygon_mode,
            vk::CullModeFlags cull_mode,
            vk::FrontFace front_face,
            const std::vector<vk::PipelineColorBlendAttachmentState>& blend_attachment_states,
            const vk::PipelineDepthStencilStateCreateInfo& depth_stencil_state,
            vk::PipelineLayout pipeline_layout,
            vk::RenderPass render_pass);

        vk::ImageView createImageViewCPP(
            vk::Device device,
            vk::Image image,
            vk::ImageViewType view_type,
            vk::Format format,
            vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor,
            uint32_t base_mip_level = 0,
            uint32_t level_count = 1,
            uint32_t base_array_layer = 0,
            uint32_t layer_count = 1);

        vk::QueryPool createQueryPool(
            vk::Device device,
            vk::QueryType query_type,
            uint32_t query_count,
            vk::QueryPipelineStatisticFlags pipeline_statistics = {});

        vk::Sampler createSampler(
            vk::Device device,
            vk::Filter mag_filter,
            vk::Filter min_filter,
            vk::SamplerMipmapMode mipmap_mode,
            vk::SamplerAddressMode address_mode,
            float max_anisotropy,
            float max_lod);

        vk::Sampler createSampler(
            vk::PhysicalDevice gpu,
            vk::Device device,
            vk::Format format,
            vk::Filter filter,
            vk::SamplerAddressMode address_mode,
            float max_anisotropy,
            float max_lod);

        vk::ImageAspectFlags getImageCPPAspectFlags(
            vk::ImageUsageFlagBits usage,
            vk::Format format);

        void submitAndWait(
            vk::Device device,
            vk::Queue queue,
            std::vector<vk::CommandBuffer> command_buffers,
            std::vector<vk::Semaphore> semaphores = {});

        std::vector<rendering::LoadStoreInfo> getLoadAllStoreSwapchain();

        std::vector<rendering::LoadStoreInfo> getClearAllStoreSwapchain();

        std::vector<rendering::LoadStoreInfo> getClearStoreAll();

        std::vector<vk::ClearValue> getClearValue();
    }
}
