/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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
#include "common/allocator.h"
#include "common/object_builder.h"

namespace frame {
	class core::Device;
	
    namespace common {
        class Buffer;
        class BufferBuilder : public alloc::ObjectBuilder<BufferBuilder, vk::BufferCreateInfo> {
        private:
            using ParentType = alloc::ObjectBuilder<BufferBuilder, vk::BufferCreateInfo>;
        public:
            BufferBuilder(vk::DeviceSize size);

            Buffer build(core::Device& device) const;
            std::unique_ptr<Buffer> buildUnique(core::Device& device) const;
            BufferBuilder& withFlags(vk::BufferCreateFlags flags);
            BufferBuilder& withUsage(vk::BufferUsageFlags usage);
        };

        class Buffer : public alloc::VmaAllocated<vk::Buffer> {
        private:
            using ParentType = alloc::VmaAllocated<vk::Buffer>;
        public:
            static Buffer createStagingBuffer(core::Device& device, vk::DeviceSize size, const void* data);

            template <typename T>
            static Buffer createStagingBuffer(core::Device& device, std::vector<T> const& data) {
                return createStagingBuffer(device, data.size() * sizeof(T), data.data());
            }

            template <typename T>
            static Buffer createStagingBuffer(core::Device& device, const T& data) {
                return createStagingBuffer(device, sizeof(T), &data);
            }

            Buffer() = delete;
            Buffer(const Buffer&) = delete;
            Buffer(Buffer&& other) = default;
            Buffer& operator=(const Buffer&) = delete;
            Buffer& operator=(Buffer&&) = default;

            Buffer(core::Device& device,
                vk::DeviceSize size,
                vk::BufferUsageFlags buffer_usage,
                VmaMemoryUsage memory_usage,
                VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                const std::vector<uint32_t>& queue_family_indices = {});

            Buffer(core::Device& device, BufferBuilder const& builder);

            ~Buffer();

            uint64_t getDeviceAddress() const;

            vk::DeviceSize getSize() const;

        private:
            vk::DeviceSize m_size = 0;
        };
        
    }
}
