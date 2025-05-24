/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "common/helper.h"
#include "common/common.h"
#include <vulkan/vulkan.hpp>

#include "vulkan_resource.h"

namespace frame {
	namespace core {
        class Device;
        class ShaderModuleCPP;

        struct ShaderResource;

        class DescriptorSetLayoutCPP : public VulkanResource<vk::DescriptorSetLayout> {
        public:
            DescriptorSetLayoutCPP(Device& device,
                const uint32_t set_index,
                const std::vector<ShaderModuleCPP*>& shader_modules,
                const std::vector<ShaderResource>& resource_set);
            
            DescriptorSetLayoutCPP(const DescriptorSetLayoutCPP&) = delete;
            DescriptorSetLayoutCPP(DescriptorSetLayoutCPP&& other);
            ~DescriptorSetLayoutCPP();

            DescriptorSetLayoutCPP& operator=(const DescriptorSetLayoutCPP&) = delete;
            DescriptorSetLayoutCPP& operator=(DescriptorSetLayoutCPP&&) = delete;
            
            const uint32_t getIndex() const;
            const std::vector<vk::DescriptorSetLayoutBinding>& getBindings() const;
            std::unique_ptr<vk::DescriptorSetLayoutBinding> getLayoutBinding(const uint32_t binding_index) const;
            std::unique_ptr<vk::DescriptorSetLayoutBinding> getLayoutBinding(const std::string& name) const;
            const std::vector<vk::DescriptorBindingFlagsEXT>& getBindingFlags() const;
            vk::DescriptorBindingFlagsEXT getLayoutBindingFlag(const uint32_t binding_index) const;
            const std::vector<ShaderModuleCPP*>& getShaderModules() const;

        private:
            const uint32_t m_set_index;
            std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
            std::vector<vk::DescriptorBindingFlagsEXT> m_binding_flags;
            std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> m_bindings_lookup;
            std::unordered_map<uint32_t, vk::DescriptorBindingFlagsEXT> m_binding_flags_lookup;
            std::unordered_map<std::string, uint32_t> m_resources_lookup;
            std::vector<ShaderModuleCPP*> m_shader_modules;
        };
	}
}
