/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Arm Limited and Contributors
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

#include "core/device.h"
#include "core/swapchain.h"
#include "platform/window.h"
#include "rendering/render_frame.h"

namespace frame {
    namespace core {
        class Device;
    }
    namespace rendering {

        enum RenderMethod {
	        FORWARD,
            DEFERRED,
            COMBINE
        };

        class RenderContext {
        public:
            static vk::Format DEFAULT_VK_FORMAT;

            RenderContext(core::Device& device,
                vk::SurfaceKHR surface,
                const platform::Window& window,
                vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo,
                std::vector<vk::PresentModeKHR> const& present_mode_priority_list = { vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox },
                std::vector<vk::SurfaceFormatKHR> const& surface_format_priority_list = {
                    {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
                    {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear} }
            );

            RenderContext(const RenderContext&) = delete;
            RenderContext(RenderContext&&) = delete;
            virtual ~RenderContext() = default;
            RenderContext& operator=(const RenderContext&) = delete;
            RenderContext& operator=(RenderContext&&) = delete;

            void prepare(size_t thread_count = 1, 
                RenderTarget::CreateFunc create_render_target_func = RenderTarget::CREATE_FUNC);

            void updateSwapchain(const vk::Extent2D& extent);
            void updateSwapchain(const uint32_t image_count);
            void updateSwapchain(const std::set<vk::ImageUsageFlagBits>& image_usage_flags);
            void updateSwapchain(const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform);

            bool hasSwapchain();
            void recreate();
            void recreateSwapchain();

            core::CommandBuffer& begin(core::CommandBuffer::ResetMode reset_mode = core::CommandBuffer::ResetMode::ResetPool);
            void submit(core::CommandBuffer& command_buffer);
            void submit(const std::vector<core::CommandBuffer*>& command_buffers);
            void beginFrame();

            vk::Semaphore submit(const core::Queue& queue,
                const std::vector<core::CommandBuffer*>& command_buffers,
                vk::Semaphore wait_semaphore,
                vk::PipelineStageFlags wait_pipeline_stage);

            void submit(const core::Queue& queue, const std::vector<core::CommandBuffer*>& command_buffers);
            virtual void waitFrame();
            void endFrame(vk::Semaphore semaphore);

            RenderFrame& getActiveFrame();
            uint32_t getActiveFrameIndex();
            RenderFrame& getLastRenderedFrame();
            vk::Semaphore requestSemaphore();
            vk::Semaphore requestSemaphoreWithOwnership();
            void releaseOwnedSemaphore(vk::Semaphore semaphore);
            core::Device& getDevice();
            vk::Format getFormat() const;
            core::Swapchain const& getSwapchain() const;
            vk::Extent2D const& getSurfaceExtent() const;
            uint32_t getActiveFrameIndex() const;
            std::vector<std::unique_ptr<RenderFrame>>& getRenderFrames();
            virtual bool handleSurfaceChanges(bool force_update = false);
            vk::Semaphore consumeAcquiredSemaphore();

        protected:
            vk::Extent2D m_surface_extent;

        private:
            core::Device& m_device;
            const platform::Window& m_window;
            const core::Queue& m_queue;
            std::unique_ptr<core::Swapchain> m_swapchain;
            core::SwapchainProperties m_swapchain_properties;
            std::vector<std::unique_ptr<RenderFrame>> m_frames;
            vk::Semaphore m_acquired_semaphore;
            bool m_prepared{ false };
            uint32_t m_active_frame_index{ 0 };
            bool m_frame_active{ false };
            RenderTarget::CreateFunc m_create_render_target_func = RenderTarget::CREATE_FUNC;
            vk::SurfaceTransformFlagBitsKHR m_pre_transform{ vk::SurfaceTransformFlagBitsKHR::eIdentity };
            size_t m_thread_count{ 1 };
        };
    }
}
