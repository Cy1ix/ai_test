/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "common/buffer.h"
#include "core/device.h"
#include "core/physical_device.h"

namespace frame {
	namespace common {
		class BufferAllocation {
		public:
			BufferAllocation() = default;
			BufferAllocation(const BufferAllocation&) = delete;
			BufferAllocation(BufferAllocation&&) = default;
			BufferAllocation& operator=(const BufferAllocation&) = delete;
			BufferAllocation& operator=(BufferAllocation&&) = default;

			BufferAllocation(Buffer& buffer, vk::DeviceSize size, vk::DeviceSize offset);

			bool isEmpty() const;
			Buffer& getBuffer();
			vk::DeviceSize getOffset() const;
			vk::DeviceSize getSize() const;
			void update(const std::vector<uint8_t>& data, uint32_t offset = 0);

			template <typename T>
			void update(const T& value, uint32_t offset = 0);

		private:
			Buffer* m_buffer = nullptr;
			vk::DeviceSize m_offset = 0;
			vk::DeviceSize m_size = 0;
		};
		
		class BufferBlock {
		public:
			BufferBlock() = delete;
			BufferBlock(BufferBlock const& rhs) = delete;
			BufferBlock(BufferBlock&& rhs) = default;
			BufferBlock& operator=(BufferBlock const& rhs) = delete;
			BufferBlock& operator=(BufferBlock&& rhs) = default;

			BufferBlock(core::Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage);
			
			BufferAllocation allocate(vk::DeviceSize size);
			
			bool canAllocate(vk::DeviceSize size) const;

			vk::DeviceSize getSize() const;
			void reset();

		private:
			
			vk::DeviceSize alignedOffset() const;
			vk::DeviceSize determineAlignment(vk::BufferUsageFlags usage, vk::PhysicalDeviceLimits const& limits) const;

		private:
			Buffer m_buffer;
			vk::DeviceSize m_alignment = 0;
			vk::DeviceSize m_offset = 0;
		};

		class BufferPool {
		public:
			BufferPool(core::Device& device, vk::DeviceSize block_size, vk::BufferUsageFlags usage,
				VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);

			BufferBlock& requestBufferBlock(vk::DeviceSize minimum_size, bool minimal = false);
			
			void reset();

		private:
			core::Device& m_device;
			std::vector<std::unique_ptr<BufferBlock>> m_buffer_blocks;
			vk::DeviceSize m_block_size = 0;
			vk::BufferUsageFlags m_usage;
			VmaMemoryUsage m_memory_usage{};
		};
		
		inline BufferAllocation::BufferAllocation(Buffer& buffer, vk::DeviceSize size, vk::DeviceSize offset) :
			m_buffer(&buffer),
			m_offset(offset),
			m_size(size) {}

		inline bool BufferAllocation::isEmpty() const {
			return m_size == 0 || m_buffer == nullptr;
		}

		inline Buffer& BufferAllocation::getBuffer() {
			assert(m_buffer && "[BufferPool] ASSERT: Invalid buffer pointer");
			return *m_buffer;
		}

		inline vk::DeviceSize BufferAllocation::getOffset() const {
			return m_offset;
		}

		inline vk::DeviceSize BufferAllocation::getSize() const {
			return m_size;
		}

		inline void BufferAllocation::update(const std::vector<uint8_t>& data, uint32_t offset) {
			assert(m_buffer && "[BufferPool] ASSERT: Invalid buffer pointer");

			if (offset + data.size() <= m_size) {
				m_buffer->update(data.data(), data.size(), toU32(m_offset) + offset);
			}
			else {
				LOGE("Ignore buffer allocation update");
			}
		}

		template <typename T>
		inline void BufferAllocation::update(const T& value, uint32_t offset) {
			update(toBytes(value), offset);
		}

		inline BufferBlock::BufferBlock(core::Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage) :
			m_buffer{ device, size, usage, memory_usage }
		{
			m_alignment = determineAlignment(usage, device.getPhysicalDevice().getProperties().limits);
		}

		inline BufferAllocation BufferBlock::allocate(vk::DeviceSize size) {
			if (canAllocate(size)) {
				auto aligned = alignedOffset();
				m_offset = aligned + size;
				return BufferAllocation{ m_buffer, size, aligned };
			}
			
			return BufferAllocation{};
		}

		inline bool BufferBlock::canAllocate(vk::DeviceSize size) const {
			assert(size > 0 && "[BufferPool] ASSERT: Allocation size must be greater than zero");
			return (alignedOffset() + size <= m_buffer.getSize());
		}

		inline vk::DeviceSize BufferBlock::getSize() const {
			return m_buffer.getSize();
		}

		inline void BufferBlock::reset() {
			m_offset = 0;
		}

		inline vk::DeviceSize BufferBlock::alignedOffset() const {
			return (m_offset + m_alignment - 1) & ~(m_alignment - 1);
		}

		inline vk::DeviceSize BufferBlock::determineAlignment(vk::BufferUsageFlags usage, vk::PhysicalDeviceLimits const& limits) const {
			if (usage == vk::BufferUsageFlagBits::eUniformBuffer) {
				return limits.minUniformBufferOffsetAlignment;
			}
			else if (usage == vk::BufferUsageFlagBits::eStorageBuffer) {
				return limits.minStorageBufferOffsetAlignment;
			}
			else if (usage == vk::BufferUsageFlagBits::eUniformTexelBuffer) {
				return limits.minTexelBufferOffsetAlignment;
			}
			else if (usage == vk::BufferUsageFlagBits::eIndexBuffer || usage == vk::BufferUsageFlagBits::eVertexBuffer ||
				usage == vk::BufferUsageFlagBits::eIndirectBuffer) {
				return 16;
			}
			else {
				throw std::runtime_error("[BufferPool] ERROR: Usage not recognised");
			}
		}

		inline BufferPool::BufferPool(core::Device& device, vk::DeviceSize block_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage) :
			m_device{ device },
			m_block_size{ block_size },
			m_usage{ usage },
			m_memory_usage{ memory_usage }
		{}

		inline BufferBlock& BufferPool::requestBufferBlock(vk::DeviceSize minimum_size, bool minimal) {

			auto it = minimal ?
				std::find_if(m_buffer_blocks.begin(), m_buffer_blocks.end(),
					[&minimum_size](auto const& buffer_block) {
						return (buffer_block->getSize() == minimum_size) && buffer_block->canAllocate(minimum_size);
					}) :
				std::find_if(m_buffer_blocks.begin(), m_buffer_blocks.end(),
					[&minimum_size](auto const& buffer_block) {
						return buffer_block->canAllocate(minimum_size);
					});

					if (it == m_buffer_blocks.end()) {
						LOGD("Building #{} buffer block ({})", m_buffer_blocks.size(), vk::to_string(m_usage));

						vk::DeviceSize new_block_size = minimal ? minimum_size : std::max(m_block_size, minimum_size);
						
						it = m_buffer_blocks.emplace(m_buffer_blocks.end(),
							std::make_unique<BufferBlock>(m_device, new_block_size, m_usage, m_memory_usage));
					}

					return *it->get();
		}

		inline void BufferPool::reset() {
			for (auto& buffer_block : m_buffer_blocks) {
				buffer_block->reset();
			}
		}
	}
}
