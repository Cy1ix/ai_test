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

#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "core/shader_module.h"

namespace frame {
    namespace core {
        class Device;
        class DescriptorSetLayoutCPP;

        class PipelineLayoutCPP : public VulkanResource<vk::PipelineLayout> {
        public:
            PipelineLayoutCPP(Device& device, const std::vector<ShaderModuleCPP*> shader_modules);
            PipelineLayoutCPP(const PipelineLayoutCPP&) = delete;
            PipelineLayoutCPP(PipelineLayoutCPP&& other);
            ~PipelineLayoutCPP();

            PipelineLayoutCPP& operator=(const PipelineLayoutCPP&) = delete;
            PipelineLayoutCPP& operator=(PipelineLayoutCPP&&) = delete;

            DescriptorSetLayoutCPP& getDescriptorSetLayout(const uint32_t set_index) const;
            vk::ShaderStageFlags getPushConstantRangeStage(uint32_t size, uint32_t offset = 0) const;
            std::vector<ShaderResource> getResources(
                const ShaderResourceType& type = ShaderResourceType::All,
                vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eAll) const;
            const std::vector<ShaderModuleCPP*>& getShaderModules() const;
            const std::unordered_map<uint32_t, std::vector<ShaderResource>>& getShaderSets() const;
            bool hasDescriptorSetLayout(const uint32_t set_index) const;

        private:
            std::vector<ShaderModuleCPP*> m_shader_modules;
            std::unordered_map<std::string, ShaderResource> m_shader_resources;
            std::unordered_map<uint32_t, std::vector<ShaderResource>> m_shader_sets;
            std::vector<DescriptorSetLayoutCPP*> m_descriptor_set_layouts;
        };
    }
}
