/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/descriptor_set.h"
#include "core/framebuffer.h"
#include "core/pipeline_layout.h"
#include "core/render_pass.h"
#include "core/resource_record.h"
#include "core/resource_replay.h"
#include <vulkan/vulkan.hpp>

namespace frame {
	namespace rendering {
		class RenderTarget;
	}
	namespace core {
		class ComputePipelineCPP;
		class DescriptorPoolCPP;
		class DescriptorSetLayoutCPP;
		class ImageViewCPP;

		struct ResourceCacheState {
			std::unordered_map<std::size_t, ShaderModuleCPP> shader_modules;
			std::unordered_map<std::size_t, RenderPassCPP> render_passes;
			std::unordered_map<std::size_t, PipelineLayoutCPP> pipeline_layouts;
			std::unordered_map<std::size_t, GraphicsPipelineCPP> graphics_pipelines;
			std::unordered_map<std::size_t, ComputePipelineCPP> compute_pipelines;
			std::unordered_map<std::size_t, FramebufferCPP> framebuffers;
			std::unordered_map<std::size_t, DescriptorPoolCPP> descriptor_pools;
			std::unordered_map<std::size_t, DescriptorSetCPP> descriptor_sets;
			std::unordered_map<std::size_t, DescriptorSetLayoutCPP> descriptor_set_layouts;
		};
		
		class ResourceCache {
		public:
			ResourceCache(Device& device);

			ResourceCache(const ResourceCache&) = delete;
			ResourceCache(ResourceCache&&) = delete;
			ResourceCache& operator=(const ResourceCache&) = delete;
			ResourceCache& operator=(ResourceCache&&) = delete;

			void clear();
			void clearFramebuffers();
			void clearPipelines();
			const ResourceCacheState& getInternalState() const;
			ComputePipelineCPP& requestComputePipeline(rendering::PipelineState& pipeline_state);
			DescriptorSetCPP& requestDescriptorSet(DescriptorSetLayoutCPP& descriptor_set_layout,
				const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
				const BindingMap<vk::DescriptorImageInfo>& image_infos);
			DescriptorSetLayoutCPP& requestDescriptorSetLayoutCPP(const uint32_t set_index,
				const std::vector<ShaderModuleCPP*>& shader_modules,
				const std::vector<ShaderResource>& set_resources);
			FramebufferCPP& requestFramebuffer(const rendering::RenderTarget& render_target, const RenderPassCPP& render_pass);
			GraphicsPipelineCPP& requestGraphicsPipeline(rendering::PipelineState& pipeline_state);
			PipelineLayoutCPP& requestPipelineLayout(const std::vector<ShaderModuleCPP*>& shader_modules);
			RenderPassCPP& requestRenderPass(const std::vector<rendering::Attachment>& attachments,
				const std::vector<rendering::LoadStoreInfo>& load_store_infos,
				const std::vector<SubpassInfo>& subpasses);
			ShaderModuleCPP& requestShaderModule(
				vk::ShaderStageFlagBits stage, const ShaderSource& glsl_source, const ShaderVariant& shader_variant = {});
			std::vector<uint8_t> serialize();
			void setPipelineCache(vk::PipelineCache pipeline_cache);

			void updateDescriptorSets(const std::vector<ImageViewCPP>& old_views, const std::vector<ImageViewCPP>& new_views);

			void warmup(const std::vector<uint8_t>& data);

		private:
			Device& m_device;
			ResourceRecord m_recorder = {};
			ResourceReplay m_replayer = {};
			vk::PipelineCache m_pipeline_cache = nullptr;
			ResourceCacheState m_state = {};
			std::mutex m_descriptor_set_mutex = {};
			std::mutex m_pipeline_layout_mutex = {};
			std::mutex m_shader_module_mutex = {};
			std::mutex m_descriptor_set_layout_mutex = {};
			std::mutex m_graphics_pipeline_mutex = {};
			std::mutex m_render_pass_mutex = {};
			std::mutex m_compute_pipeline_mutex = {};
			std::mutex m_framebuffer_mutex = {};
		};
	}
}