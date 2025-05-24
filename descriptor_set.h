/* Copyright (c) 2019-2022, Arm Limited and Contributors
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
#include "core/descriptor_pool.h"
#include "core/descriptor_set_layout.h"

namespace frame {
    namespace core {
        class Device;
        class DescriptorSetLayoutCPP;
        class DescriptorPoolCPP;

        class DescriptorSetCPP : public VulkanResource<vk::DescriptorSet> {
        public:
            DescriptorSetCPP(Device& device,
                const DescriptorSetLayoutCPP& descriptor_set_layout,
                DescriptorPoolCPP& descriptor_pool,
                const BindingMap<vk::DescriptorBufferInfo>& buffer_infos = {},
                const BindingMap<vk::DescriptorImageInfo>& image_infos = {});

            DescriptorSetCPP(const DescriptorSetCPP&) = delete;
            DescriptorSetCPP(DescriptorSetCPP&& other);
            DescriptorSetCPP& operator=(const DescriptorSetCPP&) = delete;
            DescriptorSetCPP& operator=(DescriptorSetCPP&&) = delete;

            ~DescriptorSetCPP() = default;

            void reset(const BindingMap<vk::DescriptorBufferInfo>& new_buffer_infos = {},
                const BindingMap<vk::DescriptorImageInfo>& new_image_infos = {});

            void update(const std::vector<uint32_t>& bindings_to_update = {});

            void applyWrites() const;

            const DescriptorSetLayoutCPP& getLayout() const;

            BindingMap<vk::DescriptorBufferInfo>& getBufferInfos();
            BindingMap<vk::DescriptorBufferInfo>& getBufferInfos() const;
            BindingMap<vk::DescriptorImageInfo>& getImageInfos();
            BindingMap<vk::DescriptorImageInfo>& getImageInfos() const;
            
        protected:
            void prepare();
        private:
            const DescriptorSetLayoutCPP& m_descriptor_set_layout;
            DescriptorPoolCPP& m_descriptor_pool;

            BindingMap<vk::DescriptorBufferInfo> m_buffer_infos;
            BindingMap<vk::DescriptorImageInfo> m_image_infos;
            
            std::vector<vk::WriteDescriptorSet> m_write_descriptor_sets;

            std::unordered_map<uint32_t, size_t> m_updated_bindings;
        };
    }
}
