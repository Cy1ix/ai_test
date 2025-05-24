/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "core/instance.h"
#include <map>
#include <vulkan/vulkan.hpp>

#include "utils/logger.h"

namespace frame {
	namespace core {
		class Instance;

		struct DriverVersion {
			uint16_t major{ 0 };
			uint16_t minor{ 0 };
			uint16_t patch{ 0 };
		};

		class PhysicalDevice {
		public:
			PhysicalDevice(Instance& instance, vk::PhysicalDevice physical_device);

			PhysicalDevice(const PhysicalDevice&) = delete;
			PhysicalDevice(PhysicalDevice&&) = delete;
			PhysicalDevice& operator=(const PhysicalDevice&) = delete;
			PhysicalDevice& operator=(PhysicalDevice&&) = delete;

			DriverVersion getDriverVersion() const;

			void* getExtensionFeatureChain() const;

			bool isExtensionSupported(const std::string& requested_extension) const;

			const vk::PhysicalDeviceFeatures& getFeatures() const;
			vk::PhysicalDevice getHandle() const;
			Instance& getInstance() const;
			const vk::FormatProperties getFormatProperties(vk::Format format) const;
			const vk::PhysicalDeviceMemoryProperties& getMemoryProperties() const;
			uint32_t getMemoryType(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32* memory_type_found = nullptr) const;
			const vk::PhysicalDeviceProperties& getProperties() const;
			const std::vector<vk::QueueFamilyProperties>& getQueueFamilyProperties() const;
			uint32_t getQueueFamilyPerformanceQueryPasses(vk::QueryPoolPerformanceCreateInfoKHR& perf_query_create_info) const;

			void enumerateQueueFamilyPerformanceQueryCounters(
				uint32_t queue_family_index,
				uint32_t& count,
				vk::PerformanceCounterKHR* counters,
				vk::PerformanceCounterDescriptionKHR* descriptions) const;

			const vk::PhysicalDeviceFeatures getRequestedFeatures() const;
			vk::PhysicalDeviceFeatures& getMutableRequestedFeatures();

			template <typename StructureType>
			StructureType getExtensionFeatures() {
				if (!m_instance.isEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
					throw std::runtime_error("[PhysicalDevice] ERROR: Unable to enable device extension: " +
						std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME));
				}

				return m_handle.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, StructureType>().template get<StructureType>();
			}
			
			template <typename StructureType>
			StructureType& addExtensionFeatures() {
				if (!m_instance.isEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
					throw std::runtime_error("[PhysicalDevice] ERROR: Unable to enable device extension: " 
						+ std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) + " not enabled");
				}

				auto [it, added] = m_extension_features.insert({ StructureType::structureType, std::make_shared<StructureType>() });
				if (added) {
					if (m_last_requested_extension_feature) {
						static_cast<StructureType*>(it->second.get())->pNext = m_last_requested_extension_feature;
					}
					m_last_requested_extension_feature = it->second.get();
				}

				return *static_cast<StructureType*>(it->second.get());
			}
			
			template <typename Feature>
			vk::Bool32 requestOptionalFeature(vk::Bool32 Feature::* flag, std::string const& featureName, std::string const& flagName) {
				vk::Bool32 supported = getExtensionFeatures<Feature>().*flag;
				if (supported) {
					addExtensionFeatures<Feature>().*flag = true;
				}
				else {
					LOGI("[PhysicalDevice] Requested optional extension <{}::{}> is unsupport", featureName, flagName);
				}
				return supported;
			}
			
			template <typename Feature>
			void requestRequiredFeature(vk::Bool32 Feature::* flag, std::string const& featureName, std::string const& flagName) {
				if (getExtensionFeatures<Feature>().*flag) {
					addExtensionFeatures<Feature>().*flag = true;
				}
				else {
					throw std::runtime_error(std::string("[PhysicalDevice] Requested required extension <") + featureName + "::" + flagName + "> is unsupport");
				}
			}
			
			void setHighPriorityGraphicsQueueEnable(bool enable) { m_high_priority_graphics_queue = enable; }
			
			bool hasHighPriorityGraphicsQueue() const { return m_high_priority_graphics_queue; }

		private:
			Instance& m_instance;
			vk::PhysicalDevice m_handle{ nullptr };
			vk::PhysicalDeviceFeatures m_features;
			std::vector<vk::ExtensionProperties> m_device_extensions;
			vk::PhysicalDeviceProperties m_properties;
			vk::PhysicalDeviceMemoryProperties m_memory_properties;
			std::vector<vk::QueueFamilyProperties> m_queue_family_properties;
			vk::PhysicalDeviceFeatures m_requested_features;
			void* m_last_requested_extension_feature{ nullptr };
			std::map<vk::StructureType, std::shared_ptr<void>> m_extension_features;
			bool m_high_priority_graphics_queue{ false };
		};

#define _REQUEST_OPTIONAL_FEATURE(physical, Feature, flag) physical.requestOptionalFeature<Feature>(&Feature::flag, #Feature, #flag)
#define _REQUEST_REQUIRED_FEATURE(physical, Feature, flag) physical.requestRequiredFeature<Feature>(&Feature::flag, #Feature, #flag)

	}
}
