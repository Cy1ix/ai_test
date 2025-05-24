/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2020-2024, Broadcom Inc.
 * Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "Volk/volk.h"

#include "stats/stats.h"

#include "common/profiling.h"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "core/device.h"
#include "stats/frame_stats_provider.h"
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	#include "hwcpipe_stats_provider.h"
#endif
#include "common/allocator.h"
#include "rendering/render_context.h"
#include "core/command_buffer.h"
#include "vulkan_stats_provider.h"

namespace frame {
	namespace stats {
		Stats::Stats(rendering::RenderContext& render_context, size_t buffer_size) :
			m_render_context(render_context),
			m_buffer_size(buffer_size)
		{
			assert(buffer_size >= 2 && "Buffers size should be greater than 2");
		}

		Stats::~Stats() {
			if (m_stop_worker) {
				m_stop_worker->set_value();
			}

			if (m_worker_thread.joinable()) {
				m_worker_thread.join();
			}
		}

		void Stats::requestStats(const std::set<StatIndex>& wanted_stats, CounterSamplingConfig config) {

			if (m_providers.size() != 0) {
				throw std::runtime_error("Stats must only be requested once");
			}

			m_requested_stats = wanted_stats;
			m_sampling_config = config;
			
			std::set<StatIndex> stats = m_requested_stats;
			
			m_providers.emplace_back(std::make_unique<FrameTimeStatsProvider>(stats));

#ifdef VK_USE_PLATFORM_ANDROID_KHR
			m_providers.emplace_back(std::make_unique<HWCPipeStatsProvider>(stats));
#endif

			rendering::RenderContext& render_context = m_render_context;
			m_providers.emplace_back(std::make_unique<VulkanStatsProvider>(stats, m_sampling_config, render_context));
			
			m_frame_time_provider = m_providers[0].get();

			for (const auto& stat : m_requested_stats) {
				m_counters[stat] = std::vector<float>(m_buffer_size, 0);
			}

			if (m_sampling_config.mode == CounterSamplingMode::Continuous) {

				m_stop_worker = std::make_unique<std::promise<void>>();

				m_worker_thread = std::thread([this] {
					continuousSamplingWorker(m_stop_worker->get_future());
					});
				
				m_alpha_smoothing = 0.6f;
			}

			for (const auto& stat_index : m_requested_stats) {
				if (!isAvailable(stat_index)) {
					LOGW(StatsProvider::defaultGraphData(stat_index).m_name + " : not available");
				}
			}
		}

		void Stats::resize(const size_t width) {

			m_buffer_size = width >> 4;

			for (auto& counter : m_counters) {
				counter.second.resize(m_buffer_size);
				counter.second.shrink_to_fit();
			}
		}

		bool Stats::isAvailable(const StatIndex index) const {

			for (const auto& p : m_providers) {
				if (p->isAvailable(index)) {
					return true;
				}
			}
			return false;
		}

		static void AddSmoothedValue(std::vector<float>& values, float value, float alpha) {

			assert(values.size() >= 2 && "Buffers size should be greater than 2");

			if (values.size() == values.capacity()) {
				std::rotate(values.begin(), values.begin() + 1, values.end());
			}
			
			values.back() = value * alpha + *(values.end() - 2) * (1.0f - alpha);
		}

		void Stats::update(float delta_time) {

			switch (m_sampling_config.mode) {
			case CounterSamplingMode::Polling: {

				StatsProvider::Counters sample;

				for (auto& p : m_providers) {
					auto s = p->sample(delta_time);
					sample.insert(s.begin(), s.end());
				}

				pushSample(sample);
				break;
			}
			case CounterSamplingMode::Continuous: {
				if (m_pending_samples.size() == 0) {

					std::unique_lock<std::mutex> lock(m_continuous_sampling_mutex);

					if (!m_should_add_to_continuous_samples) {
						m_should_add_to_continuous_samples = true;
					}
					else {
						m_should_add_to_continuous_samples = false;
						m_pending_samples.clear();
						std::swap(m_pending_samples, m_continuous_samples);
					}
				}

				if (m_pending_samples.size() == 0) {
					return;
				}

				if (m_pending_samples.size() > 100) {
					std::move(m_pending_samples.end() - 100, m_pending_samples.end(), m_pending_samples.begin());
					m_pending_samples.erase(m_pending_samples.begin() + 100, m_pending_samples.end());

					m_fractional_pending_samples += 1.0f;
				}

				float floating_sample_count = m_sampling_config.speed * delta_time * static_cast<float>(m_buffer_size) + m_fractional_pending_samples;

				m_fractional_pending_samples = floating_sample_count - std::floor(floating_sample_count);

				auto sample_count = static_cast<size_t>(floating_sample_count);

				sample_count = std::max<size_t>(1, std::min<size_t>(sample_count, m_pending_samples.size()));

				StatsProvider::Counters frame_time_sample = m_frame_time_provider->sample(delta_time);

				std::for_each(m_pending_samples.begin(), m_pending_samples.begin() + sample_count,
					[this, frame_time_sample](auto& s) {
						s.insert(frame_time_sample.begin(), frame_time_sample.end());
						this->pushSample(s);
					});
				m_pending_samples.erase(m_pending_samples.begin(), m_pending_samples.begin() + sample_count);

				break;
			}
			}

			profileCounters();
		}

		void Stats::continuousSamplingWorker(std::future<void> should_terminate) {
			m_worker_timer.tick();

			for (auto& p : m_providers) {
				p->continuousSample(0.0f);
			}

			while (should_terminate.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
				auto delta_time = static_cast<float>(m_worker_timer.tick());
				auto interval = std::chrono::duration_cast<std::chrono::duration<float>>(m_sampling_config.interval).count();
				
				if (delta_time < interval) {
					std::this_thread::sleep_for(std::chrono::duration<float>(interval - delta_time));
					delta_time += static_cast<float>(m_worker_timer.tick());
				}
				
				StatsProvider::Counters sample;
				for (auto& p : m_providers) {
					StatsProvider::Counters s = p->continuousSample(delta_time);
					sample.insert(s.begin(), s.end());
				}
				
				{
					std::unique_lock<std::mutex> lock(m_continuous_sampling_mutex);
					if (m_should_add_to_continuous_samples) {
						m_continuous_samples.push_back(sample);
					}
				}
			}
		}

		void Stats::pushSample(const StatsProvider::Counters& sample) {

			for (auto& c : m_counters) {
				StatIndex idx = c.first;
				std::vector<float>& values = c.second;
				
				const auto& smp = sample.find(idx);
				if (smp == sample.end()) {
					continue;
				}

				float measurement = static_cast<float>(smp->second.result);

				AddSmoothedValue(values, measurement, m_alpha_smoothing);
			}
		}

		namespace {
			const char* ToString(StatIndex index) {
				switch (index) {
				case StatIndex::frame_times:
					return "Frame Times (ms)";
				case StatIndex::cpu_cycles:
					return "CPU Cycles (M/s)";
				case StatIndex::cpu_instructions:
					return "CPU Instructions (M/s)";
				case StatIndex::cpu_cache_miss_ratio:
					return "Cache Miss Ratio (%)";
				case StatIndex::cpu_branch_miss_ratio:
					return "Branch Miss Ratio (%)";
				case StatIndex::cpu_l1_accesses:
					return "CPU L1 Accesses (M/s)";
				case StatIndex::cpu_instr_retired:
					return "CPU Instructions Retired (M/s)";
				case StatIndex::cpu_l2_accesses:
					return "CPU L2 Accesses (M/s)";
				case StatIndex::cpu_l3_accesses:
					return "CPU L3 Accesses (M/s)";
				case StatIndex::cpu_bus_reads:
					return "CPU Bus Read Beats (M/s)";
				case StatIndex::cpu_bus_writes:
					return "CPU Bus Write Beats (M/s)";
				case StatIndex::cpu_mem_reads:
					return "CPU Memory Read Instructions (M/s)";
				case StatIndex::cpu_mem_writes:
					return "CPU Memory Write Instructions (M/s)";
				case StatIndex::cpu_ase_spec:
					return "CPU Speculatively Exec. SIMD Instructions (M/s)";
				case StatIndex::cpu_vfp_spec:
					return "CPU Speculatively Exec. FP Instructions (M/s)";
				case StatIndex::cpu_crypto_spec:
					return "CPU Speculatively Exec. Crypto Instructions (M/s)";
				case StatIndex::gpu_cycles:
					return "GPU Cycles (M/s)";
				case StatIndex::gpu_vertex_cycles:
					return "Vertex Cycles (M/s)";
				case StatIndex::gpu_load_store_cycles:
					return "Load Store Cycles (k/s)";
				case StatIndex::gpu_tiles:
					return "Tiles (k/s)";
				case StatIndex::gpu_killed_tiles:
					return "Tiles killed by CRC match (k/s)";
				case StatIndex::gpu_fragment_jobs:
					return "Fragment Jobs (s)";
				case StatIndex::gpu_fragment_cycles:
					return "Fragment Cycles (M/s)";
				case StatIndex::gpu_tex_cycles:
					return "Shader Texture Cycles (k/s)";
				case StatIndex::gpu_ext_reads:
					return "External Reads (M/s)";
				case StatIndex::gpu_ext_writes:
					return "External Writes (M/s)";
				case StatIndex::gpu_ext_read_stalls:
					return "External Read Stalls (M/s)";
				case StatIndex::gpu_ext_write_stalls:
					return "External Write Stalls (M/s)";
				case StatIndex::gpu_ext_read_bytes:
					return "External Read Bytes (MiB/s)";
				case StatIndex::gpu_ext_write_bytes:
					return "External Write Bytes (MiB/s)";
				default:
					return nullptr;
				}
			}
		}

		void Stats::profileCounters() const
		{
#if VK_PROFILING
			static std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();
			std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

			if (now - last_time < std::chrono::milliseconds(100)) {
				return;
			}

			last_time = now;

			for (auto& c : m_counters) {
				StatIndex idx = c.first;
				auto& graph_data = getGraphData(idx);

				if (c.second.empty()) {
					continue;
				}

				float average = 0.0f;
				for (auto& v : c.second) {
					average += v;
				}
				average /= c.second.size();

				if (auto* index_name = ToString(idx)) {
					Plot<float>::plot(index_name, average * graph_data.m_scale_factor);
				}
			}

			static std::vector<std::string> labels;

			auto& device = reinterpret_cast<rendering::RenderContext&>(m_render_context).getDevice();
			VmaAllocator allocator = alloc::getMemoryAllocator();

			VmaBudget heap_budgets[VK_MAX_MEMORY_HEAPS];
			vmaGetHeapBudgets(allocator, heap_budgets);
			
			if (labels.size() == 0) {
				VkPhysicalDeviceMemoryProperties memory_properties;
				vkGetPhysicalDeviceMemoryProperties(device.getPhysicalDevice().getHandle(), &memory_properties);

				labels.reserve(memory_properties.memoryHeapCount);

				for (size_t heap = 0; heap < memory_properties.memoryHeapCount; heap++) {
					VkMemoryPropertyFlags flags = memory_properties.memoryHeaps[heap].flags;
					labels.push_back("Heap " + std::to_string(heap) + " " + vk::to_string(vk::MemoryPropertyFlags{ flags }));
				}
			}

			for (size_t heap = 0; heap < labels.size(); heap++) {
				Plot<float, PlotType::eMemory>::plot(labels[heap].c_str(), heap_budgets[heap].usage / (1024.0f * 1024.0f));
			}
#endif
		}

		void Stats::beginSampling(core::CommandBuffer& cb) {

			core::CommandBuffer& command_buffer = cb;
			
			for (auto& p : m_providers) {
				p->beginSampling(command_buffer);
			}
		}

		void Stats::endSampling(core::CommandBuffer& cb) {

			core::CommandBuffer& command_buffer = cb;
			
			for (auto& p : m_providers) {
				p->endSampling(command_buffer);
			}
		}

		const StatGraphData& Stats::getGraphData(StatIndex index) const {
			for (auto& p : m_providers) {
				if (p->isAvailable(index)) {
					return p->getGraphData(index);
				}
			}
			return StatsProvider::defaultGraphData(index);
		}
	}
}
