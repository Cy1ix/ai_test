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

#include "stats/stats_common.h"

#include <map>
#include <set>
#include <unordered_map>

namespace frame {
	namespace core {
		class CommandBuffer;
	}

	namespace stats {
		class StatsProvider
		{
		public:
			struct Counter {
				double result;
			};

			using Counters = std::unordered_map<StatIndex, Counter, StatIndexHash>;
			
			virtual ~StatsProvider() = default;
			
			virtual bool isAvailable(StatIndex index) const = 0;
			
			virtual const StatGraphData& getGraphData(StatIndex index) const {
				return m_default_graph_map.at(index);
			}
			
			static const StatGraphData& defaultGraphData(StatIndex index);
			
			virtual Counters sample(float delta_time) = 0;
			
			virtual Counters continuousSample(float delta_time) {
				return Counters();
			}
			
			virtual void beginSampling(core::CommandBuffer& cb) {}

			virtual void endSampling(core::CommandBuffer& cb) {}

		protected:
			static std::map<StatIndex, StatGraphData> m_default_graph_map;
		};
	}
}
