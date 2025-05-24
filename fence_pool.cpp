/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "core/fence_pool.h"
#include "core/device.h"

namespace frame {
    namespace core {
        FencePool::FencePool(Device& device) :
            m_device{ device }
        {}

        FencePool::~FencePool() {
            wait();
            reset();
            
            for (auto& fence : m_fences) {
                m_device.getHandle().destroyFence(fence);
            }

            m_fences.clear();
        }

        vk::Fence FencePool::requestFence() {
            if (m_active_fence_count < m_fences.size()) {
                return m_fences[m_active_fence_count++];
            }
            
            vk::FenceCreateInfo create_info{};

            try {
                vk::Fence fence = m_device.getHandle().createFence(create_info);
                m_fences.push_back(fence);
                m_active_fence_count++;
                return m_fences.back();
            }
            catch (const std::exception& e) {
                LOGE(e.what());
                throw std::runtime_error("[FencePool] ERROR: Failed to create fence.");
            }
        }

        vk::Result FencePool::wait(uint32_t timeout) const {
            if (m_active_fence_count < 1 || m_fences.empty()) {
                return vk::Result::eSuccess;
            }

            try {
                return m_device.getHandle().waitForFences(m_active_fence_count, m_fences.data(), true, timeout);
            }
            catch (const vk::SystemError& e) {
                return static_cast<vk::Result>(e.code().value());
            }
        }

        vk::Result FencePool::reset() {
            if (m_active_fence_count < 1 || m_fences.empty()) {
                return vk::Result::eSuccess;
            }

            try {
                vk::Result result = m_device.getHandle().resetFences(m_active_fence_count, m_fences.data());
                m_active_fence_count = 0;
                return result;
            }
            catch (const vk::SystemError& e) {
                return static_cast<vk::Result>(e.code().value());
            }
        }
    }
}
