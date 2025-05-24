/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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

#include "core/vulkan_resource.h"
#include "utils/logger.h"

namespace frame {
    namespace alloc {
        inline VmaAllocator& getMemoryAllocator() {
            static VmaAllocator memory_allocator = VK_NULL_HANDLE;
            return memory_allocator;
        }

        inline void shutdown() {
            auto& allocator = getMemoryAllocator();
            if (allocator != VK_NULL_HANDLE)
            {
                VmaTotalStatistics stats;
                vmaCalculateStatistics(allocator, &stats);
                LOGI("Total device memory leaked: {} bytes.", stats.total.statistics.allocationBytes);
                vmaDestroyAllocator(allocator);
                allocator = VK_NULL_HANDLE;
            }
        }

        template <typename HandleType>
        class VmaAllocated : public core::VulkanResource<HandleType> {
        public:
            VmaAllocated() = delete;
            VmaAllocated(const VmaAllocated&) = delete;
            VmaAllocated(VmaAllocated&& other);
            VmaAllocated& operator=(VmaAllocated const& other) = delete;
            VmaAllocated& operator=(VmaAllocated&& other) = default;

        protected:
            template <typename... Args>
            VmaAllocated(const VmaAllocationCreateInfo& allocation_create_info, Args&&... args);
            VmaAllocated(HandleType handle, core::Device* device_ = nullptr);

        public:
            const HandleType* get() const;

            void flush(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);

            const uint8_t* getData() const;

            vk::DeviceMemory getMemory() const;

            uint8_t* map();

            bool mapped() const;

            void unmap();

            size_t update(const uint8_t* data, size_t size, size_t offset = 0);

            size_t update(void const* data, size_t size, size_t offset = 0);

            template <typename T>
            size_t update(std::vector<T> const& data, size_t offset = 0) {
                return update(data.data(), data.size() * sizeof(T), offset);
            }
            
            template <typename T, size_t N>
            size_t update(std::array<T, N> const& data, size_t offset = 0) {
                return update(data.data(), data.size() * sizeof(T), offset);
            }
            
            template <class T>
            size_t convertAndUpdate(const T& object, size_t offset = 0) {
                return update(reinterpret_cast<const uint8_t*>(&object), sizeof(T), offset);
            }

            template <class T>
            size_t updateTyped(const vk::ArrayProxy<T>& object, size_t offset = 0) {
                return update(reinterpret_cast<const uint8_t*>(object.data()), object.size() * sizeof(T), offset);
            }

        protected:

            vk::Buffer createBuffer(const vk::BufferCreateInfo& create_info);

            vk::Image createImage(const vk::ImageCreateInfo& create_info);

            virtual void postCreate(VmaAllocationInfo const& allocation_info);

            void destroyBuffer(vk::Buffer buffer);

            void destroyImage(vk::Image image);

            void clear();

        private:
            VmaAllocationCreateInfo allocation_create_info = {};
            VmaAllocation allocation = VK_NULL_HANDLE;
            uint8_t* mapped_data = nullptr;
            bool coherent = false;
            bool persistent = false;
        };

        template <typename HandleType>
        inline VmaAllocated<HandleType>::VmaAllocated(VmaAllocated&& other) :
            core::VulkanResource<HandleType>{ static_cast<core::VulkanResource<HandleType>&&>(other) },
            allocation_create_info(std::exchange(other.allocation_create_info, {})),
            allocation(std::exchange(other.allocation, {})),
            mapped_data(std::exchange(other.mapped_data, {})),
            coherent(std::exchange(other.coherent, {})),
            persistent(std::exchange(other.persistent, {}))
        {}

        template <typename HandleType>
        template <typename... Args>
        inline VmaAllocated<HandleType>::VmaAllocated(const VmaAllocationCreateInfo& allocation_create_info, Args&&... args) :
            core::VulkanResource<HandleType>{ std::forward<Args>(args)... },
            allocation_create_info(allocation_create_info)
        {}

        template <typename HandleType>
        inline VmaAllocated<HandleType>::VmaAllocated(HandleType handle, core::Device* device_) :
            core::VulkanResource<HandleType>(handle, device_)
        {}

        template <typename HandleType>
        inline const HandleType* VmaAllocated<HandleType>::get() const {
            return &core::VulkanResource<HandleType>::getHandle();
        }

        template <typename HandleType>
        inline void VmaAllocated<HandleType>::clear() {
            mapped_data = nullptr;
            persistent = false;
            allocation_create_info = {};
        }

        template <typename HandleType>
        inline vk::Buffer VmaAllocated<HandleType>::createBuffer(const vk::BufferCreateInfo& create_info) {

            vk::Buffer buffer = VK_NULL_HANDLE;
            VmaAllocationInfo allocation_info{};

            if (vmaCreateBuffer(
                getMemoryAllocator(),
                reinterpret_cast<VkBufferCreateInfo const*>(&create_info),
                &allocation_create_info,
                reinterpret_cast<VkBuffer*>(&buffer),
                &allocation,
                &allocation_info) != VK_SUCCESS)
            {
                throw std::runtime_error("[Allocator] ERROR: Create buffer fail");
            }

            postCreate(allocation_info);
            return buffer;
        }

        template <typename HandleType>
        inline vk::Image VmaAllocated<HandleType>::createImage(const vk::ImageCreateInfo& create_info) {

            assert(0 < create_info.mipLevels && "[Allocator] ASSERT: ImageCPPs should have at least one level");
            assert(0 < create_info.arrayLayers && "[Allocator] ASSERT: ImageCPPs should have at least one layer");
            assert(create_info.usage && "[Allocator] ASSERT: ImageCPPs should have at least one usage type");

            vk::Image image = VK_NULL_HANDLE;
            VmaAllocationInfo allocation_info{};

            if (vmaCreateImage(getMemoryAllocator(),
                reinterpret_cast<VkImageCreateInfo const*>(&create_info),
                &allocation_create_info,
                reinterpret_cast<VkImage*>(&image),
                &allocation,
                &allocation_info) != VK_SUCCESS)
            {
                throw std::runtime_error("[Allocator] ERROR: Create image fail");
            }

            postCreate(allocation_info);
            return image;
        }

        template <typename HandleType>
        inline void VmaAllocated<HandleType>::destroyBuffer(vk::Buffer buffer) {
            if (buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
                unmap();
                vmaDestroyBuffer(getMemoryAllocator(), static_cast<VkBuffer>(buffer), allocation);
                clear();
            }
        }

        template <typename HandleType>
        inline void VmaAllocated<HandleType>::destroyImage(vk::Image image) {
            if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
                unmap();
                vmaDestroyImage(getMemoryAllocator(), image, allocation);
                clear();
            }
        }

        template <typename HandleType>
        inline void VmaAllocated<HandleType>::flush(vk::DeviceSize offset, vk::DeviceSize size) {
            if (!coherent) {
                vmaFlushAllocation(getMemoryAllocator(), allocation, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size));
            }
        }

        template <typename HandleType>
        inline const uint8_t* VmaAllocated<HandleType>::getData() const { return mapped_data; }

        template <typename HandleType>
        inline vk::DeviceMemory VmaAllocated<HandleType>::getMemory() const {
            VmaAllocationInfo alloc_info;
            vmaGetAllocationInfo(getMemoryAllocator(), allocation, &alloc_info);
            return alloc_info.deviceMemory;
        }

        template <typename HandleType>
        inline uint8_t* VmaAllocated<HandleType>::map() {
            if (!persistent && !mapped()) {
                if (vmaMapMemory(getMemoryAllocator(), allocation, reinterpret_cast<void**>(&mapped_data)) != VK_SUCCESS) {
                    throw std::runtime_error("[Allocator] ERROR: Map memory fail");
                }
                assert(mapped_data);
            }
            return mapped_data;
        }

        template <typename HandleType>
        inline bool VmaAllocated<HandleType>::mapped() const { return mapped_data != nullptr; }

        template <typename HandleType>
        inline void VmaAllocated<HandleType>::postCreate(VmaAllocationInfo const& allocation_info) {
            VkMemoryPropertyFlags memory_properties;
            vmaGetAllocationMemoryProperties(getMemoryAllocator(), allocation, &memory_properties);
            coherent = (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            mapped_data = static_cast<uint8_t*>(allocation_info.pMappedData);
            persistent = mapped();
        }

        template <typename HandleType>
        inline void VmaAllocated<HandleType>::unmap() {
            if (!persistent && mapped()) {
                vmaUnmapMemory(getMemoryAllocator(), allocation);
                mapped_data = nullptr;
            }
        }

        template <typename HandleType>
        inline size_t VmaAllocated<HandleType>::update(const uint8_t* data, size_t size, size_t offset) {
            if (persistent) {
                std::copy(data, data + size, mapped_data + offset);
                flush();
            }
            else {
                map();
                std::copy(data, data + size, mapped_data + offset);
                flush();
                unmap();
            }
            return size;
        }

        template <typename HandleType>
        inline size_t VmaAllocated<HandleType>::update(void const* data, size_t size, size_t offset) {
            return update(reinterpret_cast<const uint8_t*>(data), size, offset);
        }
    }
}
