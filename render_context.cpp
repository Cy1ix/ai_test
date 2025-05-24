/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/render_context.h"
#include "core/queue.h"

namespace frame {
    namespace rendering {
        vk::Format RenderContext::DEFAULT_VK_FORMAT = vk::Format::eR8G8B8A8Srgb;

        RenderContext::RenderContext(core::Device& device,
            vk::SurfaceKHR surface,
            const platform::Window& window,
            vk::PresentModeKHR present_mode,
            std::vector<vk::PresentModeKHR> const& present_mode_priority_list,
            std::vector<vk::SurfaceFormatKHR> const& surface_format_priority_list
            ) :
            m_surface_extent{ window.getExtent().width, window.getExtent().height },
            m_device{ device },
            m_window{ window },
            m_queue{ device.getSuitableGraphicsQueue() }
        {
            if(surface) {
                vk::SurfaceCapabilitiesKHR surface_props = device.getPhysicalDevice().getHandle().getSurfaceCapabilitiesKHR(surface);

                if(surface_props.currentExtent.width == 0xFFFFFFFF) {
                    m_swapchain = std::make_unique<core::Swapchain>(
                        device, surface, present_mode, present_mode_priority_list, surface_format_priority_list, m_surface_extent);
                }
                else {
                    m_swapchain = std::make_unique<core::Swapchain>(
                        device, surface, present_mode, present_mode_priority_list, surface_format_priority_list);
                }
            }
        }

        void RenderContext::prepare(size_t thread_count, rendering::RenderTarget::CreateFunc create_render_target_func) {

            m_device.getHandle().waitIdle();

            if(m_swapchain) {
                m_surface_extent = m_swapchain->getExtent();
                vk::Extent3D extent{ m_surface_extent.width, m_surface_extent.height, 1 };

                for (auto& image_handle : m_swapchain->getImages()) {
                    auto swapchain_image = core::ImageCPP{ m_device, image_handle, extent, m_swapchain->getFormat(), m_swapchain->getUsage(), true };
                    auto render_target = create_render_target_func(std::move(swapchain_image));
                    m_frames.emplace_back(std::make_unique<rendering::RenderFrame>(m_device, std::move(render_target), thread_count));
                }
            }
            else {
                auto color_image = core::ImageCPP{
                    m_device,
                    vk::Extent3D{m_surface_extent.width, m_surface_extent.height, 1},
                    DEFAULT_VK_FORMAT,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                    VMA_MEMORY_USAGE_GPU_ONLY
                };

                auto render_target = create_render_target_func(std::move(color_image));
                m_frames.emplace_back(std::make_unique<rendering::RenderFrame>(m_device, std::move(render_target), thread_count));
            }

            m_create_render_target_func = create_render_target_func;
            m_thread_count = thread_count;
            m_prepared = true;
        }

        vk::Format RenderContext::getFormat() const {
            return m_swapchain ? m_swapchain->getFormat() : DEFAULT_VK_FORMAT;
        }

        void RenderContext::updateSwapchain(const vk::Extent2D& extent) {
            if(!m_swapchain) {
                LOGW("Can't update the swapchains extent. No swapchain, offscreen rendering detected, skipping.");
                    return;
            }

            m_device.getResourceCache().clearFramebuffers();
            m_swapchain = std::make_unique<core::Swapchain>(*m_swapchain, extent);
            recreate();
        }

        void RenderContext::updateSwapchain(const uint32_t image_count) {
            if(!m_swapchain) {
                LOGW("Can't update the swapchains image count. No swapchain, offscreen rendering detected, skipping.");
                    return;
            }

            m_device.getResourceCache().clearFramebuffers();
            m_device.getHandle().waitIdle();
            m_swapchain = std::make_unique<core::Swapchain>(*m_swapchain, image_count);
            recreate();
        }

        void RenderContext::updateSwapchain(const std::set<vk::ImageUsageFlagBits>& image_usage_flags) {
            if(!m_swapchain) {
                LOGW("Can't update the swapchains image usage. No swapchain, offscreen rendering detected, skipping.");
                    return;
            }

            m_device.getResourceCache().clearFramebuffers();
            m_swapchain = std::make_unique<core::Swapchain>(*m_swapchain, image_usage_flags);
            recreate();
        }

        void RenderContext::updateSwapchain(const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform) {
            if(!m_swapchain) {
                LOGW("Can't update the swapchains extent and surface transform. No swapchain, offscreen rendering detected, skipping.");
                    return;
            }

            m_device.getResourceCache().clearFramebuffers();

            auto width = extent.width;
            auto height = extent.height;
            if(transform == vk::SurfaceTransformFlagBitsKHR::eRotate90 || transform == vk::SurfaceTransformFlagBitsKHR::eRotate270) {
                std::swap(width, height);
            }

            m_swapchain = std::make_unique<core::Swapchain>(*m_swapchain, vk::Extent2D{ width, height }, transform);
            m_pre_transform = transform;
            recreate();
        }

        void RenderContext::recreate() {
            LOGI("Recreated swapchain");

                vk::Extent2D swapchain_extent = m_swapchain->getExtent();
            vk::Extent3D extent{ swapchain_extent.width, swapchain_extent.height, 1 };

            auto frame_it = m_frames.begin();

            for (auto& image_handle : m_swapchain->getImages()) {
                core::ImageCPP swapchain_image{ m_device, image_handle, extent, m_swapchain->getFormat(), m_swapchain->getUsage() };
                auto render_target = m_create_render_target_func(std::move(swapchain_image));

                if(frame_it != m_frames.end()) {
                    (*frame_it)->updateRenderTarget(std::move(render_target));
                }
                else {
                    m_frames.emplace_back(std::make_unique<rendering::RenderFrame>(m_device, std::move(render_target), m_thread_count));
                }

                ++frame_it;
            }

            m_device.getResourceCache().clearFramebuffers();
        }

        bool RenderContext::hasSwapchain() {
            return m_swapchain != nullptr;
        }

        bool RenderContext::handleSurfaceChanges(bool force_update) {
            if(!m_swapchain) {
                LOGW("Can't handle surface changes. No swapchain, offscreen rendering detected, skipping.");
                    return false;
            }

            vk::SurfaceCapabilitiesKHR surface_props = m_device.getPhysicalDevice().getHandle().getSurfaceCapabilitiesKHR(m_swapchain->getSurface());

            if(surface_props.currentExtent.width == 0xFFFFFFFF) {
                return false;
            }

            if(surface_props.currentExtent.width != m_surface_extent.width ||
                surface_props.currentExtent.height != m_surface_extent.height ||
                force_update)
            {
                m_device.getHandle().waitIdle();
                updateSwapchain(surface_props.currentExtent, m_pre_transform);
                m_surface_extent = surface_props.currentExtent;
                return true;
            }

            return false;
        }

        core::CommandBuffer& RenderContext::begin(core::CommandBuffer::ResetMode reset_mode) {
            assert(m_prepared && "[RenderContext] ASSERT: RenderContext not prepared for rendering, call prepare()");

            if(!m_frame_active) {
                beginFrame();
            }

            if(!m_acquired_semaphore) {
                throw std::runtime_error("[RenderContext] ERROR: Couldn't begin frame");
            }

            const auto& queue = m_device.getQueueByFlags(vk::QueueFlagBits::eGraphics, 0);
            return getActiveFrame().requestCommandBuffer(queue, reset_mode);
        }

        void RenderContext::submit(core::CommandBuffer& command_buffer) {
            submit({ &command_buffer });
        }

        void RenderContext::submit(const std::vector<core::CommandBuffer*>& command_buffers) {

            assert(m_frame_active && "[RenderContext] ASSERT: RenderContext is inactive, cannot submit command buffer. Please call begin()");

            vk::Semaphore render_semaphore;

            if(m_swapchain) {
                assert(m_acquired_semaphore && "[RenderContext] ASSERT: We do not have acquired_semaphore, it was probably consumed?");
                render_semaphore = submit(m_queue, command_buffers, m_acquired_semaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
            }
            else {
                submit(m_queue, command_buffers);
            }

            endFrame(render_semaphore);
        }

        void RenderContext::beginFrame() {
            if(m_swapchain) {
                handleSurfaceChanges();
            }

            assert(!m_frame_active && "[RenderContext] ASSERT: Frame is still active, please call endFrame");

            auto& prev_frame = *m_frames[m_active_frame_index];
            m_acquired_semaphore = prev_frame.requestSemaphoreWithOwnership();

            if(m_swapchain) {
                vk::Result result;
                try {
                    std::tie(result, m_active_frame_index) = m_swapchain->acquireNextImage(m_acquired_semaphore);
                }
                catch (vk::OutOfDateKHRError&) {
                    result = vk::Result::eErrorOutOfDateKHR;
                }

                if(result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {

#if defined(PLATFORM__MACOS)
                    bool swapchain_updated = handleSurfaceChanges(true);
#else
                    bool swapchain_updated = handleSurfaceChanges(result == vk::Result::eErrorOutOfDateKHR);
#endif

                    if(swapchain_updated) {
                        m_device.getHandle().destroySemaphore(m_acquired_semaphore);
                        m_acquired_semaphore = prev_frame.requestSemaphoreWithOwnership();
                        std::tie(result, m_active_frame_index) = m_swapchain->acquireNextImage(m_acquired_semaphore);
                    }
                }

                if(result != vk::Result::eSuccess) {
                    prev_frame.reset();
                    return;
                }
            }

            m_frame_active = true;
            waitFrame();
        }

        vk::Semaphore RenderContext::submit(const core::Queue& queue,
            const std::vector<core::CommandBuffer*>& command_buffers,
            vk::Semaphore wait_semaphore,
            vk::PipelineStageFlags wait_pipeline_stage)
        {
            std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
            std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(),
                [](const core::CommandBuffer* cmd_buf) { return cmd_buf->getHandle(); });

            RenderFrame& frame = getActiveFrame();
            vk::Semaphore signal_semaphore = frame.requestSemaphore();

            vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles, signal_semaphore);
            if(wait_semaphore) {
                submit_info.setWaitSemaphores(wait_semaphore);
                submit_info.pWaitDstStageMask = &wait_pipeline_stage;
            }

            vk::Fence fence = frame.requestFence();
            queue.getHandle().submit(submit_info, fence);

            return signal_semaphore;
        }

        void RenderContext::submit(const core::Queue& queue, const std::vector<core::CommandBuffer*>& command_buffers) {

            std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
            std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(),
                [](const core::CommandBuffer* cmd_buf) { return cmd_buf->getHandle(); });

            RenderFrame& frame = getActiveFrame();
            vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles);
            vk::Fence fence = frame.requestFence();
            queue.getHandle().submit(submit_info, fence);
        }

        void RenderContext::waitFrame() {
            getActiveFrame().reset();
        }

        void RenderContext::endFrame(vk::Semaphore semaphore) {

            assert(m_frame_active && "[RenderContext] ASSERT: Frame is not active, please call beginFrame");

            if(m_swapchain) {
                vk::SwapchainKHR vk_swapchain = m_swapchain->getHandle();
                vk::PresentInfoKHR present_info(semaphore, vk_swapchain, m_active_frame_index);

                vk::DisplayPresentInfoKHR disp_present_info;
                if(m_device.isExtensionSupported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
                    m_window.getDisplayPresentInfo(reinterpret_cast<VkDisplayPresentInfoKHR*>(&disp_present_info),
                        m_surface_extent.width, m_surface_extent.height))
                {
                    present_info.pNext = &disp_present_info;
                }

                vk::Result result;
                try {
                    result = m_queue.present(present_info);
                }
                catch (vk::OutOfDateKHRError&) {
                    result = vk::Result::eErrorOutOfDateKHR;
                }

                if(result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
                    handleSurfaceChanges();
                }
            }

            if(m_acquired_semaphore) {
                releaseOwnedSemaphore(m_acquired_semaphore);
                m_acquired_semaphore = nullptr;
            }
            m_frame_active = false;
        }

        vk::Semaphore RenderContext::consumeAcquiredSemaphore() {
            assert(m_frame_active && "[RenderContext] ASSERT: Frame is not active, please call beginFrame");
            return std::exchange(m_acquired_semaphore, nullptr);
        }

        RenderFrame& RenderContext::getActiveFrame() {
            assert(m_frame_active && "[RenderContext] ASSERT: Frame is not active, please call beginFrame");
            return *m_frames[m_active_frame_index];
        }

        uint32_t RenderContext::getActiveFrameIndex() {
            assert(m_frame_active && "[RenderContext] ASSERT: Frame is not active, please call beginFrame");
            return m_active_frame_index;
        }

        RenderFrame& RenderContext::getLastRenderedFrame() {
            assert(!m_frame_active && "[RenderContext] ASSERT: Frame is still active, please call endFrame");
            return *m_frames[m_active_frame_index];
        }

        vk::Semaphore RenderContext::requestSemaphore() {
            return getActiveFrame().requestSemaphore();
        }

        vk::Semaphore RenderContext::requestSemaphoreWithOwnership() {
            return getActiveFrame().requestSemaphoreWithOwnership();
        }

        void RenderContext::releaseOwnedSemaphore(vk::Semaphore semaphore) {
            getActiveFrame().releaseOwnedSemaphore(semaphore);
        }

        core::Device& RenderContext::getDevice() {
            return m_device;
        }

        void RenderContext::recreateSwapchain() {

            m_device.getHandle().waitIdle();
            m_device.getResourceCache().clearFramebuffers();

            vk::Extent2D swapchain_extent = m_swapchain->getExtent();
            vk::Extent3D extent{ swapchain_extent.width, swapchain_extent.height, 1 };

            auto frame_it = m_frames.begin();

            for (auto& image_handle : m_swapchain->getImages()) {
                core::ImageCPP swapchain_image{ m_device, image_handle, extent, m_swapchain->getFormat(), m_swapchain->getUsage() };
                auto render_target = m_create_render_target_func(std::move(swapchain_image));
                (*frame_it)->updateRenderTarget(std::move(render_target));
                ++frame_it;
            }
        }

        core::Swapchain const& RenderContext::getSwapchain() const {
            assert(m_swapchain && "[RenderContext] ASSERT: Swapchain is not valid");
            return *m_swapchain;
        }

        vk::Extent2D const& RenderContext::getSurfaceExtent() const {
            return m_surface_extent;
        }

        uint32_t RenderContext::getActiveFrameIndex() const {
            return m_active_frame_index;
        }

        std::vector<std::unique_ptr<RenderFrame>>& RenderContext::getRenderFrames() {
            return m_frames;
        }
    }
}
