/* Copyright (c) 2021-2024, Arm Limited and Contributors
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

#include "common/debug.h"
#include <utility>
#include <vulkan/vulkan.hpp>

namespace frame {
    namespace core {
        class Device;
        class DebugUtilsExtDebugUtils;

        template <typename Handle>
        class VulkanResource {
        public:
            VulkanResource(Handle handle = nullptr, Device* device = nullptr);

            VulkanResource(const VulkanResource&) = delete;
            VulkanResource& operator=(const VulkanResource&) = delete;

            VulkanResource(VulkanResource&& other);
            VulkanResource& operator=(VulkanResource&& other);

            virtual ~VulkanResource() = default;

            Device& getDevice();
            Device const& getDevice() const;
            uint64_t getHandleU64() const;
            Handle& getHandle();
            const Handle& getHandle() const;
            vk::ObjectType getObjectType() const;
            const std::string& getDebugName() const;
            bool hasDevice() const;
            bool hasHandle() const;
            void setDebugName(const std::string& name);
            void setHandle(Handle hdl);

        private:
            std::string m_debug_name;
            Device* m_device = nullptr;
            Handle m_handle;
        };

        template <typename Handle>
        VulkanResource<Handle>::VulkanResource(Handle handle, Device* device) :
            m_device{ device },
            m_handle{ handle } {}

        template <typename Handle>
        VulkanResource<Handle>::VulkanResource(VulkanResource&& other) :
            m_device(std::exchange(other.m_device, {})),
    		m_handle(std::exchange(other.m_handle, {})),
            m_debug_name(std::exchange(other.m_debug_name, {})) {}

        template <typename Handle>
        VulkanResource<Handle>& VulkanResource<Handle>::operator=(VulkanResource&& other) {
            m_handle = std::exchange(other.m_handle, {});
            m_device = std::exchange(other.m_device, {});
            m_debug_name = std::exchange(other.m_debug_name, {});
            return *this;
        }

        template <typename Handle>
        Device& VulkanResource<Handle>::getDevice() {
            assert(m_device && "[VulkanResource] ASSERT: Device handle not set");
            return *m_device;
        }

        template <typename Handle>
        Device const& VulkanResource<Handle>::getDevice() const {
            assert(m_device && "[VulkanResource] ASSERT: Device handle not set");
            return *m_device;
        }

        template <typename Handle>
        uint64_t VulkanResource<Handle>::getHandleU64() const {
            using UintHandle = typename std::conditional<sizeof(Handle) == sizeof(uint32_t), uint32_t, uint64_t>::type;
            return static_cast<uint64_t>(*reinterpret_cast<UintHandle const*>(&m_handle));
        }

        template <typename Handle>
        const std::string& VulkanResource<Handle>::getDebugName() const { return m_debug_name; }

        template <typename Handle>
        Handle& VulkanResource<Handle>::getHandle() { return m_handle; }

        template <typename Handle>
        const Handle& VulkanResource<Handle>::getHandle() const { return m_handle; }

        template <typename Handle>
        vk::ObjectType VulkanResource<Handle>::getObjectType() const { return Handle::objectType; }

        template <typename Handle>
        bool VulkanResource<Handle>::hasDevice() const { return m_device != nullptr; }

        template <typename Handle>
        bool VulkanResource<Handle>::hasHandle() const { return m_handle != nullptr; }

        template <typename Handle>
        void VulkanResource<Handle>::setHandle(Handle hdl) { m_handle = hdl; }

        template <typename Handle>
        void VulkanResource<Handle>::setDebugName(const std::string& name) {
            m_debug_name = name;

            if (m_device && !m_debug_name.empty()) {
                getDevice().getDebugUtils().setDebugName(getDevice().getHandle(), getObjectType(), getHandleU64(), m_debug_name.c_str());
            }
        }
    }
}
