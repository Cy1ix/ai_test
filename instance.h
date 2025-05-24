/* Copyright (c) 2022-2025, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024-2025, Arm Limited and Contributors
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

#include <optional>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace frame {
    namespace core {
        class PhysicalDevice;
        
        class Instance {
        public:
            static std::optional<uint32_t> m_selected_gpu_index;
            
            Instance(const std::string& application_name,
                const std::unordered_map<const char*, bool>& requested_extensions = {},
                const std::unordered_map<const char*, bool>& requested_layers = {},
                const std::vector<vk::LayerSettingEXT>& required_layer_settings = {},
                uint32_t api_version = VK_API_VERSION_1_3);
            
            Instance(vk::Instance instance);
            
            Instance(const Instance&) = delete;
            Instance(Instance&&) = delete;

            ~Instance();

            const std::vector<const char*>& getExtensions();
            PhysicalDevice& getFirstPhysicalDevice();
            [[nodiscard]] vk::Instance getHandle() const;
            PhysicalDevice& getSuitablePhysicalDevice(vk::SurfaceKHR surface);
            bool isEnabled(const char* extension) const;

        private:
            void queryPhysicalDevices();

            vk::Instance m_handle;
            std::vector<const char*> m_enabled_instance_extensions;
            std::vector<const char*> m_enabled_layers;

#if defined(VK_DEBUG) || defined(VK_VALIDATION_LAYERS)
            vk::DebugUtilsMessengerEXT m_debug_utils_messenger;
            vk::DebugReportCallbackEXT m_debug_report_callback;
#endif

            std::vector<std::unique_ptr<PhysicalDevice>> m_physical_devices;
        };
    }
}
