/* Copyright (c) 2019-2022, Arm Limited and Contributors
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

#include "core/descriptor_pool.h"
#include "core/descriptor_set_layout.h"
#include "core/device.h"

namespace frame {
    namespace core {
        DescriptorPoolCPP::DescriptorPoolCPP(Device& device,
            const DescriptorSetLayoutCPP& descriptor_set_layout,
            uint32_t pool_size) :
            m_device{ device },
            m_descriptor_set_layout{ &descriptor_set_layout }
        {
            const auto& bindings = descriptor_set_layout.getBindings();
            std::map<vk::DescriptorType, uint32_t> descriptor_type_counts;

            for (const auto& binding : bindings) {
                descriptor_type_counts[static_cast<vk::DescriptorType>(binding.descriptorType)] += binding.descriptorCount;
            }

            m_pool_sizes.resize(descriptor_type_counts.size());
            auto pool_size_it = m_pool_sizes.begin();

            for (const auto& [type, count] : descriptor_type_counts) {
                pool_size_it->type = type;
                pool_size_it->descriptorCount = count * pool_size;
                ++pool_size_it;
            }

            m_pool_max_sets = pool_size;
        }

        DescriptorPoolCPP::~DescriptorPoolCPP() {
            for (auto pool : m_pools) {
                m_device.getHandle().destroyDescriptorPool(pool);
            }
        }

        void DescriptorPoolCPP::reset() {
            for (auto pool : m_pools) {
                m_device.getHandle().resetDescriptorPool(pool);
            }

            std::fill(m_pool_sets_count.begin(), m_pool_sets_count.end(), 0);
            m_set_pool_mapping.clear();
            m_pool_index = 0;
        }

        const DescriptorSetLayoutCPP& DescriptorPoolCPP::getDescriptorSetLayout() const {
            assert(m_descriptor_set_layout && "Descriptor set layout is invalid");
            return *m_descriptor_set_layout;
        }

        void DescriptorPoolCPP::setDescriptorSetLayout(const DescriptorSetLayoutCPP& set_layout) {
            m_descriptor_set_layout = &set_layout;
        }

        vk::DescriptorSet DescriptorPoolCPP::allocate() {

            m_pool_index = findAvailablePool(m_pool_index);
            ++m_pool_sets_count[m_pool_index];

            vk::DescriptorSetLayout set_layout = getDescriptorSetLayout().getHandle();

            vk::DescriptorSetAllocateInfo alloc_info;
            alloc_info.descriptorPool = m_pools[m_pool_index];
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts = &set_layout;

            try {
                auto descriptor_sets = m_device.getHandle().allocateDescriptorSets(alloc_info);
                auto handle = descriptor_sets[0];

                m_set_pool_mapping.emplace(handle, m_pool_index);

                return handle;
            }
            catch (const std::exception&) {
                --m_pool_sets_count[m_pool_index];
                return nullptr;
            }
        }

        vk::Result DescriptorPoolCPP::free(vk::DescriptorSet descriptor_set) {

            auto it = m_set_pool_mapping.find(descriptor_set);

            if (it == m_set_pool_mapping.end()) {
                return vk::Result::eIncomplete;
            }

            auto desc_pool_index = it->second;

            m_device.getHandle().freeDescriptorSets(m_pools[desc_pool_index], descriptor_set);

            m_set_pool_mapping.erase(it);
            --m_pool_sets_count[desc_pool_index];
            m_pool_index = desc_pool_index;

            return vk::Result::eSuccess;
        }

        uint32_t DescriptorPoolCPP::findAvailablePool(uint32_t search_index) {

            if (m_pools.size() <= search_index) {
                vk::DescriptorPoolCreateInfo create_info;
                create_info.poolSizeCount = static_cast<uint32_t>(m_pool_sizes.size());
                create_info.pPoolSizes = m_pool_sizes.data();
                create_info.maxSets = m_pool_max_sets;

                vk::DescriptorPoolCreateFlags flags;
                auto& binding_flags = m_descriptor_set_layout->getBindingFlags();

                for (auto binding_flag : binding_flags) {
                    if (binding_flag & static_cast<vk::DescriptorBindingFlagBits>(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT)) {
                        flags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
                        break;
                    }
                }

                create_info.flags = flags;

                try {
                    auto handle = m_device.getHandle().createDescriptorPool(create_info);

                    m_pools.push_back(handle);
                    m_pool_sets_count.push_back(0);

                    return search_index;
                }
                catch (const std::exception&) {
                    return 0;
                }
            }
            else if (m_pool_sets_count[search_index] < m_pool_max_sets) {
                return search_index;
            }

            return findAvailablePool(++search_index);
        }
    }
}
