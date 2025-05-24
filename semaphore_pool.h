/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

namespace frame {
    namespace core {
        class Device;

        class SemaphorePool {
        public:
            SemaphorePool(Device& device);

            SemaphorePool(const SemaphorePool&) = delete;
            SemaphorePool(SemaphorePool&&) = delete;
            SemaphorePool& operator=(const SemaphorePool&) = delete;
            SemaphorePool& operator=(SemaphorePool&&) = delete;

            ~SemaphorePool();

            vk::Semaphore requestSemaphore();

            vk::Semaphore requestSemaphoreWithOwnership();

            void releaseOwnedSemaphore(vk::Semaphore semaphore);

            void reset();

            uint32_t getActiveSemaphoreCount() const;
        private:
            Device& m_device;
            std::vector<vk::Semaphore> m_semaphores;
            std::vector<vk::Semaphore> m_released_semaphores;
            uint32_t m_active_semaphore_count{ 0 };
        };
    }
}
