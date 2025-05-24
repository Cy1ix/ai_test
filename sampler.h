/* Copyright (c) 2019-2024, Arm Limited and Contributors
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
#include "core/vulkan_resource.h"
#include "core/device.h"

namespace frame {
	namespace core {
		class Device;

		class Sampler : public VulkanResource<vk::Sampler> {
		public:
			Sampler(Device& device, const vk::SamplerCreateInfo& info) :
				VulkanResource{ device.getHandle().createSampler(info), &device }
			{}

			Sampler(const Sampler&) = delete;

			Sampler(Sampler&& other_sampler) : VulkanResource(std::move(other_sampler)) {}

			~Sampler() {
				if (getHandle()) {
					getDevice().getHandle().destroySampler(getHandle());
				}
			}

			Sampler& operator=(const Sampler&) = delete;

			Sampler& operator=(Sampler&&) = delete;
		};
	}
}