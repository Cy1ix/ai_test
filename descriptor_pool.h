/* Copyright (c) 2019, Arm Limited and Contributors
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

#include <unordered_map>

#include "common/helper.h"
#include "common/common.h"
#include "core/descriptor_set_layout.h"
#include <functional>
#include <vulkan/vulkan.hpp>

#ifdef MemoryBarrier
#undef MemoryBarrier
#endif
#include <vulkan/vulkan_hash.hpp>

namespace frame {
    namespace core {
        class Device;
        class DescriptorSetLayoutCPP;
        
        class DescriptorPoolCPP {
        public:
            static const uint32_t MAX_SETS_PER_POOL = 16;

            DescriptorPoolCPP(Device& device,
                const DescriptorSetLayoutCPP& descriptor_set_layout,
                uint32_t pool_size = MAX_SETS_PER_POOL);

            DescriptorPoolCPP(const DescriptorPoolCPP&) = delete;
            DescriptorPoolCPP(DescriptorPoolCPP&&) = default;
            ~DescriptorPoolCPP();

            DescriptorPoolCPP& operator=(const DescriptorPoolCPP&) = delete;
            DescriptorPoolCPP& operator=(DescriptorPoolCPP&&) = delete;

            void reset();
            const DescriptorSetLayoutCPP& getDescriptorSetLayout() const;
            void setDescriptorSetLayout(const DescriptorSetLayoutCPP& set_layout);
            vk::DescriptorSet allocate();
            vk::Result free(vk::DescriptorSet descriptor_set);

        private:
            Device& m_device;
            const DescriptorSetLayoutCPP* m_descriptor_set_layout{ nullptr };

            std::vector<vk::DescriptorPoolSize> m_pool_sizes;
            uint32_t m_pool_max_sets{ 0 };

            std::vector<vk::DescriptorPool> m_pools;
            std::vector<uint32_t> m_pool_sets_count;
            uint32_t m_pool_index{ 0 };

            std::unordered_map<vk::DescriptorSet, uint32_t> m_set_pool_mapping;

            uint32_t findAvailablePool(uint32_t search_index);
        };
    }
}

#ifndef MemoryBarrier
#define MemoryBarrier __faststorefence
#endif

