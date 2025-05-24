/* Copyright (c) 2020, Broadcom Inc. and Contributors
 * Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/vulkan_resource.h"
#include "common/helper.h"
#include "common/common.h"

namespace frame {
    namespace core {
        class Device;

        class QueryPool : public VulkanResource<vk::QueryPool> {
        public:
            QueryPool(Device& device, const vk::QueryPoolCreateInfo& info);

            QueryPool(const QueryPool&) = delete;
            QueryPool(QueryPool&& pool);
            QueryPool& operator=(const QueryPool&) = delete;
            QueryPool& operator=(QueryPool&&) = delete;

            ~QueryPool();
            
            void hostReset(uint32_t first_query, uint32_t query_count);
            vk::Result getResults(uint32_t first_query, uint32_t num_queries,
                size_t result_bytes, void* results, vk::DeviceSize stride,
                vk::QueryResultFlags flags);
        };
    }
}
