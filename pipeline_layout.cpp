/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/pipeline_layout.h"
#include "core/device.h"
#include "core/shader_module.h"

namespace frame {
    namespace core {

        PipelineLayoutCPP::PipelineLayoutCPP(core::Device& device, const std::vector<core::ShaderModuleCPP*> shader_modules) :
            VulkanResource{ VK_NULL_HANDLE, &device },
            m_shader_modules{ shader_modules }
        {
            for (auto shader_module : m_shader_modules) {
                for (const auto& resource : shader_module->getResources()) {
                    std::string key = resource.name;
                    /*
                    if (resource.type == core::ShaderResourceType::Input ||
                        resource.type == core::ShaderResourceType::Output)
                    {
                        key = std::to_string(static_cast<uint32_t>(resource.stages)) + "_" + key;
                    }
                    */
                    auto it = m_shader_resources.find(key);

                    if (it != m_shader_resources.end()) {
                        it->second.stages |= resource.stages;
                    }
                    else {
                        m_shader_resources.emplace(key, resource);
                    }
                }
            }

            for (const auto& [key, resource] : m_shader_resources) {
                m_shader_sets[resource.set].push_back(resource);
            }

            m_descriptor_set_layouts.resize(m_shader_sets.size());

            for (const auto& [set_index, resources] : m_shader_sets) {
                m_descriptor_set_layouts[set_index] = 
                    &device.getResourceCache().requestDescriptorSetLayoutCPP(set_index, m_shader_modules, resources);
            }

            std::vector<vk::DescriptorSetLayout> dsl_handles;
            for (auto* dsl : m_descriptor_set_layouts) {
                dsl_handles.push_back(dsl ? dsl->getHandle() : nullptr);
            }

            std::vector<vk::PushConstantRange> push_constant_ranges;
            for (const auto& pc_resource : getResources(core::ShaderResourceType::PushConstant)) {
                push_constant_ranges.push_back({ pc_resource.stages, pc_resource.offset, pc_resource.size });
            }

            vk::PipelineLayoutCreateInfo create_info({}, dsl_handles, push_constant_ranges);
            setHandle(getDevice().getHandle().createPipelineLayout(create_info));
        }

        PipelineLayoutCPP::PipelineLayoutCPP(PipelineLayoutCPP&& other) :
            VulkanResource{ std::move(other) },
            m_shader_modules{ std::move(other.m_shader_modules) },
            m_shader_resources{ std::move(other.m_shader_resources) },
            m_shader_sets{ std::move(other.m_shader_sets) },
            m_descriptor_set_layouts{ std::move(other.m_descriptor_set_layouts) }
        {
            other.setHandle(VK_NULL_HANDLE);
        }

        PipelineLayoutCPP::~PipelineLayoutCPP() {
            if (hasHandle()) {
                getDevice().getHandle().destroyPipelineLayout(getHandle());
            }
        }

        DescriptorSetLayoutCPP& PipelineLayoutCPP::getDescriptorSetLayout(const uint32_t set_index) const {
            auto it = std::find_if(m_descriptor_set_layouts.begin(),
                m_descriptor_set_layouts.end(),
                [set_index](auto const* dsl) { return dsl->getIndex() == set_index; });

            if (it == m_descriptor_set_layouts.end()) {
                throw std::runtime_error("[PipelineLayoutCPP] ERROR: No descriptor set layout is found at set index " + std::to_string(set_index));
            }

            return **it;
        }
        
        vk::ShaderStageFlags PipelineLayoutCPP::getPushConstantRangeStage(uint32_t size, uint32_t offset) const {
            vk::ShaderStageFlags stages;

            for (const auto& pc_resource : getResources(core::ShaderResourceType::PushConstant)) {
                uint32_t pc_end = pc_resource.offset + pc_resource.size;
                uint32_t range_end = offset + size;

                if (pc_resource.offset <= offset && range_end <= pc_end) {
                    stages |= pc_resource.stages;
                }
            }

            return stages;
        }

        std::vector<core::ShaderResource> PipelineLayoutCPP::getResources(
            const core::ShaderResourceType& type, vk::ShaderStageFlagBits stage) const
        {
            std::vector<core::ShaderResource> found_resources;

            for (const auto& [key, resource] : m_shader_resources) {
                bool type_match = (resource.type == type || type == core::ShaderResourceType::All);
                bool stage_match = (resource.stages == stage || stage == vk::ShaderStageFlagBits::eAll);

                if (type_match && stage_match) {
                    found_resources.push_back(resource);
                }
            }

            return found_resources;
        }

        const std::vector<core::ShaderModuleCPP*>& PipelineLayoutCPP::getShaderModules() const {
            return m_shader_modules;
        }

        const std::unordered_map<uint32_t, std::vector<core::ShaderResource>>& PipelineLayoutCPP::getShaderSets() const {
            return m_shader_sets;
        }

        bool PipelineLayoutCPP::hasDescriptorSetLayout(const uint32_t set_index) const {
            return set_index < m_descriptor_set_layouts.size();
        }
    }
}
