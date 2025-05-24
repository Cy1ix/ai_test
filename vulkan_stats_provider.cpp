/* Copyright (c) 2020-2024, Broadcom Inc. and Contributors
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

#include <Volk/volk.h>

#include "core/device.h"

#include "core/command_buffer.h"
#include "rendering/render_context.h"
#include "stats/vulkan_stats_provider.h"

#include <regex>

namespace frame {
	namespace stats {
		VulkanStatsProvider::VulkanStatsProvider(std::set<StatIndex>& requested_stats,
			const CounterSamplingConfig& sampling_config,
			rendering::RenderContext& render_context) :
			m_render_context(render_context)
		{
			if (!isSupported(sampling_config)) {
				return;
			}

			core::Device& device = m_render_context.getDevice();
			const core::PhysicalDevice& gpu = device.getPhysicalDevice();

			m_has_timestamps = gpu.getProperties().limits.timestampComputeAndGraphics;
			m_timestamp_period = gpu.getProperties().limits.timestampPeriod;
			
			uint32_t queue_family_index = device.getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
			
			uint32_t count = 0;
			gpu.enumerateQueueFamilyPerformanceQueryCounters(queue_family_index, count, nullptr, nullptr);

			if (count == 0) {
				return;
			}

			std::vector<vk::PerformanceCounterKHR> counters(count);
			std::vector<vk::PerformanceCounterDescriptionKHR> descs(count);

			for (uint32_t i = 0; i < count; i++) {
				counters[i].sType = vk::StructureType::ePerformanceCounterKHR;
				counters[i].pNext = nullptr;
				descs[i].sType = vk::StructureType::ePerformanceCounterDescriptionKHR;
				descs[i].pNext = nullptr;
			}
			
			gpu.enumerateQueueFamilyPerformanceQueryCounters(queue_family_index, count, counters.data(), descs.data());
			
			if (!fillVendorData()) {
				return;
			}

			bool performance_impact = false;
			
			for (auto& s : m_vendor_data) {
				StatIndex index = s.first;

				if (requested_stats.find(index) == requested_stats.end()) {
					continue;
				}

				VendorStat& init = s.second;
				bool found_ctr = false;
				bool found_div = (init.divisor_name == "");
				uint32_t ctr_idx, div_idx;

				std::regex name_regex(init.name);
				std::regex div_regex(init.divisor_name);

				for (uint32_t i = 0; !(found_ctr && found_div) && i < descs.size(); i++) {
					if (!found_ctr && std::regex_match(descs[i].name.data(), name_regex)) {
						ctr_idx = i;
						found_ctr = true;
					}
					if (!found_div && std::regex_match(descs[i].name.data(), div_regex)) {
						div_idx = i;
						found_div = true;
					}
				}

				if (found_ctr && found_div) {
					if ((descs[ctr_idx].flags & vk::PerformanceCounterDescriptionFlagBitsKHR::ePerformanceImpacting) ||
						(init.divisor_name != "" && descs[div_idx].flags != vk::PerformanceCounterDescriptionFlagBitsKHR::ePerformanceImpacting))
					{
						performance_impact = true;
					}
					
					m_counter_indices.emplace_back(ctr_idx);
					if (init.divisor_name == "") {
						m_stat_data[index] = StatData(ctr_idx, counters[ctr_idx].storage);
					}
					else
					{
						m_counter_indices.emplace_back(div_idx);
						m_stat_data[index] = StatData(ctr_idx, counters[ctr_idx].storage, init.scaling,
							div_idx, counters[div_idx].storage);
					}
				}
			}

			if (performance_impact) {
				LOGW("The collection of performance counters may impact performance");
			}

			if (m_counter_indices.size() == 0) {
				return;
			}
			
			VkAcquireProfilingLockInfoKHR info{};
			info.sType = VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR;
			info.timeout = 2000000000;

			if (vkAcquireProfilingLockKHR(device.getHandle(), &info) != VK_SUCCESS) {
				m_stat_data.clear();
				m_counter_indices.clear();
				LOGW("Profiling lock acquisition timed-out");
				return;
			}
			
			if (!createQueryPools(queue_family_index)) {
				m_stat_data.clear();
				m_counter_indices.clear();
				return;
			}
			
			for (const auto& s : m_stat_data) {
				requested_stats.erase(s.first);
			}
		}

		VulkanStatsProvider::~VulkanStatsProvider() {
			if (m_stat_data.size() > 0) {
				vkReleaseProfilingLockKHR(m_render_context.getDevice().getHandle());
			}
		}

		bool VulkanStatsProvider::fillVendorData() {

			const auto& pd_props = m_render_context.getDevice().getPhysicalDevice().getProperties();

			if (pd_props.vendorID == 0x14E4) {

				LOGI("Using Vulkan performance counters from Broadcom device");
				
				m_vendor_data = {
					{StatIndex::gpu_cycles,          {"cycle_count"}},
					{StatIndex::gpu_vertex_cycles,   {"gpu_vertex_cycles"}},
					{StatIndex::gpu_fragment_cycles, {"gpu_fragment_cycles"}},
					{StatIndex::gpu_fragment_jobs,   {"render_jobs_completed"}},
					{StatIndex::gpu_ext_reads,       {"gpu_mem_reads"}},
					{StatIndex::gpu_ext_writes,      {"gpu_mem_writes"}},
					{StatIndex::gpu_ext_read_bytes,  {"gpu_bytes_read"}},
					{StatIndex::gpu_ext_write_bytes, {"gpu_bytes_written"}},
				};

				m_vendor_data.at(StatIndex::gpu_vertex_cycles).setVendorGraphData({ "Vertex/Coord/User Cycles", "{:4.1f} M/s", static_cast<float>(1e-6) });
				m_vendor_data.at(StatIndex::gpu_fragment_jobs).setVendorGraphData({ "Render Jobs", "{:4.0f}/s" });

				return true;
			}
#if 0
			else if (pd_props.vendorID == xxxx) {
				return true;
			}
#endif
			{
				return false;
			}
		}

		bool VulkanStatsProvider::createQueryPools(uint32_t queue_family_index) {

			core::Device& device = m_render_context.getDevice();
			const core::PhysicalDevice& gpu = device.getPhysicalDevice();
			uint32_t num_framebuffers = static_cast<uint32_t>(m_render_context.getRenderFrames().size());
			
			vk::QueryPoolPerformanceCreateInfoKHR perf_create_info{};
			perf_create_info.sType = vk::StructureType::eQueryPoolPerformanceCreateInfoKHR;
			perf_create_info.queueFamilyIndex = queue_family_index;
			perf_create_info.counterIndexCount = static_cast<uint32_t>(m_counter_indices.size());
			perf_create_info.pCounterIndices = m_counter_indices.data();

			uint32_t passes_needed = gpu.getQueueFamilyPerformanceQueryPasses(perf_create_info);

			if (passes_needed != 1) {
				LOGW("Requested Vulkan stats require multiple passes, we won't collect them");
				return false;
			}
			
			vk::QueryPoolCreateInfo pool_create_info{};
			pool_create_info.sType = vk::StructureType::eQueryPoolCreateInfo;
			pool_create_info.pNext = &perf_create_info;
			pool_create_info.queryType = vk::QueryType::ePerformanceQueryKHR;
			pool_create_info.queryCount = num_framebuffers;

			m_query_pool = std::make_unique<core::QueryPool>(device, pool_create_info);

			if (!m_query_pool) {
				LOGW("Failed to create performance query pool");
				return false;
			}
			
			m_query_pool->hostReset(0, num_framebuffers);

			if (m_has_timestamps) {

				vk::QueryPoolCreateInfo timestamp_pool_create_info{};
				timestamp_pool_create_info.sType = vk::StructureType::eQueryPoolCreateInfo;
				timestamp_pool_create_info.queryType = vk::QueryType::eTimestamp;
				timestamp_pool_create_info.queryCount = num_framebuffers * 2;

				m_timestamp_pool = std::make_unique<core::QueryPool>(device, timestamp_pool_create_info);
			}

			return true;
		}

		bool VulkanStatsProvider::isSupported(const CounterSamplingConfig& sampling_config) const {
			if (sampling_config.mode == CounterSamplingMode::Continuous) {
				return false;
			}

			core::Device& device = m_render_context.getDevice();
			
			if (!(device.isEnabled("VK_KHR_performance_query") && device.isEnabled("VK_EXT_host_query_reset"))) {
				return false;
			}
			
			VkPhysicalDevicePerformanceQueryFeaturesKHR perf_query_features{};
			perf_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR;

			VkPhysicalDeviceFeatures2KHR device_features{};
			device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
			device_features.pNext = &perf_query_features;

			vkGetPhysicalDeviceFeatures2KHR(device.getPhysicalDevice().getHandle(), &device_features);
			if (!perf_query_features.performanceCounterQueryPools) {
				return false;
			}

			return true;
		}

		bool VulkanStatsProvider::isAvailable(StatIndex index) const {
			return m_stat_data.find(index) != m_stat_data.end();
		}

		const StatGraphData& VulkanStatsProvider::getGraphData(StatIndex index) const {

			assert(isAvailable(index) && "VulkanStatsProvider::getGraphData() called with invalid StatIndex");

			const auto& data = m_vendor_data.find(index)->second;
			if (data.has_vendor_graph_data) {
				return data.graph_data;
			}

			return m_default_graph_map[index];
		}

		void VulkanStatsProvider::beginSampling(core::CommandBuffer& command_buffer) {

			uint32_t active_frame_idx = m_render_context.getActiveFrameIndex();

			if (m_timestamp_pool) {
				command_buffer.resetQueryPool(*m_timestamp_pool, active_frame_idx * 2, 1);
				command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *m_timestamp_pool, active_frame_idx * 2);
			}

			if (m_query_pool) {
				command_buffer.beginQuery(*m_query_pool, active_frame_idx, vk::QueryControlFlags{});
			}
		}

		void VulkanStatsProvider::endSampling(core::CommandBuffer& command_buffer) {
			uint32_t active_frame_idx = m_render_context.getActiveFrameIndex();

			if (m_query_pool) {
				vkCmdPipelineBarrier(command_buffer.getHandle(),
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					0, 0, nullptr, 0, nullptr, 0, nullptr);
				command_buffer.endQuery(*m_query_pool, active_frame_idx);

				++m_queries_ready;
			}

			if (m_timestamp_pool) {
				command_buffer.resetQueryPool(*m_timestamp_pool, active_frame_idx * 2 + 1, 1);
				command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *m_timestamp_pool, active_frame_idx * 2 + 1);
			}
		}

		static double getCounterValue(const vk::PerformanceCounterResultKHR& result, vk::PerformanceCounterStorageKHR storage) {
			switch (storage) {
			case vk::PerformanceCounterStorageKHR::eInt32:
				return static_cast<double>(result.int32);
			case vk::PerformanceCounterStorageKHR::eInt64:
				return static_cast<double>(result.int64);
			case vk::PerformanceCounterStorageKHR::eUint32:
				return static_cast<double>(result.uint32);
			case vk::PerformanceCounterStorageKHR::eUint64:
				return static_cast<double>(result.uint64);
			case vk::PerformanceCounterStorageKHR::eFloat32:
				return static_cast<double>(result.float32);
			case vk::PerformanceCounterStorageKHR::eFloat64:
				return (result.float64);
			default:
				assert(0);
				return 0.0;
			}
		}

		float VulkanStatsProvider::getBestDeltaTime(float sw_delta_time) const {

			if (!m_timestamp_pool) {
				return sw_delta_time;
			}

			float delta_time = sw_delta_time;
			
			std::array<uint64_t, 2> timestamps;

			uint32_t active_frame_idx = m_render_context.getActiveFrameIndex();

			vk::Result r = m_timestamp_pool->getResults(active_frame_idx * 2, 2,
				timestamps.size() * sizeof(uint64_t),
				timestamps.data(), sizeof(uint64_t),
				vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64);
			if (r == vk::Result::eSuccess) {
				float elapsed_ns = m_timestamp_period * static_cast<float>(timestamps[1] - timestamps[0]);
				delta_time = elapsed_ns * 0.000000001f;
			}

			return delta_time;
		}

		StatsProvider::Counters VulkanStatsProvider::sample(float delta_time) {
			Counters out;
			if (!m_query_pool || m_queries_ready == 0)
			{
				return out;
			}

			uint32_t active_frame_idx = m_render_context.getActiveFrameIndex();

			vk::DeviceSize stride = sizeof(vk::PerformanceCounterResultKHR) * m_counter_indices.size();

			std::vector<vk::PerformanceCounterResultKHR> results(m_counter_indices.size());

			vk::Result r = m_query_pool->getResults(active_frame_idx, 1,
				results.size() * sizeof(vk::PerformanceCounterResultKHR),
				results.data(), stride, vk::QueryResultFlagBits::eWait);

			if (r != vk::Result::eSuccess) {
				return out;
			}
			
			delta_time = getBestDeltaTime(delta_time);
			
			for (const auto& s : m_stat_data) {
				StatIndex si = s.first;

				bool need_divisor = (m_stat_data[si].scaling == StatScaling::ByCounter);
				double divisor_value = 1.0;
				double value = 0.0;
				bool found_ctr = false, found_div = !need_divisor;

				for (uint32_t i = 0; !(found_ctr && found_div) && i < m_counter_indices.size(); i++) {
					if (s.second.counter_index == m_counter_indices[i]) {
						value = getCounterValue(results[i], m_stat_data[si].storage);
						found_ctr = true;
					}
					if (need_divisor && s.second.divisor_counter_index == m_counter_indices[i]) {
						divisor_value = getCounterValue(results[i], m_stat_data[si].divisor_storage);
						found_div = true;
					}
				}

				if (found_ctr && found_div) {
					if (m_stat_data[si].scaling == StatScaling::ByDeltaTime && delta_time != 0.0) {
						value /= delta_time;
					}
					else if (m_stat_data[si].scaling == StatScaling::ByCounter && divisor_value != 0.0) {
						value /= divisor_value;
					}
					out[si].result = value;
				}
			}
			
			m_query_pool->hostReset(active_frame_idx, 1);

			--m_queries_ready;

			return out;
		}
	}
}
