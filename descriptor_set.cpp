/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "core/descriptor_set.h"
#include "core/descriptor_pool.h"
#include "core/descriptor_set_layout.h"
#include "core/physical_device.h"
#include "core/device.h"
#include "utils/logger.h"
#include "common/resource_caching.h"

namespace frame {
    namespace core {
        DescriptorSetCPP::DescriptorSetCPP(Device& device,
            const DescriptorSetLayoutCPP& descriptor_set_layout,
            DescriptorPoolCPP& descriptor_pool,
            const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
            const BindingMap<vk::DescriptorImageInfo>& image_infos) :
            VulkanResource{ descriptor_pool.allocate(), &device },
            m_descriptor_set_layout{ descriptor_set_layout },
            m_descriptor_pool{ descriptor_pool },
            m_buffer_infos{ buffer_infos },
            m_image_infos{ image_infos }
        {
            prepare();
        }

        void DescriptorSetCPP::reset(const BindingMap<vk::DescriptorBufferInfo>& new_buffer_infos,
            const BindingMap<vk::DescriptorImageInfo>& new_image_infos)
        {
            if (!new_buffer_infos.empty() || !new_image_infos.empty()) {
                m_buffer_infos = new_buffer_infos;
                m_image_infos = new_image_infos;
            }
            else {
                LOGW("[DescriptorSetCPP] Calling Reset on Descriptor Set with no new buffer infos and no new image infos.");
            }

            m_write_descriptor_sets.clear();
            m_updated_bindings.clear();

            prepare();
        }

        void DescriptorSetCPP::prepare() {
            if (!m_write_descriptor_sets.empty()) {
                LOGW("[DescriptorSetCPP] Trying to prepare a descriptor set that has already been prepared, skipping.");
                return;
            }

            for (auto& binding_it : m_buffer_infos) {
                auto binding_index = binding_it.first;
                auto& buffer_bindings = binding_it.second;

                if (auto binding_info = m_descriptor_set_layout.getLayoutBinding(binding_index)) {
                    for (auto& element_it : buffer_bindings) {
                        auto& buffer_info = element_it.second;

                        size_t uniform_buffer_range_limit = getDevice().getPhysicalDevice().getProperties().limits.maxUniformBufferRange;
                        size_t storage_buffer_range_limit = getDevice().getPhysicalDevice().getProperties().limits.maxStorageBufferRange;
                        size_t buffer_range_limit = buffer_info.range;

                        if ((binding_info->descriptorType == vk::DescriptorType::eUniformBuffer ||
                            binding_info->descriptorType == vk::DescriptorType::eUniformBufferDynamic) &&
                            buffer_range_limit > uniform_buffer_range_limit)
                        {
                            LOGE("[DescriptorSetCPP] Set {} binding {} cannot be updated: buffer size {} exceeds the uniform buffer range limit {}",
                                m_descriptor_set_layout.getIndex(), binding_index, buffer_info.range, uniform_buffer_range_limit);
                            buffer_range_limit = uniform_buffer_range_limit;
                        }
                        else if ((binding_info->descriptorType == vk::DescriptorType::eStorageBuffer ||
                            binding_info->descriptorType == vk::DescriptorType::eStorageBufferDynamic) &&
                            buffer_range_limit > storage_buffer_range_limit)
                        {
                            LOGE("[DescriptorSetCPP] Set {} binding {} cannot be updated: buffer size {} exceeds the storage buffer range limit {}",
                                m_descriptor_set_layout.getIndex(), binding_index, buffer_info.range, storage_buffer_range_limit);
                            buffer_range_limit = storage_buffer_range_limit;
                        }

                        buffer_info.range = buffer_range_limit;

                        vk::WriteDescriptorSet write_descriptor_set{};
                        write_descriptor_set.dstBinding = binding_index;
                        write_descriptor_set.descriptorType = binding_info->descriptorType;
                        write_descriptor_set.pBufferInfo = &buffer_info;
                        write_descriptor_set.dstSet = getHandle();
                        write_descriptor_set.dstArrayElement = element_it.first;
                        write_descriptor_set.descriptorCount = 1;

                        m_write_descriptor_sets.push_back(write_descriptor_set);
                    }
                }
                else {
                    LOGE("[DescriptorSetCPP] Shader layout set does not use buffer binding at #{}", binding_index);
                }
            }

            for (auto& binding_it : m_image_infos) {
                auto binding_index = binding_it.first;
                auto& binding_resources = binding_it.second;

                if (auto binding_info = m_descriptor_set_layout.getLayoutBinding(binding_index)) {
                    for (auto& element_it : binding_resources) {
                        auto& image_info = element_it.second;

                        vk::WriteDescriptorSet write_descriptor_set{};
                        write_descriptor_set.dstBinding = binding_index;
                        write_descriptor_set.descriptorType = binding_info->descriptorType;
                        write_descriptor_set.pImageInfo = &image_info;
                        write_descriptor_set.dstSet = getHandle();
                        write_descriptor_set.dstArrayElement = element_it.first;
                        write_descriptor_set.descriptorCount = 1;

                        m_write_descriptor_sets.push_back(write_descriptor_set);
                    }
                }
                else {
                    LOGE("[DescriptorSetCPP] Shader layout set does not use image binding at #{}", binding_index);
                }
            }
        }

        void DescriptorSetCPP::update(const std::vector<uint32_t>& bindings_to_update) {
            std::vector<vk::WriteDescriptorSet> write_operations;
            std::vector<size_t> write_operation_hashes;

            if (bindings_to_update.empty()) {
                for (const auto& write_operation : m_write_descriptor_sets) {
                    size_t write_operation_hash = 0;

                    common::hashParam(write_operation_hash, write_operation);
                    
                    auto update_pair_it = m_updated_bindings.find(write_operation.dstBinding);
                    if (update_pair_it == m_updated_bindings.end() || update_pair_it->second != write_operation_hash) {
                        write_operations.push_back(write_operation);
                        write_operation_hashes.push_back(write_operation_hash);
                    }
                }
            }
            else {
                for (const auto& write_operation : m_write_descriptor_sets) {
                    if (std::find(bindings_to_update.begin(), bindings_to_update.end(), write_operation.dstBinding) != bindings_to_update.end()) {
                        size_t write_operation_hash = 0;

                        common::hashParam(write_operation_hash, write_operation);
                        
                        auto update_pair_it = m_updated_bindings.find(write_operation.dstBinding);
                        if (update_pair_it == m_updated_bindings.end() || update_pair_it->second != write_operation_hash) {
                            write_operations.push_back(write_operation);
                            write_operation_hashes.push_back(write_operation_hash);
                        }
                    }
                }
            }

            if (!write_operations.empty()) {
                getDevice().getHandle().updateDescriptorSets(write_operations, {});
            }

            for (size_t i = 0; i < write_operations.size(); i++){
                m_updated_bindings[write_operations[i].dstBinding] = write_operation_hashes[i];
            }
        }

        void DescriptorSetCPP::applyWrites() const {
            getDevice().getHandle().updateDescriptorSets(m_write_descriptor_sets, {});
        }

        DescriptorSetCPP::DescriptorSetCPP(DescriptorSetCPP&& other) :
            VulkanResource{ std::move(other) },
            m_descriptor_set_layout{ other.m_descriptor_set_layout },
            m_descriptor_pool{ other.m_descriptor_pool },
            m_buffer_infos{ std::move(other.m_buffer_infos) },
            m_image_infos{ std::move(other.m_image_infos) },
            m_write_descriptor_sets{ std::move(other.m_write_descriptor_sets) },
            m_updated_bindings{ std::move(other.m_updated_bindings) }
        {
            other.setHandle(VK_NULL_HANDLE);
        }
        
        const DescriptorSetLayoutCPP& DescriptorSetCPP::getLayout() const {
            return m_descriptor_set_layout;
        }

        BindingMap<vk::DescriptorBufferInfo>& DescriptorSetCPP::getBufferInfos() {
            return m_buffer_infos;
        }

        BindingMap<vk::DescriptorBufferInfo>& DescriptorSetCPP::getBufferInfos() const {
            return const_cast<BindingMap<vk::DescriptorBufferInfo>&>(m_buffer_infos);
        }

        BindingMap<vk::DescriptorImageInfo>& DescriptorSetCPP::getImageInfos() {
            return m_image_infos;
        }

        BindingMap<vk::DescriptorImageInfo>& DescriptorSetCPP::getImageInfos() const {
            return const_cast<BindingMap<vk::DescriptorImageInfo>&>(m_image_infos);
        }
    }
}
