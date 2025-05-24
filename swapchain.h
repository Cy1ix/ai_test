/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include <set>
#include <vulkan/vulkan.hpp>
#include "core/vulkan_resource.h"

namespace frame {
    namespace core {
        class Device;

        struct SwapchainProperties {
            vk::SwapchainKHR old_swapchain;
            uint32_t image_count{ 3 };
            vk::Extent2D extent;
            vk::SurfaceFormatKHR surface_format;
            uint32_t array_layers;
            vk::ImageUsageFlags image_usage;
            vk::SurfaceTransformFlagBitsKHR pre_transform;
            vk::CompositeAlphaFlagBitsKHR composite_alpha;
            vk::PresentModeKHR present_mode;
        };
        
        class Swapchain : public VulkanResource<vk::SwapchainKHR> {
        public:
            Swapchain(Swapchain& old_swapchain, const vk::Extent2D& extent);

            Swapchain(Swapchain& old_swapchain, const uint32_t image_count);

            Swapchain(Swapchain& old_swapchain, const std::set<vk::ImageUsageFlagBits>& image_usage_flags);

            Swapchain(Swapchain& swapchain, const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform);

            Swapchain(Device& device,
                vk::SurfaceKHR surface,
                const vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo,
                const std::vector<vk::PresentModeKHR>& present_mode_priority_list = { vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifo },
                const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list = { {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
                                                                                         {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear} },
                const vk::Extent2D& extent = {},
                const uint32_t image_count = 3,
                const vk::SurfaceTransformFlagBitsKHR transform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
                const std::set<vk::ImageUsageFlagBits>& image_usage_flags = { vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc },
                vk::SwapchainKHR old_swapchain = nullptr);

            Swapchain(const Swapchain&) = delete;
            Swapchain(Swapchain&& other);
            Swapchain& operator=(const Swapchain&) = delete;
            Swapchain& operator=(Swapchain&&) = delete;

            ~Swapchain();

            bool isValid() const;
            std::pair<vk::Result, uint32_t> acquireNextImage(vk::Semaphore image_acquired_semaphore, vk::Fence fence = nullptr) const;
            const vk::Extent2D& getExtent() const;
            vk::Format getFormat() const;
            const std::vector<vk::Image>& getImages() const;
            vk::SurfaceTransformFlagBitsKHR getTransform() const;
            vk::SurfaceKHR getSurface() const;
            vk::ImageUsageFlags getUsage() const;
            vk::PresentModeKHR getPresentMode() const;

        private:
            vk::SurfaceKHR m_surface;
            std::vector<vk::Image> m_images;
            SwapchainProperties m_properties;
            std::vector<vk::PresentModeKHR> m_present_mode_priority_list;
            std::vector<vk::SurfaceFormatKHR> m_surface_format_priority_list;
            std::set<vk::ImageUsageFlagBits> m_image_usage_flags;
        };
    }
}
