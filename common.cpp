/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2019-2024, Sascha Willems
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

#include "common/common.h"
#include "common/glsl_compiler.h"
#include "rendering/pipeline_state.h"
#include "filesystem/filesystem.h"
#include "utils/logger.h"
#include "fmt/format.h"

namespace frame {
    namespace common {

        vk::ShaderStageFlagBits findShaderStage(const std::string& file_name) {

            static const std::map<std::string, vk::ShaderStageFlagBits> shader_stage_map = {
                {"vert", vk::ShaderStageFlagBits::eVertex},
                {"frag", vk::ShaderStageFlagBits::eFragment},
                {"comp", vk::ShaderStageFlagBits::eCompute},
                {"geom", vk::ShaderStageFlagBits::eGeometry},
                {"mesh", vk::ShaderStageFlagBits::eMeshEXT},
                {"rahit", vk::ShaderStageFlagBits::eAnyHitKHR},
                {"rcall", vk::ShaderStageFlagBits::eCallableKHR},
                {"rchit", vk::ShaderStageFlagBits::eClosestHitKHR},
                {"rgen", vk::ShaderStageFlagBits::eRaygenKHR},
                {"rint", vk::ShaderStageFlagBits::eIntersectionKHR},
                {"rmiss", vk::ShaderStageFlagBits::eMissKHR},
                {"task", vk::ShaderStageFlagBits::eTaskEXT},
                {"tesc", vk::ShaderStageFlagBits::eTessellationControl},
                {"tese", vk::ShaderStageFlagBits::eTessellationEvaluation},
                {"glsl", vk::ShaderStageFlagBits::eAll}
            };

            std::string ext = file_name.substr(file_name.find_last_of(".") + 1);

            auto stage_it = shader_stage_map.find(ext);
            if (stage_it == shader_stage_map.end()) {
                throw std::runtime_error("[ShaderCompile] ERROR: File extension " + ext + " does not have a vulkan shader stage.");
            }

            return stage_it->second;
        }

        /*
        constexpr auto getShaderStageMap() noexcept {
            using Pair = std::pair<std::string_view, vk::ShaderStageFlagBits>;

            constexpr std::array main_extensions = {
                Pair{"comp", vk::ShaderStageFlagBits::eCompute},
                Pair{"frag", vk::ShaderStageFlagBits::eFragment},
                Pair{"geom", vk::ShaderStageFlagBits::eGeometry},
                Pair{"tesc", vk::ShaderStageFlagBits::eTessellationControl},
                Pair{"tese", vk::ShaderStageFlagBits::eTessellationEvaluation},
                Pair{"vert", vk::ShaderStageFlagBits::eVertex}
            };

            constexpr std::array extended_extensions = {
        #if VK_ENABLE_BETA_EXTENSIONS
                Pair{"mesh", vk::ShaderStageFlagBits::eMeshEXT},
                Pair{"task", vk::ShaderStageFlagBits::eTaskEXT},
        #endif
        #if VK_KHR_ray_tracing_pipeline
                Pair{"rgen",   vk::ShaderStageFlagBits::eRaygenKHR},
                Pair{"rahit",  vk::ShaderStageFlagBits::eAnyHitKHR},
                Pair{"rchit",  vk::ShaderStageFlagBits::eClosestHitKHR},
                Pair{"rmiss",  vk::ShaderStageFlagBits::eMissKHR},
                Pair{"rint",   vk::ShaderStageFlagBits::eIntersectionKHR},
                Pair{"rcall",  vk::ShaderStageFlagBits::eCallableKHR},
        #endif
            };

            constexpr auto merge = [] {
                std::array<Pair, main_extensions.size() + extended_extensions.size()> merged;
                size_t i = 0;

                for (const auto& p : main_extensions) {
                    merged[i++] = p;
                }

                for (const auto& p : extended_extensions) {
                    merged[i++] = p;
                }

                constexpr auto is_sorted = [](const auto& arr) {
                    for (size_t j = 1; j < arr.size(); ++j)
                        if (arr[j - 1].first >= arr[j].first)
                            return false;
                    return true;
                    };

                static_assert(is_sorted(main_extensions), "Main extensions must be pre-sorted");
                static_assert(is_sorted(merged), "Merged array must be sorted");

                return merged;
                }();

            return merge;
        }

        vk::ShaderStageFlagBits findShaderStage(std::string_view ext) {

            static constexpr auto stage_map = getShaderStageMap();

            const auto it = std::lower_bound(
                stage_map.begin(), stage_map.end(), ext,
                [](const auto& pair, std::string_view e) {
                    return pair.first < e;
                }
            );

            if (it == stage_map.end() || it->first != ext) {

                std::string msg = "Invalid shader extension `" + std::string(ext) + "`. Valid: [";
                for (size_t i = 0; i < stage_map.size(); ++i) {
                    msg += stage_map[i].first;
                    if (i != stage_map.size() - 1) msg += ", ";
                }
                msg += "]";
                throw std::runtime_error(msg);
            }

            return it->second;
        }
        */

        vk::Extent3D to3D(vk::Extent2D extent) {
            return { extent.width, extent.height, 1 };
        }

        bool isDepthOnlyFormat(vk::Format format) {
            return format == vk::Format::eD16Unorm ||
                format == vk::Format::eD32Sfloat;
        }

        bool isDepthStencilFormat(vk::Format format) {
            return format == vk::Format::eD16UnormS8Uint ||
                format == vk::Format::eD24UnormS8Uint ||
                format == vk::Format::eD32SfloatS8Uint;
        }

        bool isDepthFormat(vk::Format format) {
            return isDepthOnlyFormat(format) || isDepthStencilFormat(format);
        }

        vk::Format getSuitableDepthFormat(vk::PhysicalDevice physical_device,
            bool depth_only,
            const std::vector<vk::Format>& depth_format_priority_list)
        {
            vk::Format depth_format = vk::Format::eUndefined;

            for (auto& format : depth_format_priority_list) {
                if (depth_only && !isDepthOnlyFormat(format)) {
                    continue;
                }

                vk::FormatProperties properties = physical_device.getFormatProperties(format);

                if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                    depth_format = format;
                    break;
                }
            }

            if (depth_format != vk::Format::eUndefined) {
                LOGI("Depth format selected: {}", vk::to_string(depth_format));
                return depth_format;
            }

            throw std::runtime_error("[Common] ERROR: No suitable depth format could be determined");
        }

        vk::Format chooseBlendableFormat(vk::PhysicalDevice physical_device,
            const std::vector<vk::Format>& format_priority_list)
        {
            for (const auto& format : format_priority_list) {
                vk::FormatProperties properties = physical_device.getFormatProperties(format);
                if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend) {
                    return format;
                }
            }

            throw std::runtime_error("[Common] ERROR: No suitable blendable format could be determined");
        }

        void makeFiltersValid(vk::PhysicalDevice physical_device,
            vk::Format format,
            vk::Filter* filter,
            vk::SamplerMipmapMode* mipmap_mode)
        {
            if (*filter == vk::Filter::eNearest &&
                (mipmap_mode == nullptr || *mipmap_mode == vk::SamplerMipmapMode::eNearest)) {
                return;
            }

            vk::FormatProperties properties = physical_device.getFormatProperties(format);

            if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
                *filter = vk::Filter::eNearest;
                if (mipmap_mode) {
                    *mipmap_mode = vk::SamplerMipmapMode::eNearest;
                }
            }
        }

        bool isDynamicBufferDescriptorType(vk::DescriptorType descriptor_type) {
            return descriptor_type == vk::DescriptorType::eStorageBufferDynamic ||
                descriptor_type == vk::DescriptorType::eUniformBufferDynamic;
        }

        bool isBufferDescriptorType(vk::DescriptorType descriptor_type) {
            return descriptor_type == vk::DescriptorType::eStorageBuffer ||
                descriptor_type == vk::DescriptorType::eUniformBuffer ||
                isDynamicBufferDescriptorType(descriptor_type);
        }

        int32_t getBitsPerPixel(vk::Format format) {
            switch (format) {
            case vk::Format::eR4G4UnormPack8:
                return 8;
            case vk::Format::eR4G4B4A4UnormPack16:
            case vk::Format::eB4G4R4A4UnormPack16:
            case vk::Format::eR5G6B5UnormPack16:
            case vk::Format::eB5G6R5UnormPack16:
            case vk::Format::eR5G5B5A1UnormPack16:
            case vk::Format::eB5G5R5A1UnormPack16:
            case vk::Format::eA1R5G5B5UnormPack16:
                return 16;
            case vk::Format::eR8Unorm:
            case vk::Format::eR8Snorm:
            case vk::Format::eR8Uscaled:
            case vk::Format::eR8Sscaled:
            case vk::Format::eR8Uint:
            case vk::Format::eR8Sint:
            case vk::Format::eR8Srgb:
                return 8;
            case vk::Format::eR8G8Unorm:
            case vk::Format::eR8G8Snorm:
            case vk::Format::eR8G8Uscaled:
            case vk::Format::eR8G8Sscaled:
            case vk::Format::eR8G8Uint:
            case vk::Format::eR8G8Sint:
            case vk::Format::eR8G8Srgb:
                return 16;
            case vk::Format::eR8G8B8Unorm:
            case vk::Format::eR8G8B8Snorm:
            case vk::Format::eR8G8B8Uscaled:
            case vk::Format::eR8G8B8Sscaled:
            case vk::Format::eR8G8B8Uint:
            case vk::Format::eR8G8B8Sint:
            case vk::Format::eR8G8B8Srgb:
            case vk::Format::eB8G8R8Unorm:
            case vk::Format::eB8G8R8Snorm:
            case vk::Format::eB8G8R8Uscaled:
            case vk::Format::eB8G8R8Sscaled:
            case vk::Format::eB8G8R8Uint:
            case vk::Format::eB8G8R8Sint:
            case vk::Format::eB8G8R8Srgb:
                return 24;
            case vk::Format::eR8G8B8A8Unorm:
            case vk::Format::eR8G8B8A8Snorm:
            case vk::Format::eR8G8B8A8Uscaled:
            case vk::Format::eR8G8B8A8Sscaled:
            case vk::Format::eR8G8B8A8Uint:
            case vk::Format::eR8G8B8A8Sint:
            case vk::Format::eR8G8B8A8Srgb:
            case vk::Format::eB8G8R8A8Unorm:
            case vk::Format::eB8G8R8A8Snorm:
            case vk::Format::eB8G8R8A8Uscaled:
            case vk::Format::eB8G8R8A8Sscaled:
            case vk::Format::eB8G8R8A8Uint:
            case vk::Format::eB8G8R8A8Sint:
            case vk::Format::eB8G8R8A8Srgb:
            case vk::Format::eA8B8G8R8UnormPack32:
            case vk::Format::eA8B8G8R8SnormPack32:
            case vk::Format::eA8B8G8R8UscaledPack32:
            case vk::Format::eA8B8G8R8SscaledPack32:
            case vk::Format::eA8B8G8R8UintPack32:
            case vk::Format::eA8B8G8R8SintPack32:
            case vk::Format::eA8B8G8R8SrgbPack32:
                return 32;
            case vk::Format::eA2R10G10B10UnormPack32:
            case vk::Format::eA2R10G10B10SnormPack32:
            case vk::Format::eA2R10G10B10UscaledPack32:
            case vk::Format::eA2R10G10B10SscaledPack32:
            case vk::Format::eA2R10G10B10UintPack32:
            case vk::Format::eA2R10G10B10SintPack32:
            case vk::Format::eA2B10G10R10UnormPack32:
            case vk::Format::eA2B10G10R10SnormPack32:
            case vk::Format::eA2B10G10R10UscaledPack32:
            case vk::Format::eA2B10G10R10SscaledPack32:
            case vk::Format::eA2B10G10R10UintPack32:
            case vk::Format::eA2B10G10R10SintPack32:
                return 32;
            case vk::Format::eR16Unorm:
            case vk::Format::eR16Snorm:
            case vk::Format::eR16Uscaled:
            case vk::Format::eR16Sscaled:
            case vk::Format::eR16Uint:
            case vk::Format::eR16Sint:
            case vk::Format::eR16Sfloat:
                return 16;
            case vk::Format::eR16G16Unorm:
            case vk::Format::eR16G16Snorm:
            case vk::Format::eR16G16Uscaled:
            case vk::Format::eR16G16Sscaled:
            case vk::Format::eR16G16Uint:
            case vk::Format::eR16G16Sint:
            case vk::Format::eR16G16Sfloat:
                return 32;
            case vk::Format::eR16G16B16Unorm:
            case vk::Format::eR16G16B16Snorm:
            case vk::Format::eR16G16B16Uscaled:
            case vk::Format::eR16G16B16Sscaled:
            case vk::Format::eR16G16B16Uint:
            case vk::Format::eR16G16B16Sint:
            case vk::Format::eR16G16B16Sfloat:
                return 48;
            case vk::Format::eR16G16B16A16Unorm:
            case vk::Format::eR16G16B16A16Snorm:
            case vk::Format::eR16G16B16A16Uscaled:
            case vk::Format::eR16G16B16A16Sscaled:
            case vk::Format::eR16G16B16A16Uint:
            case vk::Format::eR16G16B16A16Sint:
            case vk::Format::eR16G16B16A16Sfloat:
                return 64;
            case vk::Format::eR32Uint:
            case vk::Format::eR32Sint:
            case vk::Format::eR32Sfloat:
                return 32;
            case vk::Format::eR32G32Uint:
            case vk::Format::eR32G32Sint:
            case vk::Format::eR32G32Sfloat:
                return 64;
            case vk::Format::eR32G32B32Uint:
            case vk::Format::eR32G32B32Sint:
            case vk::Format::eR32G32B32Sfloat:
                return 96;
            case vk::Format::eR32G32B32A32Uint:
            case vk::Format::eR32G32B32A32Sint:
            case vk::Format::eR32G32B32A32Sfloat:
                return 128;
            case vk::Format::eR64Uint:
            case vk::Format::eR64Sint:
            case vk::Format::eR64Sfloat:
                return 64;
            case vk::Format::eR64G64Uint:
            case vk::Format::eR64G64Sint:
            case vk::Format::eR64G64Sfloat:
                return 128;
            case vk::Format::eR64G64B64Uint:
            case vk::Format::eR64G64B64Sint:
            case vk::Format::eR64G64B64Sfloat:
                return 192;
            case vk::Format::eR64G64B64A64Uint:
            case vk::Format::eR64G64B64A64Sint:
            case vk::Format::eR64G64B64A64Sfloat:
                return 256;
            case vk::Format::eB10G11R11UfloatPack32:
                return 32;
            case vk::Format::eE5B9G9R9UfloatPack32:
                return 32;
            case vk::Format::eD16Unorm:
                return 16;
            case vk::Format::eX8D24UnormPack32:
                return 32;
            case vk::Format::eD32Sfloat:
                return 32;
            case vk::Format::eS8Uint:
                return 8;
            case vk::Format::eD16UnormS8Uint:
                return 24;
            case vk::Format::eD24UnormS8Uint:
                return 32;
            case vk::Format::eD32SfloatS8Uint:
                return 40;
            case vk::Format::eUndefined:
            default:
                return -1;
            }
        }

        vk::ShaderModule loadShader(const std::string& filename,
            vk::Device device,
            vk::ShaderStageFlagBits stage,
            ShaderSourceLanguage src_language) {
            GLSLCompiler glsl_compiler;

            auto buffer = filesystem::readShader(filename);
            std::vector<uint32_t> spirv;

            if (ShaderSourceLanguage::GLSL == src_language) {
                
                std::string info_log;

                if (!glsl_compiler.compileToSpirv(buffer, spirv, findShaderStage(filename), "main", {}, info_log)) {
                    LOGE("Failed to compile shader, Error: {}", info_log.c_str());
                    return nullptr;
                }
            }
            else if (ShaderSourceLanguage::SPV == src_language) {
                spirv = std::vector<uint32_t>(reinterpret_cast<uint32_t*>(buffer.data()),
                    reinterpret_cast<uint32_t*>(buffer.data()) + buffer.size() / sizeof(uint32_t));
            }
            else {
                LOGE("The format is not supported");
                return nullptr;
            }

            vk::ShaderModuleCreateInfo module_create_info({}, spirv.size() * sizeof(uint32_t), spirv.data());

            return device.createShaderModule(module_create_info);
        }

        vk::AccessFlags getAccessFlags(vk::ImageLayout layout) {
            switch (layout) {
            case vk::ImageLayout::eUndefined:
            case vk::ImageLayout::ePresentSrcKHR:
                return {};
            case vk::ImageLayout::ePreinitialized:
                return vk::AccessFlagBits::eHostWrite;
            case vk::ImageLayout::eColorAttachmentOptimal:
                return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
            case vk::ImageLayout::eDepthAttachmentOptimal:
                return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
                return vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
            case vk::ImageLayout::eTransferSrcOptimal:
                return vk::AccessFlagBits::eTransferRead;
            case vk::ImageLayout::eTransferDstOptimal:
                return vk::AccessFlagBits::eTransferWrite;
            case vk::ImageLayout::eGeneral:
                assert(false && "Don't know how to get meaningful vk::AccessFlags for vk::ImageLayout::eGeneral!");
                return {};
            default:
                assert(false);
                return {};
            }
        }

        vk::PipelineStageFlags getPipelineStageFlags(vk::ImageLayout layout) {
            switch (layout) {
            case vk::ImageLayout::eUndefined:
                return vk::PipelineStageFlagBits::eTopOfPipe;
            case vk::ImageLayout::ePreinitialized:
                return vk::PipelineStageFlagBits::eHost;
            case vk::ImageLayout::eTransferDstOptimal:
            case vk::ImageLayout::eTransferSrcOptimal:
                return vk::PipelineStageFlagBits::eTransfer;
            case vk::ImageLayout::eColorAttachmentOptimal:
                return vk::PipelineStageFlagBits::eColorAttachmentOutput;
            case vk::ImageLayout::eDepthAttachmentOptimal:
                return vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
            case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
                return vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                return vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
            case vk::ImageLayout::ePresentSrcKHR:
                return vk::PipelineStageFlagBits::eBottomOfPipe;
            case vk::ImageLayout::eGeneral:
                assert(false && "Don't know how to get meaningful vk::PipelineStageFlags for vk::ImageLayout::eGeneral!");
                return {};
            default:
                assert(false);
                return {};
            }
        }

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            vk::Image image,
            vk::PipelineStageFlags src_stage_mask,
            vk::PipelineStageFlags dst_stage_mask,
            vk::AccessFlags src_access_mask,
            vk::AccessFlags dst_access_mask,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            const vk::ImageSubresourceRange& subresource_range)
        {
            vk::ImageMemoryBarrier barrier(
                src_access_mask,
                dst_access_mask,
                old_layout,
                new_layout,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                image,
                subresource_range
            );

            command_buffer.pipelineBarrier(
                src_stage_mask,
                dst_stage_mask,
                {},
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            vk::Image image,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            const vk::ImageSubresourceRange& subresource_range) {
            vk::PipelineStageFlags src_stage_mask = getPipelineStageFlags(old_layout);
            vk::PipelineStageFlags dst_stage_mask = getPipelineStageFlags(new_layout);
            vk::AccessFlags src_access_mask = getAccessFlags(old_layout);
            vk::AccessFlags dst_access_mask = getAccessFlags(new_layout);

            imageLayoutTransition(
                command_buffer,
                image,
                src_stage_mask,
                dst_stage_mask,
                src_access_mask,
                dst_access_mask,
                old_layout,
                new_layout,
                subresource_range
            );
        }

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            vk::Image image,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout) {
            vk::ImageSubresourceRange subresource_range(
                vk::ImageAspectFlagBits::eColor,
                0, 1,
                0, 1
            );

            imageLayoutTransition(
                command_buffer,
                image,
                old_layout,
                new_layout,
                subresource_range
            );
        }

        void imageLayoutTransition(vk::CommandBuffer command_buffer,
            const std::vector<std::pair<vk::Image, vk::ImageSubresourceRange>>& images_and_ranges,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout) {
            vk::PipelineStageFlags src_stage_mask = getPipelineStageFlags(old_layout);
            vk::PipelineStageFlags dst_stage_mask = getPipelineStageFlags(new_layout);
            vk::AccessFlags src_access_mask = getAccessFlags(old_layout);
            vk::AccessFlags dst_access_mask = getAccessFlags(new_layout);

            std::vector<vk::ImageMemoryBarrier> barriers;
            barriers.reserve(images_and_ranges.size());

            for (const auto& [image, range] : images_and_ranges) {
                barriers.emplace_back(
                    src_access_mask,
                    dst_access_mask,
                    old_layout,
                    new_layout,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image,
                    range
                );
            }
            
            command_buffer.pipelineBarrier(
                src_stage_mask,
                dst_stage_mask,
                {},
                0, nullptr,
                0, nullptr,
                static_cast<uint32_t>(barriers.size()), 
                barriers.data()
            );
        }

        std::vector<vk::ImageCompressionFixedRateFlagBitsEXT> fixedRateCompressionFlagsToVector(
            vk::ImageCompressionFixedRateFlagsEXT flags) {
            const std::vector<vk::ImageCompressionFixedRateFlagBitsEXT> all_flags = {
                vk::ImageCompressionFixedRateFlagBitsEXT::e1Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e2Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e3Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e4Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e5Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e6Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e7Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e8Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e9Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e10Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e11Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e12Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e13Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e14Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e15Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e16Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e17Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e18Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e19Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e20Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e21Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e22Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e23Bpc,
                vk::ImageCompressionFixedRateFlagBitsEXT::e24Bpc
            };

            std::vector<vk::ImageCompressionFixedRateFlagBitsEXT> result;

            for (const auto& flag : all_flags) {
                if (static_cast<vk::ImageCompressionFixedRateFlagsEXT>(flag) & flags) {
                    result.push_back(flag);
                }
            }

            return result;
        }

        vk::ImageCompressionPropertiesEXT querySupportedFixedRateCompression(
            vk::PhysicalDevice gpu,
            const vk::ImageCreateInfo& create_info) {
            vk::ImageCompressionPropertiesEXT compression_props;
            compression_props.sType = vk::StructureType::eImageCompressionPropertiesEXT;
            compression_props.pNext = nullptr;

            vk::ImageCompressionControlEXT compression_control;
            compression_control.sType = vk::StructureType::eImageCompressionControlEXT;
            compression_control.pNext = nullptr;
            compression_control.flags = vk::ImageCompressionFlagBitsEXT::eFixedRateDefault;

            vk::PhysicalDeviceImageFormatInfo2 format_info;
            format_info.sType = vk::StructureType::ePhysicalDeviceImageFormatInfo2;
            format_info.pNext = &compression_control;
            format_info.format = create_info.format;
            format_info.type = create_info.imageType;
            format_info.tiling = create_info.tiling;
            format_info.usage = create_info.usage;
            format_info.flags = {};

            vk::ImageFormatProperties2 format_props;
            format_props.sType = vk::StructureType::eImageFormatProperties2;
            format_props.pNext = &compression_props;

            vk::Result result = gpu.getImageFormatProperties2KHR(&format_info, &format_props);

            if(result != vk::Result::eSuccess) {
                LOGE("Get image format properties fail");
            }

            return compression_props;
        }

        vk::ImageCompressionPropertiesEXT queryAppliedCompression(
            vk::Device device,
            vk::Image image)
        {
            vk::ImageSubresource2EXT subresource;
            subresource.sType = vk::StructureType::eImageSubresource2KHR;
            subresource.pNext = nullptr;
            subresource.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresource.imageSubresource.mipLevel = 0;
            subresource.imageSubresource.arrayLayer = 0;

            vk::ImageCompressionPropertiesEXT compression_props;
            compression_props.sType = vk::StructureType::eImageCompressionPropertiesEXT;
            compression_props.pNext = nullptr;

            vk::SubresourceLayout2EXT layout;
            layout.sType = vk::StructureType::eSubresourceLayout2KHR;
            layout.pNext = &compression_props;

            device.getImageSubresourceLayout2EXT(image, &subresource, &layout);

            return compression_props;
        }

        vk::SurfaceFormatKHR selectSurfaceFormat(
            vk::PhysicalDevice gpu,
            vk::SurfaceKHR surface,
            const std::vector<vk::Format>& preferred_formats)
        {
            std::vector<vk::SurfaceFormatKHR> available_formats = gpu.getSurfaceFormatsKHR(surface);
            assert(!available_formats.empty());

            auto it = std::find_if(available_formats.begin(), available_formats.end(),
                [&preferred_formats](const vk::SurfaceFormatKHR& fmt) {
                    return std::find(preferred_formats.begin(), preferred_formats.end(),
                        fmt.format) != preferred_formats.end();
                });

            return (it != available_formats.end()) ? *it : available_formats[0];
        }

        vk::CommandBuffer allocateCommandBuffer(
            vk::Device device,
            vk::CommandPool command_pool,
            vk::CommandBufferLevel level) {
            vk::CommandBufferAllocateInfo alloc_info(command_pool, level, 1);
            return device.allocateCommandBuffers(alloc_info).front();
        }

        vk::DescriptorSet allocateDescriptorSet(
            vk::Device device,
            vk::DescriptorPool descriptor_pool,
            vk::DescriptorSetLayout descriptor_set_layout) {
#if defined(ANDROID)
            vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, 1, &descriptor_set_layout);
#else
            vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, descriptor_set_layout);
#endif
            return device.allocateDescriptorSets(alloc_info).front();
        }

        vk::Framebuffer createFramebuffer(
            vk::Device device,
            vk::RenderPass render_pass,
            const std::vector<vk::ImageView>& attachments,
            const vk::Extent2D& extent) {
            vk::FramebufferCreateInfo create_info({}, render_pass, attachments, extent.width, extent.height, 1);
            return device.createFramebuffer(create_info);
        }

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
            vk::RenderPass render_pass)
        {
            vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, primitive_topology, false);

            vk::PipelineTessellationStateCreateInfo tessellation_state({}, patch_control_points);

            vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

            vk::PipelineRasterizationStateCreateInfo rasterization_state;
            rasterization_state.polygonMode = polygon_mode;
            rasterization_state.cullMode = cull_mode;
            rasterization_state.frontFace = front_face;
            rasterization_state.lineWidth = 1.0f;

            vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

            vk::PipelineColorBlendStateCreateInfo color_blend_state({}, false, {}, blend_attachment_states);

            std::array<vk::DynamicState, 2> dynamic_states = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor
            };
            vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_states);

            vk::GraphicsPipelineCreateInfo create_info(
                {},
                shader_stages,
                &vertex_input_state,
                &input_assembly_state,
                &tessellation_state,
                &viewport_state,
                &rasterization_state,
                &multisample_state,
                &depth_stencil_state,
                &color_blend_state,
                &dynamic_state,
                pipeline_layout,
                render_pass
            );

            auto result = device.createGraphicsPipeline(pipeline_cache, create_info);
            assert(result.result == vk::Result::eSuccess);
            return result.value;
        }

        vk::ImageView createImageViewCPP(
            vk::Device device,
            vk::Image image,
            vk::ImageViewType view_type,
            vk::Format format,
            vk::ImageAspectFlags aspect_mask,
            uint32_t base_mip_level,
            uint32_t level_count,
            uint32_t base_array_layer,
            uint32_t layer_count) {
            vk::ImageViewCreateInfo view_info;
            view_info.image = image;
            view_info.viewType = view_type;
            view_info.format = format;
            view_info.subresourceRange.aspectMask = aspect_mask;
            view_info.subresourceRange.baseMipLevel = base_mip_level;
            view_info.subresourceRange.levelCount = level_count;
            view_info.subresourceRange.baseArrayLayer = base_array_layer;
            view_info.subresourceRange.layerCount = layer_count;

            return device.createImageView(view_info);
        }

        vk::QueryPool createQueryPool(
            vk::Device device,
            vk::QueryType query_type,
            uint32_t query_count,
            vk::QueryPipelineStatisticFlags pipeline_statistics) {
            vk::QueryPoolCreateInfo create_info;
            create_info.queryType = query_type;
            create_info.queryCount = query_count;
            create_info.pipelineStatistics = pipeline_statistics;

            return device.createQueryPool(create_info);
        }

        vk::Sampler createSampler(
            vk::Device device,
            vk::Filter mag_filter,
            vk::Filter min_filter,
            vk::SamplerMipmapMode mipmap_mode,
            vk::SamplerAddressMode address_mode,
            float max_anisotropy,
            float max_lod) {
            vk::SamplerCreateInfo create_info;
            create_info.magFilter = mag_filter;
            create_info.minFilter = min_filter;
            create_info.mipmapMode = mipmap_mode;
            create_info.addressModeU = address_mode;
            create_info.addressModeV = address_mode;
            create_info.addressModeW = address_mode;
            create_info.anisotropyEnable = max_anisotropy > 1.0f;
            create_info.maxAnisotropy = max_anisotropy;
            create_info.compareEnable = false;
            create_info.compareOp = vk::CompareOp::eNever;
            create_info.minLod = 0.0f;
            create_info.maxLod = max_lod;
            create_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;

            return device.createSampler(create_info);
        }

        vk::Sampler createSampler(
            vk::PhysicalDevice gpu,
            vk::Device device,
            vk::Format format,
            vk::Filter filter,
            vk::SamplerAddressMode address_mode,
            float max_anisotropy,
            float max_lod)
        {
            vk::FormatProperties fmt_props = gpu.getFormatProperties(format);
            bool has_linear_filter = static_cast<bool>(fmt_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

            vk::Filter actual_filter = has_linear_filter ? filter : vk::Filter::eNearest;
            vk::SamplerMipmapMode mipmap_mode = has_linear_filter ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest;

            return createSampler(device, actual_filter, actual_filter, mipmap_mode, address_mode, max_anisotropy, max_lod);
        }

        vk::ImageAspectFlags getImageCPPAspectFlags(
            vk::ImageUsageFlagBits usage,
            vk::Format format) {
            vk::ImageAspectFlags aspect_flags;

            switch (usage) {
            case vk::ImageUsageFlagBits::eColorAttachment:
                assert(!isDepthFormat(format));
                aspect_flags = vk::ImageAspectFlagBits::eColor;
                break;

            case vk::ImageUsageFlagBits::eDepthStencilAttachment:
                assert(isDepthFormat(format));
                aspect_flags = vk::ImageAspectFlagBits::eDepth;

                if (isDepthStencilFormat(format)) {
                    aspect_flags |= vk::ImageAspectFlagBits::eStencil;
                }
                break;

            default:
                assert(false && "Unsupported image usage");
            }

            return aspect_flags;
        }

        void submitAndWait(
            vk::Device device,
            vk::Queue queue,
            std::vector<vk::CommandBuffer> command_buffers,
            std::vector<vk::Semaphore> semaphores)
        {
            vk::Fence fence = device.createFence({});

            vk::SubmitInfo submit_info;
            submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
            submit_info.pCommandBuffers = command_buffers.data();
            submit_info.signalSemaphoreCount = static_cast<uint32_t>(semaphores.size());
            submit_info.pSignalSemaphores = semaphores.empty() ? nullptr : semaphores.data();

            queue.submit(submit_info, fence);

            vk::Result result = device.waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
            if (result != vk::Result::eSuccess) {
                LOGE("Vulkan error on waitForFences: {}", vk::to_string(result));
                abort();
            }

            device.destroyFence(fence);
        }

        std::vector<rendering::LoadStoreInfo> getLoadAllStoreSwapchain() {

            std::vector<rendering::LoadStoreInfo> load_store(4);

            load_store[0].m_load_op = vk::AttachmentLoadOp::eDontCare;
            load_store[0].m_store_op = vk::AttachmentStoreOp::eStore;

            load_store[1].m_load_op = vk::AttachmentLoadOp::eLoad;
            load_store[1].m_store_op = vk::AttachmentStoreOp::eDontCare;

            load_store[2].m_load_op = vk::AttachmentLoadOp::eLoad;
            load_store[2].m_store_op = vk::AttachmentStoreOp::eDontCare;

            load_store[3].m_load_op = vk::AttachmentLoadOp::eLoad;
            load_store[3].m_store_op = vk::AttachmentStoreOp::eDontCare;

            return load_store;
        }

        std::vector<rendering::LoadStoreInfo> getClearAllStoreSwapchain() {

            std::vector<rendering::LoadStoreInfo> load_store(4);

            load_store[0].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[0].m_store_op = vk::AttachmentStoreOp::eStore;

            load_store[1].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[1].m_store_op = vk::AttachmentStoreOp::eDontCare;

            load_store[2].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[2].m_store_op = vk::AttachmentStoreOp::eDontCare;

            load_store[3].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[3].m_store_op = vk::AttachmentStoreOp::eDontCare;

            return load_store;
        }

        std::vector<rendering::LoadStoreInfo> getClearStoreAll() {

            std::vector<rendering::LoadStoreInfo> load_store(4);

            load_store[0].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[0].m_store_op = vk::AttachmentStoreOp::eStore;

            load_store[1].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[1].m_store_op = vk::AttachmentStoreOp::eStore;

            load_store[2].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[2].m_store_op = vk::AttachmentStoreOp::eStore;

            load_store[3].m_load_op = vk::AttachmentLoadOp::eClear;
            load_store[3].m_store_op = vk::AttachmentStoreOp::eStore;

            return load_store;
        }

        std::vector<vk::ClearValue> getClearValue() {

            std::vector<vk::ClearValue> clear_values(4);

            clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}});

            clear_values[1].depthStencil = vk::ClearDepthStencilValue(0.0f, ~0U);

            clear_values[2].color = vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}});

            clear_values[3].color = vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}});

            return clear_values;
        }
    }
}
