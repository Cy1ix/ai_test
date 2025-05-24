/* Copyright (c) 2020, Broadcom Inc. and Contributors
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

#include "core/query_pool.h"
#include "stats/stats_provider.h"

namespace frame {
	namespace rendering {
		class RenderContext;
	}

	namespace stats {
		class VulkanStatsProvider : public StatsProvider {
		private:
			struct StatData {
				StatScaling scaling;
				uint32_t counter_index;
				uint32_t divisor_counter_index;
				vk::PerformanceCounterStorageKHR storage;
				vk::PerformanceCounterStorageKHR divisor_storage;
				StatGraphData graph_data;

				StatData() = default;

				StatData(uint32_t counter_index, 
					vk::PerformanceCounterStorageKHR storage,
					StatScaling stat_scaling = StatScaling::ByDeltaTime,
					uint32_t divisor_index = std::numeric_limits<uint32_t>::max(),
					vk::PerformanceCounterStorageKHR divisor_storage = vk::PerformanceCounterStorageKHR::eFloat64) :
					scaling(stat_scaling),
					counter_index(counter_index),
					divisor_counter_index(divisor_index),
					storage(storage),
					divisor_storage(divisor_storage)
				{
				}
			};

			struct VendorStat {
				VendorStat(const std::string& name, const std::string& divisor_name = "") :
					name(name),
					divisor_name(divisor_name)
				{
					if (divisor_name != "") {
						scaling = StatScaling::ByCounter;
					}
				}

				void setVendorGraphData(const StatGraphData& data) {
					has_vendor_graph_data = true;
					graph_data = data;
				}

				std::string name;
				StatScaling scaling = StatScaling::ByDeltaTime;
				std::string divisor_name;
				bool has_vendor_graph_data = false;
				StatGraphData graph_data;
			};

			using StatDataMap = std::unordered_map<StatIndex, StatData, StatIndexHash>;
			using VendorStatMap = std::unordered_map<StatIndex, VendorStat, StatIndexHash>;

		public:
			VulkanStatsProvider(std::set<StatIndex>& requested_stats, 
				const CounterSamplingConfig& sampling_config,
				rendering::RenderContext& render_context);
			
			~VulkanStatsProvider();
			
			bool isAvailable(StatIndex index) const override;
			const StatGraphData& getGraphData(StatIndex index) const override;
			Counters sample(float delta_time) override;
			void beginSampling(core::CommandBuffer& cb) override;
			void endSampling(core::CommandBuffer& cb) override;

		private:
			bool isSupported(const CounterSamplingConfig& sampling_config) const;
			bool fillVendorData();
			bool createQueryPools(uint32_t queue_family_index);
			float getBestDeltaTime(float sw_delta_time) const;

		private:
			rendering::RenderContext& m_render_context;
			std::unique_ptr<core::QueryPool> m_query_pool;
			bool m_has_timestamps{ false };
			float m_timestamp_period{ 1.0f };
			std::unique_ptr<core::QueryPool> m_timestamp_pool;
			VendorStatMap m_vendor_data;
			StatDataMap m_stat_data;
			std::vector<uint32_t> m_counter_indices;
			uint32_t m_queries_ready = 0;
		};
	}
}
