/* Copyright (c) 2018-2022, Arm Limited and Contributors
 * Copyright (c) 2020-2022, Broadcom Inc.
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

#include <chrono>
#include <string>

#if defined(VK_USE_PLATFORM_XLIB_KHR)
	#undef None
#endif

namespace frame {
	namespace stats {
		enum class StatIndex {
			frame_times,
			cpu_cycles,
			cpu_instructions,
			cpu_cache_miss_ratio,
			cpu_branch_miss_ratio,
			cpu_l1_accesses,
			cpu_instr_retired,
			cpu_l2_accesses,
			cpu_l3_accesses,
			cpu_bus_reads,
			cpu_bus_writes,
			cpu_mem_reads,
			cpu_mem_writes,
			cpu_ase_spec,
			cpu_vfp_spec,
			cpu_crypto_spec,

			gpu_cycles,
			gpu_vertex_cycles,
			gpu_load_store_cycles,
			gpu_tiles,
			gpu_killed_tiles,
			gpu_fragment_jobs,
			gpu_fragment_cycles,
			gpu_ext_reads,
			gpu_ext_writes,
			gpu_ext_read_stalls,
			gpu_ext_write_stalls,
			gpu_ext_read_bytes,
			gpu_ext_write_bytes,
			gpu_tex_cycles,
		};

		struct StatIndexHash {
			template <typename T>
			std::size_t operator()(T t) const {
				return static_cast<std::size_t>(t);
			}
		};

		enum class StatScaling {
			None,
			ByDeltaTime,
			ByCounter
		};

		enum class CounterSamplingMode {
			Polling,
			Continuous
		};

		struct CounterSamplingConfig {
			CounterSamplingMode mode;
			std::chrono::milliseconds interval{ 1 };
			float speed{ 0.5f };
		};

		class StatGraphData {
		public:
			StatGraphData(const std::string& name,
				const std::string& format,
				float scale_factor = 1.0f,
				bool has_fixed_max = false,
				float max_value = 0.0f);

			StatGraphData() = default;

			std::string m_name;
			std::string m_format;
			float m_scale_factor;
			bool m_has_fixed_max;
			float m_max_value;
		};

		inline StatGraphData::StatGraphData(const std::string& name,
			const std::string& graph_label_format,
			float scale_factor,
			bool has_fixed_max,
			float max_value) :
			m_name(name),
			m_format{ graph_label_format },
			m_scale_factor{ scale_factor },
			m_has_fixed_max{ has_fixed_max },
			m_max_value{ max_value }
		{}
	}
}
