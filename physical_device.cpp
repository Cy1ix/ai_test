/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/physical_device.h"

namespace frame {
	namespace core {
		PhysicalDevice::PhysicalDevice(Instance& instance, vk::PhysicalDevice physical_device) :
			m_instance{ instance },
			m_handle{ physical_device }
		{
			m_features = physical_device.getFeatures();
			m_properties = physical_device.getProperties();
			m_memory_properties = physical_device.getMemoryProperties();
			m_queue_family_properties = physical_device.getQueueFamilyProperties();
			m_device_extensions = physical_device.enumerateDeviceExtensionProperties();

			LOGI("[PhysicalDevice] Found GPU: {}", m_properties.deviceName.data());
			
			if (!m_device_extensions.empty()) {
				LOGD("[PhysicalDevice] Supported extensions:");
				for (const auto& extension : m_device_extensions) {
					LOGD("  \t{}", extension.extensionName.data());
				}
			}
		}

		DriverVersion PhysicalDevice::getDriverVersion() const {
			DriverVersion version;
			
			switch (m_properties.vendorID) {
			case 0x10DE:
				version.major = (m_properties.driverVersion >> 22) & 0x3ff;
				version.minor = (m_properties.driverVersion >> 14) & 0x0ff;
				version.patch = (m_properties.driverVersion >> 6) & 0x0ff;
				break;
			case 0x8086:
				version.major = (m_properties.driverVersion >> 14) & 0x3ffff;
				version.minor = m_properties.driverVersion & 0x3ffff;
				break;
			default:
				version.major = VK_VERSION_MAJOR(m_properties.driverVersion);
				version.minor = VK_VERSION_MINOR(m_properties.driverVersion);
				version.patch = VK_VERSION_PATCH(m_properties.driverVersion);
				break;
			}

			return version;
		}

		void* PhysicalDevice::getExtensionFeatureChain() const { return m_last_requested_extension_feature; }

		bool PhysicalDevice::isExtensionSupported(const std::string& requested_extension) const {
			return std::find_if(m_device_extensions.begin(), m_device_extensions.end(),
				[&requested_extension](const auto& ext) {
					return std::strcmp(ext.extensionName, requested_extension.c_str()) == 0;
				}) != m_device_extensions.end();
		}

		const vk::PhysicalDeviceFeatures& PhysicalDevice::getFeatures() const { return m_features; }

		vk::PhysicalDevice PhysicalDevice::getHandle() const { return m_handle; }

		Instance& PhysicalDevice::getInstance() const { return m_instance; }

		const vk::FormatProperties PhysicalDevice::getFormatProperties(vk::Format format) const {
			return m_handle.getFormatProperties(format);
		}

		const vk::PhysicalDeviceMemoryProperties& PhysicalDevice::getMemoryProperties() const { return m_memory_properties; }

		uint32_t PhysicalDevice::getMemoryType(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32* memory_type_found) const {
			for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; i++) {
				if ((bits & 1) == 1) {
					if ((m_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
						if (memory_type_found) {
							*memory_type_found = true;
						}
						return i;
					}
				}
				bits >>= 1;
			}

			if (memory_type_found) {
				*memory_type_found = false;
				return ~0;
			}
			else {
				throw std::runtime_error("[PhysicalDevice] ERROR: Unable to find matching memory type");
			}
		}
		
		uint32_t PhysicalDevice::getQueueFamilyPerformanceQueryPasses(vk::QueryPoolPerformanceCreateInfoKHR& perf_query_create_info) const {
			uint32_t passes_needed = m_handle.getQueueFamilyPerformanceQueryPassesKHR(perf_query_create_info);
			return passes_needed;
		}
		
		void PhysicalDevice::enumerateQueueFamilyPerformanceQueryCounters(
			uint32_t queue_family_index,
			uint32_t& count,
			vk::PerformanceCounterKHR* counters,
			vk::PerformanceCounterDescriptionKHR* descriptions) const
		{
			auto result = m_handle.enumerateQueueFamilyPerformanceQueryCountersKHR(queue_family_index, &count, counters, descriptions);

			if (result != vk::Result::eSuccess) {
				throw std::runtime_error("Failed to enumerate performance counters");
			}
		}

		const vk::PhysicalDeviceProperties& PhysicalDevice::getProperties() const { return m_properties; }

		const std::vector<vk::QueueFamilyProperties>& PhysicalDevice::getQueueFamilyProperties() const { return m_queue_family_properties; }

		const vk::PhysicalDeviceFeatures PhysicalDevice::getRequestedFeatures() const { return m_requested_features; }

		vk::PhysicalDeviceFeatures& PhysicalDevice::getMutableRequestedFeatures() { return m_requested_features; }
	}
}
