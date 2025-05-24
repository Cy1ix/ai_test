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

#include "core/query_pool.h"
#include "core/device.h"

namespace frame {
    namespace core {
        QueryPool::QueryPool(Device& device, const vk::QueryPoolCreateInfo& info) :
            VulkanResource{ VK_NULL_HANDLE, &device }
        {
            auto result = getDevice().getHandle().createQueryPool(info);
            if(result != VK_NULL_HANDLE) {
                LOGE("Create query pool fail");
                getDevice().getHandle().destroyQueryPool(result);
                throw std::runtime_error("[QueryPool] ERROR: Create query pool fail");
            }
            setHandle(result);
        }

        QueryPool::QueryPool(QueryPool&& other) :
            VulkanResource{ std::move(other) }
        {
            other.setHandle(vk::QueryPool{});
        }

        QueryPool::~QueryPool() {
            if (hasHandle()) {
                getDevice().getHandle().destroyQueryPool(getHandle());
            }
        }
        
        void QueryPool::hostReset(uint32_t first_query, uint32_t query_count) {
            assert(getDevice().isEnabled("VK_EXT_host_query_reset") &&
                "[QueryPool] ASSERT: VK_EXT_host_query_reset needs to be enabled to call QueryPool::hostReset");
            getDevice().getHandle().resetQueryPoolEXT(getHandle(), first_query, query_count);
            
        }

        vk::Result QueryPool::getResults(uint32_t first_query, uint32_t query_count,
            size_t result_bytes, void* results, vk::DeviceSize stride,
            vk::QueryResultFlags flags)
        {
            return getDevice().getHandle().getQueryPoolResults(getHandle(), first_query, query_count, result_bytes, results, stride, flags);
        }
    }
}