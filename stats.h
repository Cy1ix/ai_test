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

#pragma once

#include <cstdint>
#include <ctime>
#include <future>
#include <map>
#include <set>
#include <vector>

#include "stats/stats_common.h"
#include "stats/stats_provider.h"
#include "utils/timer.h"

namespace frame {

	namespace core {
		class CommandBuffer;
	}

	namespace rendering {
		class RenderContext;
	}

	namespace stats {
		class Stats {
		public:
			explicit Stats(rendering::RenderContext& render_context, size_t buffer_size = 16);
			
			~Stats();
			
			void requestStats(const std::set<StatIndex>& requested_stats,
				CounterSamplingConfig sampling_config = { CounterSamplingMode::Polling });
			
			void resize(size_t width);
			bool isAvailable(StatIndex index) const;
			const StatGraphData& getGraphData(StatIndex index) const;
			
			const std::vector<float>& getData(StatIndex index) const {
				return m_counters.at(index);
			};
			
			const std::set<StatIndex>& getRequestedStats() const {
				return m_requested_stats;
			}
			
			void update(float delta_time);
			void beginSampling(core::CommandBuffer& cb);
			void endSampling(core::CommandBuffer& cb);

		private:
			rendering::RenderContext& m_render_context;
			std::set<StatIndex> m_requested_stats;
			StatsProvider* m_frame_time_provider{ nullptr };
			std::vector<std::unique_ptr<StatsProvider>> m_providers;
			CounterSamplingConfig m_sampling_config;
			size_t m_buffer_size;
			Timer m_main_timer;
			Timer m_worker_timer;
			float m_alpha_smoothing{ 0.2f };
			std::map<StatIndex, std::vector<float>> m_counters{};
			std::thread m_worker_thread;
			std::unique_ptr<std::promise<void>> m_stop_worker;
			std::mutex m_continuous_sampling_mutex;
			std::vector<StatsProvider::Counters> m_continuous_samples;
			bool m_should_add_to_continuous_samples{ false };
			std::vector<StatsProvider::Counters> m_pending_samples;
			float m_fractional_pending_samples{ 0.0f };
			void continuousSamplingWorker(std::future<void> should_terminate);
			void pushSample(const StatsProvider::Counters& sample);
			void profileCounters() const;
		};
	}
}
