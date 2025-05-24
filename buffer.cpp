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

#include "common/buffer.h"
#include "core/device.h"

namespace frame {
	namespace common {

        BufferBuilder::BufferBuilder(vk::DeviceSize size) :
            ParentType(vk::BufferCreateInfo{ {}, size })
        {}

        Buffer BufferBuilder::build(core::Device& device) const {
            return Buffer{ device, *this };
        }

        std::unique_ptr<Buffer> BufferBuilder::buildUnique(core::Device& device) const {
            return std::make_unique<Buffer>(device, *this);
        }

        BufferBuilder& BufferBuilder::withFlags(vk::BufferCreateFlags flags) {
            this->m_create_info.flags = flags;
            return *this;
        }

        BufferBuilder& BufferBuilder::withUsage(vk::BufferUsageFlags usage) {
            this->getCreateInfo().usage = usage;
            return *this;
        }

        Buffer Buffer::createStagingBuffer(core::Device& device, vk::DeviceSize size, const void* data) {
            BufferBuilder builder(size);
            builder.withVmaFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .withUsage(vk::BufferUsageFlagBits::eTransferSrc);
            Buffer result(device, builder);
            if (data != nullptr) {
                result.update(data, size);
            }
            return result;
        }

        Buffer::Buffer(core::Device& device,
            vk::DeviceSize size,
            vk::BufferUsageFlags buffer_usage,
            VmaMemoryUsage memory_usage,
            VmaAllocationCreateFlags flags,
            const std::vector<uint32_t>& queue_family_indices) :
            Buffer(device,
                BufferBuilder(size)
                .withUsage(buffer_usage)
                .withVmaUsage(memory_usage)
                .withVmaFlags(flags)
                .withQueueFamilies(queue_family_indices)
                .withImplicitSharingMode())
        {}

        uint64_t Buffer::getDeviceAddress() const {
            return this->getDevice().getHandle().getBufferAddressKHR({ this->getHandle() });
        }

        vk::DeviceSize Buffer::getSize() const {
            return m_size;
        }

        Buffer::Buffer(core::Device& device, const BufferBuilder& builder) :
            ParentType(builder.getAllocationCreateInfo(), nullptr, &device), m_size(builder.getCreateInfo().size) {
            this->setHandle(this->createBuffer(builder.getCreateInfo()));
            if (!builder.getDebugName().empty()) {
                this->setDebugName(builder.getDebugName());
            }
        }

        Buffer::~Buffer() {
            this->destroyBuffer(this->getHandle());
        }
	}
}
