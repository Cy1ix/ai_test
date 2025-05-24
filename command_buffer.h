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

#include "core/query_pool.h"
#include "core/resource_binding_state.h"
#include "rendering/render_target.h"
#include "rendering/pipeline_state.h"

namespace frame {
    namespace rendering {
        struct LightingState;
        class Subpass;
    }

    namespace core {
        class CommandPool;
        class DescriptorSetLayoutCPP;
        class PipelineLayoutCPP;
        class Sampler;
        class ResourceBindingState;
        class FramebufferCPP;

        class CommandBuffer : public VulkanResource<vk::CommandBuffer> {
        public:
            struct RenderPassBinding {
                const RenderPassCPP* render_pass;
                const FramebufferCPP* framebuffer;
            };

            enum class ResetMode : uint32_t {
                ResetPool,
                ResetIndividually,
                AlwaysAllocate,
            };

        public:
            CommandBuffer(CommandPool& command_pool, vk::CommandBufferLevel level);
            CommandBuffer(CommandBuffer&& other);
            ~CommandBuffer();

            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;
            CommandBuffer& operator=(CommandBuffer&&) = delete;

            vk::Result begin(vk::CommandBufferUsageFlags flags, CommandBuffer* primary_cmd_buf = nullptr);
            vk::Result begin(vk::CommandBufferUsageFlags flags, const RenderPassCPP* render_pass, const FramebufferCPP* framebuffer, uint32_t subpass_index);
            void beginQuery(const QueryPool& query_pool, uint32_t query, vk::QueryControlFlags flags);
            void beginRenderPass(const rendering::RenderTarget& render_target,
                const std::vector<rendering::LoadStoreInfo>& load_store_infos,
                const std::vector<vk::ClearValue>& clear_values,
                const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses,
                vk::SubpassContents contents = vk::SubpassContents::eInline);
            void beginRenderPass(const rendering::RenderTarget& render_target,
                const RenderPassCPP& render_pass,
                const FramebufferCPP& framebuffer,
                const std::vector<vk::ClearValue>& clear_values,
                vk::SubpassContents contents = vk::SubpassContents::eInline);
            void bindBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);
            void bindImage(const ImageViewCPP& image_view, const Sampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element);
            void bindImage(const ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element);
            void bindIndexBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::IndexType index_type);
            void bindInput(const ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element);
            void bindLighting(rendering::LightingState& lighting_state, uint32_t set, uint32_t binding);
            void bindPipelineLayout(PipelineLayoutCPP& pipeline_layout);
            void bindVertexBuffers(uint32_t first_binding,
                const std::vector<std::reference_wrapper<const common::Buffer>>& buffers,
                const std::vector<vk::DeviceSize>& offsets);
            void blitImage(const ImageCPP& src_img, const ImageCPP& dst_img, const std::vector<vk::ImageBlit>& regions);
            void bufferMemoryBarrier(const common::Buffer& buffer,
                vk::DeviceSize offset,
                vk::DeviceSize size,
                const common::BufferMemoryBarrier& memory_barrier);
            void clear(vk::ClearAttachment attachment, vk::ClearRect rect);
            void copyBuffer(const common::Buffer& src_buffer, const common::Buffer& dst_buffer, vk::DeviceSize size);
            void copyBufferToImage(const common::Buffer& buffer, const ImageCPP& image, const std::vector<vk::BufferImageCopy>& regions);
            void copyImage(const ImageCPP& src_img, const ImageCPP& dst_img, const std::vector<vk::ImageCopy>& regions);
            void copyImageToBuffer(const ImageCPP& image,
                vk::ImageLayout image_layout,
                const common::Buffer& buffer,
                const std::vector<vk::BufferImageCopy>& regions);
            void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
            void dispatchIndirect(const common::Buffer& buffer, vk::DeviceSize offset);
            void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
            void drawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
            void drawIndexedIndirect(const common::Buffer& buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride);
            vk::Result end();
            void endQuery(const QueryPool& query_pool, uint32_t query);
            void endRenderPass();
            void executeCommands(CommandBuffer& secondary_command_buffer);
            void executeCommands(std::vector<CommandBuffer*>& secondary_command_buffers);
            RenderPassCPP& getRenderPass(const rendering::RenderTarget& render_target,
                const std::vector<rendering::LoadStoreInfo>& load_store_infos,
                const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses);
            void imageMemoryBarrier(const ImageViewCPP& image_view, const common::ImageMemoryBarrier& memory_barrier) const;
            void nextSubpass();
            void pushConstants(const std::vector<uint8_t>& values);

            template <typename T>
            void pushConstants(const T& value)
            {
                auto data = common::toBytes(value);
                uint32_t size = common::toU32(m_stored_push_constants.size() + data.size());

                if (size > m_max_push_constants_size) {
                    LOGE("Push constant limit exceeded ({} / {} bytes)", size, m_max_push_constants_size);
                    throw std::runtime_error("Cannot overflow push constant limit");
                }

                m_stored_push_constants.insert(m_stored_push_constants.end(), data.begin(), data.end());
            }

            vk::Result reset(ResetMode reset_mode);
            void resetQueryPool(const QueryPool& query_pool, uint32_t first_query, uint32_t query_count);
            void resolveImage(const ImageCPP& src_img, const ImageCPP& dst_img, const std::vector<vk::ImageResolve>& regions);
            void setBlendConstants(const std::array<float, 4>& blend_constants);
            void setColorBlendState(const rendering::ColorBlendState& state_info);
            void setDepthBias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);
            void setDepthBounds(float min_depth_bounds, float max_depth_bounds);
            void setDepthStencilState(const rendering::DepthStencilState& state_info);
            void setInputAssemblyState(const rendering::InputAssemblyState& state_info);
            void setLineWidth(float line_width);
            void setMultisampleState(const rendering::MultisampleState& state_info);
            void setRasterizationState(const rendering::RasterizationState& state_info);
            void setScissor(uint32_t first_scissor, const std::vector<vk::Rect2D>& scissors);
            void setSpecializationConstant(uint32_t constant_id, const std::vector<uint8_t>& data);

            template <class T>
            void setSpecializationConstant(uint32_t constant_id, const T& data);

            void setUpdateAfterBind(bool update_after_bind_);
            void setVertexInputState(const rendering::VertexInputState& state_info);
            void setViewport(uint32_t first_viewport, const std::vector<vk::Viewport>& viewports);
            void setViewportState(const rendering::ViewportState& state_info);
            void updateBuffer(const common::Buffer& buffer, vk::DeviceSize offset, const std::vector<uint8_t>& data);
            void writeTimestamp(vk::PipelineStageFlagBits pipeline_stage, const QueryPool& query_pool, uint32_t query);

            void setPipelineState(rendering::PipelineState pipeline_state);

        private:
            void flush(vk::PipelineBindPoint pipeline_bind_point);
            void flushDescriptorState(vk::PipelineBindPoint pipeline_bind_point);
            void flushPipelineState(vk::PipelineBindPoint pipeline_bind_point);
            void flushPushConstants();
            const RenderPassBinding& getCurrentRenderPass() const;
            const uint32_t getCurrentSubpassIndex() const;
            const bool isRenderSizeOptimal(const vk::Extent2D& framebuffer_extent, const vk::Rect2D& render_area);

        private:
            const vk::CommandBufferLevel m_level = {};
            CommandPool& m_command_pool;
            RenderPassBinding m_current_render_pass = {};
            rendering::PipelineState m_pipeline_state = {};
            ResourceBindingState m_resource_binding_state = {};
            std::vector<uint8_t> m_stored_push_constants = {};
            uint32_t m_max_push_constants_size = {};
            vk::Extent2D m_last_framebuffer_extent = {};
            vk::Extent2D m_last_render_area_extent = {};
            bool m_update_after_bind = false;
            std::unordered_map<uint32_t, DescriptorSetLayoutCPP const*> m_descriptor_set_layout_binding_state;
        };

        template <class T>
        inline void CommandBuffer::setSpecializationConstant(uint32_t constant_id, const T& data)
        {
            setSpecializationConstant(constant_id, common::toBytes(data));
        }

        template <>
        inline void CommandBuffer::setSpecializationConstant<bool>(std::uint32_t constant_id, const bool& data)
        {
            setSpecializationConstant(constant_id, common::toBytes(common::toU32(data)));
        }
    }
}
