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

#include "core/swapchain.h"
#include "core/device.h"
#include "core/physical_device.h"
#include "utils/logger.h"

namespace frame {
    namespace {

        vk::Extent2D chooseExtent(vk::Extent2D request_extent,
            const vk::Extent2D& min_image_extent,
            const vk::Extent2D& max_image_extent,
            const vk::Extent2D& current_extent)
        {
            if (current_extent.width == 0xFFFFFFFF)
                return request_extent;

            if (request_extent.width < 1 || request_extent.height < 1) {
                LOGW("ImageCPP extent ({}, {}) not supported. Using ({}, {}).",
                    request_extent.width, request_extent.height,
                    current_extent.width, current_extent.height);
                return current_extent;
            }

            request_extent.width = std::clamp(request_extent.width, min_image_extent.width, max_image_extent.width);
            request_extent.height = std::clamp(request_extent.height, min_image_extent.height, max_image_extent.height);

            return request_extent;
        }

        vk::PresentModeKHR choosePresentMode(vk::PresentModeKHR request_present_mode,
            const std::vector<vk::PresentModeKHR>& available_present_modes,
            const std::vector<vk::PresentModeKHR>& present_mode_priority_list)
        {
            auto present_mode_it = std::find(available_present_modes.begin(), available_present_modes.end(), request_present_mode);
            if (present_mode_it != available_present_modes.end()) {
                LOGI("Present mode selected: {}", vk::to_string(request_present_mode));
                return request_present_mode;
            }

            auto chosen_present_mode_it = std::find_if(
                present_mode_priority_list.begin(), present_mode_priority_list.end(),
                [&available_present_modes](vk::PresentModeKHR mode) {
                    return std::find(available_present_modes.begin(), available_present_modes.end(), mode) != available_present_modes.end();
                });

            vk::PresentModeKHR chosen_present_mode = (chosen_present_mode_it != present_mode_priority_list.end())
                ? *chosen_present_mode_it
                : vk::PresentModeKHR::eFifo;

            LOGW("Present mode '{}' not supported. Using '{}'.",
                vk::to_string(request_present_mode), vk::to_string(chosen_present_mode));
            return chosen_present_mode;
        }

        vk::SurfaceFormatKHR chooseSurfaceFormat(const vk::SurfaceFormatKHR& requested_format,
            const std::vector<vk::SurfaceFormatKHR>& available_formats,
            const std::vector<vk::SurfaceFormatKHR>& priority_list)
        {
            auto format_it = std::find(available_formats.begin(), available_formats.end(), requested_format);
            if (format_it != available_formats.end()) {
                LOGI("Surface format selected: {}",
                    vk::to_string(requested_format.format) + ", " + vk::to_string(requested_format.colorSpace));
                return requested_format;
            }

            auto chosen_format_it = std::find_if(
                priority_list.begin(), priority_list.end(),
                [&available_formats](vk::SurfaceFormatKHR format) {
                    return std::find(available_formats.begin(), available_formats.end(), format) != available_formats.end();
                });

            vk::SurfaceFormatKHR chosen_format = (chosen_format_it != priority_list.end())
                ? *chosen_format_it
                : available_formats[0];

            LOGW("Surface format ({}) not supported. Using ({}).",
                vk::to_string(requested_format.format) + ", " + vk::to_string(requested_format.colorSpace),
                vk::to_string(chosen_format.format) + ", " + vk::to_string(chosen_format.colorSpace));
            return chosen_format;
        }

        vk::SurfaceTransformFlagBitsKHR chooseTransform(vk::SurfaceTransformFlagBitsKHR request_transform,
            vk::SurfaceTransformFlagsKHR supported_transform,
            vk::SurfaceTransformFlagBitsKHR current_transform)
        {
            if (request_transform & supported_transform) {
                return request_transform;
            }

            LOGW("Surface transform '{}' not supported. Using '{}'.",
                vk::to_string(request_transform), vk::to_string(current_transform));
            return current_transform;
        }

        vk::CompositeAlphaFlagBitsKHR chooseCompositeAlpha(vk::CompositeAlphaFlagBitsKHR request_alpha,
            vk::CompositeAlphaFlagsKHR supported_alpha)
        {
            if (request_alpha & supported_alpha) {
                return request_alpha;
            }

            static const std::vector<vk::CompositeAlphaFlagBitsKHR> alpha_priority_list = {
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
                vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
                vk::CompositeAlphaFlagBitsKHR::eInherit
            };

            auto chosen_alpha_it = std::find_if(
                alpha_priority_list.begin(), alpha_priority_list.end(),
                [&supported_alpha](vk::CompositeAlphaFlagBitsKHR alpha) {
                    return alpha & supported_alpha;
                });

            if (chosen_alpha_it == alpha_priority_list.end()) {
                throw std::runtime_error("No compatible composite alpha found.");
            }

            LOGW("Composite alpha '{}' not supported. Using '{}'.",
                vk::to_string(request_alpha), vk::to_string(*chosen_alpha_it));
            return *chosen_alpha_it;
        }

        bool validateFormatFeature(vk::ImageUsageFlagBits image_usage, vk::FormatFeatureFlags supported_features) {
            return (image_usage != vk::ImageUsageFlagBits::eStorage) ||
                (supported_features & vk::FormatFeatureFlagBits::eStorageImage);
        }

        std::set<vk::ImageUsageFlagBits> chooseImageCPPUsage(
            const std::set<vk::ImageUsageFlagBits>& requested_flags,
            vk::ImageUsageFlags supported_usage,
            vk::FormatFeatureFlags supported_features)
        {
            std::set<vk::ImageUsageFlagBits> validated_flags;

            for (auto flag : requested_flags) {
                if ((flag & supported_usage) && validateFormatFeature(flag, supported_features)) {
                    validated_flags.insert(flag);
                }
                else {
                    LOGW("ImageCPP usage ({}) requested but not supported.", vk::to_string(flag));
                }
            }

            if (validated_flags.empty()) {
                static const std::vector<vk::ImageUsageFlagBits> usage_priority_list = {
                    vk::ImageUsageFlagBits::eColorAttachment,
                    vk::ImageUsageFlagBits::eStorage,
                    vk::ImageUsageFlagBits::eSampled,
                    vk::ImageUsageFlagBits::eTransferDst
                };

                auto priority_it = std::find_if(
                    usage_priority_list.begin(), usage_priority_list.end(),
                    [&supported_usage, &supported_features](auto image_usage) {
                        return (image_usage & supported_usage) &&
                            validateFormatFeature(image_usage, supported_features);
                    });

                if (priority_it != usage_priority_list.end()) {
                    validated_flags.insert(*priority_it);
                }
            }

            if (validated_flags.empty()) {
                throw std::runtime_error("[Swapchain] ERROR: No compatible image usage found.");
            }

            std::string usage_list;
            for (vk::ImageUsageFlagBits usage : validated_flags)
                usage_list += to_string(usage) + " ";

            LOGI("ImageCPP usage flags: {}", usage_list);
            return validated_flags;
        }

        vk::ImageUsageFlags compositeImageCPPFlags(std::set<vk::ImageUsageFlagBits>& image_usage_flags) {
            vk::ImageUsageFlags image_usage;
            for (auto flag : image_usage_flags) {
                image_usage |= flag;
            }
            return image_usage;
        }
    }

    namespace core {
        Swapchain::Swapchain(Swapchain& old_swapchain, const vk::Extent2D& extent) :
            Swapchain{
                old_swapchain.getDevice(),
                old_swapchain.m_surface,
                old_swapchain.m_properties.present_mode,
                old_swapchain.m_present_mode_priority_list,
                old_swapchain.m_surface_format_priority_list,
                extent,
                old_swapchain.m_properties.image_count,
                old_swapchain.m_properties.pre_transform,
                old_swapchain.m_image_usage_flags,
            	old_swapchain.getHandle()
        } {
        }

        Swapchain::Swapchain(Swapchain& old_swapchain, const uint32_t image_count) :
            Swapchain{
                old_swapchain.getDevice(),
                old_swapchain.m_surface,
                old_swapchain.m_properties.present_mode,
                old_swapchain.m_present_mode_priority_list,
                old_swapchain.m_surface_format_priority_list,
                old_swapchain.m_properties.extent,
                image_count,
                old_swapchain.m_properties.pre_transform,
                old_swapchain.m_image_usage_flags,
                old_swapchain.getHandle()
        } {
        }

        Swapchain::Swapchain(Swapchain& old_swapchain, const std::set<vk::ImageUsageFlagBits>& image_usage_flags) :
            Swapchain{
                old_swapchain.getDevice(),
                old_swapchain.m_surface,
                old_swapchain.m_properties.present_mode,
                old_swapchain.m_present_mode_priority_list,
                old_swapchain.m_surface_format_priority_list,
                old_swapchain.m_properties.extent,
                old_swapchain.m_properties.image_count,
                old_swapchain.m_properties.pre_transform,
                image_usage_flags,
                old_swapchain.getHandle()
        } {
        }

        Swapchain::Swapchain(Swapchain& old_swapchain, const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform) :
            Swapchain{
                old_swapchain.getDevice(),
                old_swapchain.m_surface,
                old_swapchain.m_properties.present_mode,
                old_swapchain.m_present_mode_priority_list,
                old_swapchain.m_surface_format_priority_list,
                extent,
                old_swapchain.m_properties.image_count,
                transform,
                old_swapchain.m_image_usage_flags,
                old_swapchain.getHandle()
        } {
        }

        Swapchain::Swapchain(
            Device& device,
            vk::SurfaceKHR surface,
            const vk::PresentModeKHR present_mode,
            const std::vector<vk::PresentModeKHR>& present_mode_priority_list,
            const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list,
            const vk::Extent2D& extent,
            const uint32_t image_count,
            const vk::SurfaceTransformFlagBitsKHR transform,
            const std::set<vk::ImageUsageFlagBits>& image_usage_flags,
            vk::SwapchainKHR old_swapchain) :
            VulkanResource{ VK_NULL_HANDLE, &device },
            m_surface{ surface }
        {
            m_present_mode_priority_list = present_mode_priority_list;
            m_surface_format_priority_list = surface_format_priority_list;

            std::vector<vk::SurfaceFormatKHR> surface_formats = device.getPhysicalDevice().getHandle().getSurfaceFormatsKHR(surface);
            LOGI("Surface supports the following surface formats:");
            for (auto& format : surface_formats) {
                LOGI("  \t{}", vk::to_string(format.format) + ", " + vk::to_string(format.colorSpace));
            }

            std::vector<vk::PresentModeKHR> present_modes = device.getPhysicalDevice().getHandle().getSurfacePresentModesKHR(surface);
            LOGI("Surface supports the following present modes:");
            for (auto& mode : present_modes) {
                LOGI("  \t{}", vk::to_string(mode));
            }

            vk::SurfaceCapabilitiesKHR surface_capabilities = device.getPhysicalDevice().getHandle().getSurfaceCapabilitiesKHR(surface);

            m_properties.old_swapchain = old_swapchain;
            m_properties.image_count = std::clamp(image_count,
                surface_capabilities.minImageCount,
                surface_capabilities.maxImageCount ?
                surface_capabilities.maxImageCount :
                std::numeric_limits<uint32_t>::max());

            m_properties.extent = chooseExtent(extent,
                surface_capabilities.minImageExtent,
                surface_capabilities.maxImageExtent,
                surface_capabilities.currentExtent);

            m_properties.surface_format = chooseSurfaceFormat(
                { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
                surface_formats,
                surface_format_priority_list);

            m_properties.array_layers = 1;

            vk::FormatProperties format_properties = device.getPhysicalDevice().getHandle().getFormatProperties(m_properties.surface_format.format);
            m_image_usage_flags = chooseImageCPPUsage(image_usage_flags,
                surface_capabilities.supportedUsageFlags,
                format_properties.optimalTilingFeatures);

            m_properties.image_usage = compositeImageCPPFlags(m_image_usage_flags);
            m_properties.pre_transform = chooseTransform(transform,
                surface_capabilities.supportedTransforms,
                surface_capabilities.currentTransform);

            m_properties.composite_alpha = chooseCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eInherit,
                surface_capabilities.supportedCompositeAlpha);

            m_properties.present_mode = choosePresentMode(present_mode,
                present_modes,
                present_mode_priority_list);

            vk::SwapchainCreateInfoKHR create_info({},
                surface,
                m_properties.image_count,
                m_properties.surface_format.format,
                m_properties.surface_format.colorSpace,
                m_properties.extent,
                m_properties.array_layers,
                m_properties.image_usage,
                {},
                {},
                m_properties.pre_transform,
                m_properties.composite_alpha,
                m_properties.present_mode,
                {},
                m_properties.old_swapchain);
            
            setHandle(device.getHandle().createSwapchainKHR(create_info));

            m_images = device.getHandle().getSwapchainImagesKHR(getHandle());
        }

        Swapchain::~Swapchain() {
            if (hasHandle()) {
                getDevice().getHandle().destroySwapchainKHR(getHandle());
            }
        }

        Swapchain::Swapchain(Swapchain&& other) :
            VulkanResource{ std::move(other) },
            m_surface{ std::exchange(other.m_surface, nullptr) },
            m_images{ std::exchange(other.m_images, {}) },
            m_properties{ std::exchange(other.m_properties, {}) },
            m_present_mode_priority_list{ std::exchange(other.m_present_mode_priority_list, {}) },
            m_surface_format_priority_list{ std::exchange(other.m_surface_format_priority_list, {}) },
            m_image_usage_flags{ std::move(other.m_image_usage_flags) } {
        }

        bool Swapchain::isValid() const { return hasHandle(); }
        
        std::pair<vk::Result, uint32_t> Swapchain::acquireNextImage(vk::Semaphore image_acquired_semaphore, vk::Fence fence) const {
            vk::ResultValue<uint32_t> rv = getDevice().getHandle().acquireNextImageKHR(
                getHandle(), std::numeric_limits<uint64_t>::max(), image_acquired_semaphore, fence);
            return std::make_pair(rv.result, rv.value);
        }

        const vk::Extent2D& Swapchain::getExtent() const { return m_properties.extent; }

        vk::Format Swapchain::getFormat() const { return m_properties.surface_format.format; }

        const std::vector<vk::Image>& Swapchain::getImages() const { return m_images; }

        vk::SurfaceTransformFlagBitsKHR Swapchain::getTransform() const { return m_properties.pre_transform; }

        vk::SurfaceKHR Swapchain::getSurface() const { return m_surface; }

        vk::ImageUsageFlags Swapchain::getUsage() const { return m_properties.image_usage; }

        vk::PresentModeKHR Swapchain::getPresentMode() const { return m_properties.present_mode; }
    }
}
