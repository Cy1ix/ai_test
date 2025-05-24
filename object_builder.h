/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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

#include "common/common.h"
#include <vulkan/vulkan.hpp>

namespace frame {
    namespace alloc {
        
        template <typename BuilderType, typename CreateInfoType>
        class ObjectBuilder {
        public:
            VmaAllocationCreateInfo const& getAllocationCreateInfo() const;
            
            CreateInfoType const& getCreateInfo() const;
            
            std::string const& getDebugName() const;
            
            BuilderType& withDebugName(const std::string& name);
            
            BuilderType& withImplicitSharingMode();
            
            BuilderType& withMemoryTypeBits(uint32_t type_bits);
            
            BuilderType& withQueueFamilies(uint32_t count, const uint32_t* family_indices);
            
            BuilderType& withQueueFamilies(std::vector<uint32_t> const& queue_families);
            
            BuilderType& withSharingMode(vk::SharingMode sharing_mode);
            
            BuilderType& withVmaFlags(VmaAllocationCreateFlags flags);
            
            BuilderType& withVmaPool(VmaPool pool);
            
            BuilderType& withVmaPreferredFlags(vk::MemoryPropertyFlags flags);
            
            BuilderType& withVmaRequiredFlags(vk::MemoryPropertyFlags flags);
            
            BuilderType& withVmaUsage(VmaMemoryUsage usage);

        protected:
            ObjectBuilder(const ObjectBuilder& other) = delete;
            ObjectBuilder(const CreateInfoType& create_info);

            CreateInfoType& getCreateInfo();

        protected:
            VmaAllocationCreateInfo m_alloc_create_info = {};
            CreateInfoType m_create_info = {};
            std::string m_debug_name = {};
        };

        template <typename BuilderType, typename CreateInfoType>
        inline ObjectBuilder<BuilderType, CreateInfoType>::ObjectBuilder(const CreateInfoType& create_info) :
            m_create_info(create_info)
        {
            m_alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
        };

        template <typename BuilderType, typename CreateInfoType>
        inline VmaAllocationCreateInfo const& ObjectBuilder<BuilderType, CreateInfoType>::getAllocationCreateInfo() const
        {
            return m_alloc_create_info;
        }

        template <typename BuilderType, typename CreateInfoType>
        inline CreateInfoType const& ObjectBuilder<BuilderType, CreateInfoType>::getCreateInfo() const
        {
            return m_create_info;
        }

        template <typename BuilderType, typename CreateInfoType>
        inline CreateInfoType& ObjectBuilder<BuilderType, CreateInfoType>::getCreateInfo()
        {
            return m_create_info;
        }

        template <typename BuilderType, typename CreateInfoType>
        inline std::string const& ObjectBuilder<BuilderType, CreateInfoType>::getDebugName() const
        {
            return m_debug_name;
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withDebugName(const std::string& name)
        {
            m_debug_name = name;
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withImplicitSharingMode()
        {
            m_create_info.sharingMode = (1 < m_create_info.queueFamilyIndexCount) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withMemoryTypeBits(uint32_t type_bits)
        {
            m_alloc_create_info.memoryTypeBits = type_bits;
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withQueueFamilies(uint32_t count, const uint32_t* family_indices)
        {
            m_create_info.queueFamilyIndexCount = count;
            m_create_info.pQueueFamilyIndices = family_indices;
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withQueueFamilies(std::vector<uint32_t> const& queue_families)
        {
            return withQueueFamilies(static_cast<uint32_t>(queue_families.size()), queue_families.data());
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withSharingMode(vk::SharingMode sharing_mode)
        {
            m_create_info.sharingMode = sharing_mode;
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withVmaFlags(VmaAllocationCreateFlags flags)
        {
            m_alloc_create_info.flags = flags;
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withVmaPool(VmaPool pool)
        {
            m_alloc_create_info.pool = pool;
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withVmaPreferredFlags(vk::MemoryPropertyFlags flags)
        {
            m_alloc_create_info.preferredFlags = static_cast<VkMemoryPropertyFlags>(flags);
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withVmaRequiredFlags(vk::MemoryPropertyFlags flags)
        {
            m_alloc_create_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(flags);
            return *static_cast<BuilderType*>(this);
        }

        template <typename BuilderType, typename CreateInfoType>
        inline BuilderType& ObjectBuilder<BuilderType, CreateInfoType>::withVmaUsage(VmaMemoryUsage usage)
        {
            m_alloc_create_info.usage = usage;
            return *static_cast<BuilderType*>(this);
        }
    }
}
