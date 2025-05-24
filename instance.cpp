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

#include <Volk/volk.h>
#include "core/instance.h"
#include "core/physical_device.h"
#include "utils/logger.h"

#if defined(VK_DEBUG) || defined(VK_VALIDATION_LAYERS)
#   define USE_VALIDATION_LAYERS
#endif

#if defined(USE_VALIDATION_LAYERS) && (defined(VK_VALIDATION_LAYERS_GPU_ASSISTED) || defined(VK_VALIDATION_LAYERS_BEST_PRACTICES) || defined(VK_VALIDATION_LAYERS_SYNCHRONIZATION))
#   define USE_VALIDATION_LAYER_FEATURES
#endif

namespace frame {
    namespace core {
        namespace {
#ifdef USE_VALIDATION_LAYERS
            VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                void* user_data)
            {
                if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
                    LOGI("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
                }
                else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                    LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
                }
                else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                    LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
                }
                return VK_FALSE;
            }

            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugReportFlagsEXT flags,
                VkDebugReportObjectTypeEXT /*type*/,
                uint64_t /*object*/,
                size_t /*location*/,
                int32_t /*message_code*/,
                const char* layer_prefix,
                const char* message,
                void* /*user_data*/)
            {
                if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
                    LOGE("{}: {}", layer_prefix, message);
                }
                else if (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)) {
                    LOGW("{}: {}", layer_prefix, message);
                }
                else {
                    LOGI("{}: {}", layer_prefix, message);
                }
                return VK_FALSE;
            }
#endif

            bool validateLayers(const std::vector<const char*>& required, const std::vector<vk::LayerProperties>& available) {
                return std::all_of(required.begin(), required.end(), [&available](const char* layer) {
                    return std::any_of(available.begin(), available.end(),
                        [layer](const auto& available_layer) {
                            return strcmp(available_layer.layerName, layer) == 0;
                        });
                    });
            }

            bool enableExtension(const char* requested_extension,
                const std::vector<vk::ExtensionProperties>& available_extensions,
                std::vector<const char*>& enabled_extensions)
            {
                bool is_available = std::any_of(available_extensions.begin(), available_extensions.end(),
                    [requested_extension](const auto& ext) {
                        return strcmp(requested_extension, ext.extensionName) == 0;
                    });

                if (is_available) {
                    bool is_already_enabled = std::any_of(enabled_extensions.begin(), enabled_extensions.end(),
                        [requested_extension](const auto& ext) {
                            return strcmp(requested_extension, ext) == 0;
                        });

                    if (!is_already_enabled) {
                        LOGI("Extension {} available, enabled", requested_extension);
                        enabled_extensions.emplace_back(requested_extension);
                    }
                    return true;
                }

                LOGI("Extension {} not available", requested_extension);
                return false;
            }

            bool enableLayer(const char* requested_layer,
                const std::vector<vk::LayerProperties>& available_layers,
                std::vector<const char*>& enabled_layers)
            {
                bool is_available = std::any_of(available_layers.begin(), available_layers.end(),
                    [requested_layer](const auto& layer) {
                        return strcmp(requested_layer, layer.layerName) == 0;
                    });

                if (is_available) {
                    bool is_already_enabled = std::any_of(enabled_layers.begin(), enabled_layers.end(),
                        [requested_layer](const auto& layer) {
                            return strcmp(requested_layer, layer) == 0;
                        });

                    if (!is_already_enabled) {
                        LOGI("Layer {} available, enabld", requested_layer);
                        enabled_layers.emplace_back(requested_layer);
                    }
                    return true;
                }

                LOGI("Layer {} not available", requested_layer);
                return false;
            }

        }

        std::optional<uint32_t> Instance::m_selected_gpu_index;

        Instance::Instance(
            const std::string& application_name,
            const std::unordered_map<const char*, bool>& requested_extensions,
            const std::unordered_map<const char*, bool>& requested_layers,
            const std::vector<vk::LayerSettingEXT>& required_layer_settings,
            uint32_t api_version)
        {
            vk::DynamicLoader dl;
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

            auto available_extensions = vk::enumerateInstanceExtensionProperties();

#ifdef USE_VALIDATION_LAYERS
            const bool has_debug_utils = enableExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, available_extensions, m_enabled_instance_extensions);

            bool has_debug_report = false;
            if (!has_debug_utils) {
                has_debug_report = enableExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, available_extensions, m_enabled_instance_extensions);

                if (!has_debug_report) {
                    LOGW("Debug extensions not available, debug report disabled");
                }
            }
#endif

#if (defined(VK_ENABLE_PORTABILITY))
            enableExtension(VK_MVK_MACOS_SURFACE_EXTENSION_NAME, available_extensions, m_enabled_instance_extensions);
            bool portability_enumeration_available = enableExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, available_extensions, m_enabled_instance_extensions);
#endif

#ifdef USE_VALIDATION_LAYER_FEATURES
            bool validation_features = false;
            {
                auto validation_extensions = vk::enumerateInstanceExtensionProperties(std::string("VK_LAYER_KHRONOS_validation"));
                validation_features = enableExtension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME, validation_extensions, m_enabled_instance_extensions);
            }
#endif

            enableExtension(VK_KHR_SURFACE_EXTENSION_NAME, available_extensions, m_enabled_instance_extensions);
            enableExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, available_extensions, m_enabled_instance_extensions);

            for (const auto& extension : requested_extensions) {
                auto const& extension_name = extension.first;
                auto extension_is_optional = extension.second;
                if (!enableExtension(extension_name, available_extensions, m_enabled_instance_extensions)) {
                    if (extension_is_optional) {
                        LOGW("Optional instance extension {} not available, some features may be disabled", extension_name);
                    }
                    else {
                        LOGE("Required instance extension {} not available, cannot run", extension_name);
                        throw std::runtime_error("Required instance extensions are missing.");
                    }
                }
            }

            auto available_layers = vk::enumerateInstanceLayerProperties();

            for (const auto& layer : requested_layers) {
                auto const& layer_name = layer.first;
                auto layer_is_optional = layer.second;
                if (!enableLayer(layer_name, available_layers, m_enabled_layers)) {
                    if (layer_is_optional) {
                        LOGW("Optional layer {} not available, some features may be disabled", layer_name);
                    }
                    else {
                        LOGE("Required layer {} not available, cannot run", layer_name);
                        throw std::runtime_error("Required layers are missing.");
                    }
                }
            }

#ifdef USE_VALIDATION_LAYERS
            enableLayer("VK_LAYER_KHRONOS_validation", available_layers, m_enabled_layers);
#endif

            vk::ApplicationInfo app_info(application_name.c_str(), 0, "", 0, api_version);
            vk::InstanceCreateInfo instance_info({}, &app_info, m_enabled_layers, m_enabled_instance_extensions);

#ifdef USE_VALIDATION_LAYERS
            vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info;
            vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;

            if (has_debug_utils) {
                debug_utils_create_info = vk::DebugUtilsMessengerCreateInfoEXT(
                    {},
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                    debugUtilsMessengerCallback
                );
                instance_info.pNext = &debug_utils_create_info;
            }
            else if (has_debug_report) {
                debug_report_create_info = vk::DebugReportCallbackCreateInfoEXT(
                    vk::DebugReportFlagBitsEXT::eError |
                    vk::DebugReportFlagBitsEXT::eWarning |
                    vk::DebugReportFlagBitsEXT::ePerformanceWarning,
                    debugCallback
                );
                instance_info.pNext = &debug_report_create_info;
            }
#endif

#if (defined(VK_ENABLE_PORTABILITY))
            if (portability_enumeration_available) {
                instance_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
            }
#endif

#ifdef USE_VALIDATION_LAYER_FEATURES
            vk::ValidationFeaturesEXT validation_features_info;
            std::vector<vk::ValidationFeatureEnableEXT> enable_features;

            if (validation_features) {
#   if defined(VK_VALIDATION_LAYERS_GPU_ASSISTED)
                enable_features.push_back(vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot);
                enable_features.push_back(vk::ValidationFeatureEnableEXT::eGpuAssisted);
#   endif
#   if defined(VK_VALIDATION_LAYERS_BEST_PRACTICES)
                enable_features.push_back(vk::ValidationFeatureEnableEXT::eBestPractices);
#   endif
                validation_features_info.setEnabledValidationFeatures(enable_features);
                validation_features_info.pNext = instance_info.pNext;
                instance_info.pNext = &validation_features_info;
            }
#endif

            vk::LayerSettingsCreateInfoEXT layer_settings_info;
            if (!required_layer_settings.empty()) {
                layer_settings_info.settingCount = static_cast<uint32_t>(required_layer_settings.size());
                layer_settings_info.pSettings = required_layer_settings.data();
                layer_settings_info.pNext = instance_info.pNext;
                instance_info.pNext = &layer_settings_info;
            }

            m_handle = vk::createInstance(instance_info);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);
            volkLoadInstance(m_handle);

#ifdef USE_VALIDATION_LAYERS
            if (has_debug_utils) {
                m_debug_utils_messenger = m_handle.createDebugUtilsMessengerEXT(debug_utils_create_info);
            }
            else if (has_debug_report) {
                m_debug_report_callback = m_handle.createDebugReportCallbackEXT(debug_report_create_info);
            }
#endif
            queryPhysicalDevices();
        }

        Instance::Instance(vk::Instance instance) : m_handle{ instance }
        {
            if (!m_handle) {
                throw std::runtime_error("[Instance] ERROR: Invalid instance handle");
            }
            queryPhysicalDevices();
        }

        Instance::~Instance() {

            if (!m_handle) return;

#ifdef USE_VALIDATION_LAYERS
            if (m_debug_utils_messenger) {
                m_handle.destroyDebugUtilsMessengerEXT(m_debug_utils_messenger);
            }
            if (m_debug_report_callback) {
                m_handle.destroyDebugReportCallbackEXT(m_debug_report_callback);
            }
#endif
            m_handle.destroy();
        }

        const std::vector<const char*>& Instance::getExtensions() { return m_enabled_instance_extensions; }

        PhysicalDevice& Instance::getFirstPhysicalDevice() {

            assert(!m_physical_devices.empty() && "[Instance] ASSERT: No physical devices found");

            auto discrete_gpu = std::find_if(m_physical_devices.begin(), m_physical_devices.end(),
                [](const auto& gpu) {
                    return gpu->getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
                });

            if (discrete_gpu != m_physical_devices.end()) {
                return **discrete_gpu;
            }

            LOGW("No discrete GPU found, using default GPU");
            return *m_physical_devices[0];
        }

        vk::Instance Instance::getHandle() const { return m_handle; }

        PhysicalDevice& Instance::getSuitablePhysicalDevice(vk::SurfaceKHR surface) {

            assert(!m_physical_devices.empty() && "[Instance] ASSERT: No physical devices found");

            if (m_selected_gpu_index.has_value()) {
                std::optional<uint32_t> idx = m_selected_gpu_index;
                LOGI("Using explicitly selected GPU {}", idx.value());

                if (idx >= m_physical_devices.size()) {
                    throw std::runtime_error("[Instance] ERROR: Selected GPU index out of range");
                }
                return *m_physical_devices[idx.value()];
            }

            for (auto& gpu : m_physical_devices) {
                if (gpu->getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                    const auto& queue_families = gpu->getQueueFamilyProperties();

                    for (uint32_t i = 0; i < queue_families.size(); i++) {
                        if (gpu->getHandle().getSurfaceSupportKHR(i, surface)) {
                            return *gpu;
                        }
                    }
                }
            }

            LOGW("No suitable discrete GPU found, using default GPU");
            return *m_physical_devices[0];
        }

        bool Instance::isEnabled(const char* extension) const
        {
            return std::find_if(m_enabled_instance_extensions.begin(), m_enabled_instance_extensions.end(),
                [extension](const char* enabled) {
                    return strcmp(extension, enabled) == 0;
                }) != m_enabled_instance_extensions.end();
        }

        void Instance::queryPhysicalDevices()
        {
            auto physical_devices = m_handle.enumeratePhysicalDevices();

            if (physical_devices.empty()) {
                throw std::runtime_error("No Vulkan-capable physical devices found");
            }

            for (auto& device : physical_devices) {
                m_physical_devices.push_back(std::make_unique<PhysicalDevice>(*this, device));
            }
        }
    }
}
