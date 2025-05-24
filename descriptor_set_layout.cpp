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

#include "core/descriptor_set_layout.h"
#include "core/device.h"
#include "core/physical_device.h"
#include "core/shader_module.h"

namespace frame {
	namespace core {
        namespace {
            inline vk::DescriptorType findDescriptorType(ShaderResourceType resource_type, bool dynamic) {
                switch (resource_type) {
                case ShaderResourceType::InputAttachment:
                    return vk::DescriptorType::eInputAttachment;
                case ShaderResourceType::ImageCPP:
                    return vk::DescriptorType::eSampledImage;
                case ShaderResourceType::ImageCPPSampler:
                    return vk::DescriptorType::eCombinedImageSampler;
                case ShaderResourceType::ImageCPPStorage:
                    return vk::DescriptorType::eStorageImage;
                case ShaderResourceType::Sampler:
                    return vk::DescriptorType::eSampler;
                case ShaderResourceType::BufferUniform:
                    return dynamic ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
                case ShaderResourceType::BufferStorage:
                    return dynamic ? vk::DescriptorType::eStorageBufferDynamic : vk::DescriptorType::eStorageBuffer;
                default:
                    throw std::runtime_error("[DescriptorSetLayoutCPP] ERROR: No conversion possible for the shader resource type.");
                }
            }

            inline bool validateBinding(const vk::DescriptorSetLayoutBinding& binding, const std::vector<vk::DescriptorType>& blacklist) {
                return std::find_if(blacklist.begin(), blacklist.end(),
                    [&binding](const vk::DescriptorType& type) { return type == binding.descriptorType; }) == blacklist.end();
            }

            inline bool validateFlags(const PhysicalDevice& physical_device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                const std::vector<vk::DescriptorBindingFlagsEXT>& flags) {
                if (flags.empty()) {
                    return true;
                }
                
                if (bindings.size() != flags.size()) {
                    LOGE("Binding count has to be equal to flag count.");
                    return false;
                }

                return true;
            }
        }

        DescriptorSetLayoutCPP::DescriptorSetLayoutCPP(Device& device,
            const uint32_t set_index,
            const std::vector<ShaderModuleCPP*>& shader_modules,
            const std::vector<ShaderResource>& resource_set) :
            VulkanResource{ VK_NULL_HANDLE, &device },
            m_set_index{ set_index },
            m_shader_modules{ shader_modules }
        {
            for (auto& resource : resource_set) {
                if (resource.type == ShaderResourceType::Input ||
                    resource.type == ShaderResourceType::Output ||
                    resource.type == ShaderResourceType::PushConstant ||
                    resource.type == ShaderResourceType::SpecializationConstant)
                {
                    continue;
                }
                
                auto descriptor_type = findDescriptorType(resource.type, resource.mode == ShaderResourceMode::Dynamic);

                if (resource.mode == ShaderResourceMode::UpdateAfterBind) {
                    m_binding_flags.push_back(vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind);
                }
                else {
                    m_binding_flags.push_back(vk::DescriptorBindingFlagsEXT());
                }
                
                vk::DescriptorSetLayoutBinding layout_binding{};
                layout_binding.binding = resource.binding;
                layout_binding.descriptorCount = resource.array_size;
                layout_binding.descriptorType = descriptor_type;
                layout_binding.stageFlags = static_cast<vk::ShaderStageFlags>(resource.stages);

                m_bindings.push_back(layout_binding);
                m_bindings_lookup.emplace(resource.binding, layout_binding);
                m_binding_flags_lookup.emplace(resource.binding, m_binding_flags.back());
                m_resources_lookup.emplace(resource.name, resource.binding);
            }

            vk::DescriptorSetLayoutCreateInfo create_info;
            create_info.flags = vk::DescriptorSetLayoutCreateFlags();
            create_info.bindingCount = static_cast<uint32_t>(m_bindings.size());
            create_info.pBindings = m_bindings.data();
            
            vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags_create_info;
            if (std::find_if(resource_set.begin(), resource_set.end(),
                [](const ShaderResource& shader_resource) {
	                return shader_resource.mode == ShaderResourceMode::UpdateAfterBind;
                }) != resource_set.end()) {
                if (std::find_if(resource_set.begin(), resource_set.end(),
                    [](const ShaderResource& shader_resource) {
	                    return shader_resource.mode == ShaderResourceMode::Dynamic;
                    }) != resource_set.end()) {
                    throw std::runtime_error("[DescriptorSetLayoutCPP] ERROR: Cannot create descriptor set layout, dynamic resources are not allowed if at least one resource is update-after-bind.");
                }

                if (!validateFlags(device.getPhysicalDevice(), m_bindings, m_binding_flags)) {
                    throw std::runtime_error("[DescriptorSetLayoutCPP] ERROR: Invalid binding, couldn't create descriptor set layout.");
                }

                binding_flags_create_info.bindingCount = static_cast<uint32_t>(m_binding_flags.size());
                binding_flags_create_info.pBindingFlags = m_binding_flags.data();

                create_info.pNext = &binding_flags_create_info;

                bool has_update_after_bind = std::find(m_binding_flags.begin(), m_binding_flags.end(),
                    vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind) != m_binding_flags.end();

                if (has_update_after_bind) {
                    create_info.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
                }
            }
            
            try {
                setHandle(getDevice().getHandle().createDescriptorSetLayout(create_info));
            }
            catch (vk::SystemError& e) {
                throw std::runtime_error(std::string("[DescriptorSetLayoutCPP] ERROR: Cannot create DescriptorSetLayoutCPP: ") + e.what());
            }
        }

        DescriptorSetLayoutCPP::DescriptorSetLayoutCPP(DescriptorSetLayoutCPP&& other) :
            VulkanResource{ std::move(other) },
            m_set_index{ other.m_set_index },
            m_bindings{ std::move(other.m_bindings) },
            m_binding_flags{ std::move(other.m_binding_flags) },
            m_bindings_lookup{ std::move(other.m_bindings_lookup) },
            m_binding_flags_lookup{ std::move(other.m_binding_flags_lookup) },
            m_resources_lookup{ std::move(other.m_resources_lookup) },
            m_shader_modules{ other.m_shader_modules } 
        {
            other.setHandle(VK_NULL_HANDLE);
        }

        DescriptorSetLayoutCPP::~DescriptorSetLayoutCPP() {
            if (hasHandle()) {
                getDevice().getHandle().destroyDescriptorSetLayout(getHandle());
            }
        }
        
        const uint32_t DescriptorSetLayoutCPP::getIndex() const {
            return m_set_index;
        }

        const std::vector<vk::DescriptorSetLayoutBinding>& DescriptorSetLayoutCPP::getBindings() const {
            return m_bindings;
        }

        const std::vector<vk::DescriptorBindingFlagsEXT>& DescriptorSetLayoutCPP::getBindingFlags() const {
            return m_binding_flags;
        }

        std::unique_ptr<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutCPP::getLayoutBinding(uint32_t binding_index) const {
            auto it = m_bindings_lookup.find(binding_index);

            if (it == m_bindings_lookup.end()) {
                return nullptr;
            }

            return std::make_unique<vk::DescriptorSetLayoutBinding>(it->second);
        }

        std::unique_ptr<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutCPP::getLayoutBinding(const std::string& name) const {
            auto it = m_resources_lookup.find(name);

            if (it == m_resources_lookup.end()) {
                return nullptr;
            }

            return getLayoutBinding(it->second);
        }

        vk::DescriptorBindingFlagsEXT DescriptorSetLayoutCPP::getLayoutBindingFlag(const uint32_t binding_index) const {
            auto it = m_binding_flags_lookup.find(binding_index);

            if (it == m_binding_flags_lookup.end()) {
                return vk::DescriptorBindingFlagsEXT();
            }

            return it->second;
        }

        const std::vector<ShaderModuleCPP*>& DescriptorSetLayoutCPP::getShaderModules() const {
            return m_shader_modules;
        }
	}
}