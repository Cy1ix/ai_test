/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "core/semaphore_pool.h"
#include "core/device.h"

namespace frame {
    namespace core {
        SemaphorePool::SemaphorePool(Device& device) :
            m_device{ device }
        {}

        SemaphorePool::~SemaphorePool() {
            reset();

            for (auto semaphore : m_semaphores) {
                m_device.getHandle().destroySemaphore(semaphore);
            }

            m_semaphores.clear();
        }

        vk::Semaphore SemaphorePool::requestSemaphoreWithOwnership() {
            if (m_active_semaphore_count < m_semaphores.size()) {
                vk::Semaphore semaphore = m_semaphores.back();
                m_semaphores.pop_back();
                return semaphore;
            }

            vk::SemaphoreCreateInfo create_info{};

            try {
                return m_device.getHandle().createSemaphore(create_info);
            }
            catch (vk::SystemError& e) {
                LOGE(e.what());
                throw std::runtime_error("[SemaphorePool] ERROR: Failed to create semaphore.");
            }
        }

        void SemaphorePool::releaseOwnedSemaphore(vk::Semaphore semaphore) {
            m_released_semaphores.push_back(semaphore);
        }

        vk::Semaphore SemaphorePool::requestSemaphore() {
            if (m_active_semaphore_count < m_semaphores.size()) {
                return m_semaphores[m_active_semaphore_count++];
            }

            vk::SemaphoreCreateInfo create_info{};

            try {
                vk::Semaphore semaphore = m_device.getHandle().createSemaphore(create_info);
                m_semaphores.push_back(semaphore);
                m_active_semaphore_count++;
                return semaphore;
            }
            catch (vk::SystemError& e) {
                LOGE(e.what());
                throw std::runtime_error("[SemaphorePool] ERROR: Failed to create semaphore.");
            }
        }

        void SemaphorePool::reset() {
            m_active_semaphore_count = 0;

            for (auto& sem : m_released_semaphores) {
                m_semaphores.push_back(sem);
            }

            m_released_semaphores.clear();
        }

        uint32_t SemaphorePool::getActiveSemaphoreCount() const {
            return m_active_semaphore_count;
        }
    }
}
