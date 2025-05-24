/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/resource_cache.h"
#include "core/descriptor_set.h"
#include "core/device.h"
#include "core/image_view.h"
#include "common/resource_caching.h"
#include "core/pipeline_layout.h"

namespace frame {
	namespace core {
		namespace {
			template <class T, class... A>
			T& requestResource(
				core::Device& device,
				core::ResourceRecord& recorder, std::mutex& resource_mutex, std::unordered_map<std::size_t, T>& resources, A &...args)
			{
				std::lock_guard<std::mutex> guard(resource_mutex);
				auto& res = common::requestResources(device, &recorder, resources, args...);
				return res;
			}
		}

		ResourceCache::ResourceCache(Device& device) :
			m_device{ device }
		{}

		void ResourceCache::clear() {
			m_state.shader_modules.clear();
			m_state.pipeline_layouts.clear();
			m_state.descriptor_sets.clear();
			m_state.descriptor_set_layouts.clear();
			m_state.render_passes.clear();
			clearPipelines();
			clearFramebuffers();
		}

		void ResourceCache::clearFramebuffers() {
			m_state.framebuffers.clear();
		}

		void ResourceCache::clearPipelines() {
			m_state.graphics_pipelines.clear();
			m_state.compute_pipelines.clear();
		}

		const ResourceCacheState& ResourceCache::getInternalState() const {
			return m_state;
		}

		ComputePipelineCPP& ResourceCache::requestComputePipeline(rendering::PipelineState& pipeline_state) {
			return requestResource(m_device, m_recorder, m_compute_pipeline_mutex, m_state.compute_pipelines, m_pipeline_cache, pipeline_state);
		}

		DescriptorSetCPP& ResourceCache::requestDescriptorSet(DescriptorSetLayoutCPP& descriptor_set_layout,
			const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
			const BindingMap<vk::DescriptorImageInfo>& image_infos)
		{
			auto& descriptor_pool = requestResource(m_device, m_recorder, m_descriptor_set_mutex, m_state.descriptor_pools, descriptor_set_layout);
			return requestResource(m_device, m_recorder, m_descriptor_set_mutex, m_state.descriptor_sets, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
		}

		DescriptorSetLayoutCPP& ResourceCache::requestDescriptorSetLayoutCPP(const uint32_t set_index,
			const std::vector<ShaderModuleCPP*>& shader_modules,
			const std::vector<ShaderResource>& set_resources)
		{
			return requestResource(m_device, m_recorder, m_descriptor_set_layout_mutex, m_state.descriptor_set_layouts, set_index, shader_modules, set_resources);
		}

		FramebufferCPP& ResourceCache::requestFramebuffer(const rendering::RenderTarget& render_target,
			const RenderPassCPP& render_pass)
		{
			return requestResource(m_device, m_recorder, m_framebuffer_mutex, m_state.framebuffers, render_target, render_pass);
		}

		GraphicsPipelineCPP& ResourceCache::requestGraphicsPipeline(rendering::PipelineState& pipeline_state)
		{
			return requestResource(m_device, m_recorder, m_graphics_pipeline_mutex, m_state.graphics_pipelines, m_pipeline_cache, pipeline_state);
		}

		PipelineLayoutCPP& ResourceCache::requestPipelineLayout(const std::vector<ShaderModuleCPP*>& shader_modules)
		{
			return requestResource(m_device, m_recorder, m_pipeline_layout_mutex, m_state.pipeline_layouts, shader_modules);
		}

		RenderPassCPP& ResourceCache::requestRenderPass(const std::vector<rendering::Attachment>& attachments,
			const std::vector<rendering::LoadStoreInfo>& load_store_infos,
			const std::vector<SubpassInfo>& subpasses)
		{
			return requestResource(m_device, m_recorder, m_render_pass_mutex, m_state.render_passes, attachments, load_store_infos, subpasses);
		}

		ShaderModuleCPP& ResourceCache::requestShaderModule(vk::ShaderStageFlagBits stage,
			const ShaderSource& glsl_source,
			const ShaderVariant& shader_variant)
		{
			std::string entry_point{ "main" };
			return requestResource(m_device, m_recorder, m_shader_module_mutex, m_state.shader_modules, stage, glsl_source, entry_point, shader_variant);
		}

		std::vector<uint8_t> ResourceCache::serialize() {
			return m_recorder.getData();
		}

		void ResourceCache::setPipelineCache(vk::PipelineCache pipeline_cache) {
			m_pipeline_cache = pipeline_cache;
		}

		void ResourceCache::updateDescriptorSets(const std::vector<ImageViewCPP>& old_views, const std::vector<ImageViewCPP>& new_views) {

			std::vector<vk::WriteDescriptorSet> set_updates;
			std::set<size_t> matches;

			for (size_t i = 0; i < old_views.size(); ++i) {
				auto& old_view = old_views[i];
				auto& new_view = new_views[i];

				for (auto& kd_pair : m_state.descriptor_sets) {
					auto& key = kd_pair.first;
					auto& descriptor_set = kd_pair.second;

					auto& image_infos = descriptor_set.getImageInfos();

					for (auto& ba_pair : image_infos) {
						auto& binding = ba_pair.first;
						auto& array = ba_pair.second;

						for (auto& ai_pair : array) {
							auto& array_element = ai_pair.first;
							auto& image_info = ai_pair.second;

							if (image_info.imageView == old_view.getHandle()) {
								matches.insert(key);
								image_info.imageView = new_view.getHandle();

								if (auto binding_info = descriptor_set.getLayout().getLayoutBinding(binding))
								{
									vk::WriteDescriptorSet write_descriptor_set(descriptor_set.getHandle(), binding, array_element, binding_info->descriptorType, image_info);
									set_updates.push_back(write_descriptor_set);
								}
								else
								{
									LOGE("Shader layout set does not use image binding at #{}", binding);
								}
							}
						}
					}
				}
			}

			if (!set_updates.empty()) {
				m_device.getHandle().updateDescriptorSets(set_updates, {});
			}

			for (auto& match : matches) {
				auto it = m_state.descriptor_sets.find(match);
				auto descriptor_set = std::move(it->second);
				m_state.descriptor_sets.erase(match);

				size_t new_key = std::hash<DescriptorSetCPP>{}(descriptor_set);
				m_state.descriptor_sets.emplace(new_key, std::move(descriptor_set));
			}
		}

		void ResourceCache::warmup(const std::vector<uint8_t>& data) {
			m_recorder.setData(data);
			m_replayer.play(*this, m_recorder);
		}
	}
}
