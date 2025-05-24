/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/command_buffer.h"
#include "core/command_pool.h"
#include "core/pipeline.h"
#include "core/device.h"
#include "rendering/subpass.h"
#include "rendering/render_frame.h"
#include "core/sampler.h"

namespace frame {
    namespace core {
        CommandBuffer::CommandBuffer(CommandPool& command_pool, vk::CommandBufferLevel level) :
            VulkanResource(nullptr, &command_pool.getDevice()),
            m_level(level),
            m_command_pool(command_pool),
            m_max_push_constants_size(getDevice().getPhysicalDevice().getProperties().limits.maxPushConstantsSize)
        {
            vk::CommandBufferAllocateInfo allocate_info(command_pool.getHandle(), level, 1);
            setHandle(getDevice().getHandle().allocateCommandBuffers(allocate_info).front());
        }

        CommandBuffer::CommandBuffer(CommandBuffer&& other) :
            VulkanResource(std::move(other)),
            m_level(other.m_level),
            m_command_pool(other.m_command_pool),
            m_current_render_pass(std::exchange(other.m_current_render_pass, {})),
            m_pipeline_state(std::exchange(other.m_pipeline_state, {})),
            m_resource_binding_state(std::exchange(other.m_resource_binding_state, {})),
            m_stored_push_constants(std::exchange(other.m_stored_push_constants, {})),
            m_max_push_constants_size(std::exchange(other.m_max_push_constants_size, {})),
            m_last_framebuffer_extent(std::exchange(other.m_last_framebuffer_extent, {})),
            m_last_render_area_extent(std::exchange(other.m_last_render_area_extent, {})),
            m_update_after_bind(std::exchange(other.m_update_after_bind, {})),
            m_descriptor_set_layout_binding_state(std::exchange(other.m_descriptor_set_layout_binding_state, {}))
        {
        }

        CommandBuffer::~CommandBuffer()
        {
            if (getHandle())
            {
                getDevice().getHandle().freeCommandBuffers(m_command_pool.getHandle(), getHandle());
            }
        }

        vk::Result CommandBuffer::begin(vk::CommandBufferUsageFlags flags, CommandBuffer* primary_cmd_buf)
        {
            if (m_level == vk::CommandBufferLevel::eSecondary)
            {
                assert(primary_cmd_buf && "A primary command buffer pointer must be provided when calling begin from a secondary one");
                auto const& render_pass_binding = primary_cmd_buf->getCurrentRenderPass();
                return begin(flags, render_pass_binding.render_pass, render_pass_binding.framebuffer, primary_cmd_buf->getCurrentSubpassIndex());
            }
            return begin(flags, nullptr, nullptr, 0);
        }

        vk::Result CommandBuffer::begin(vk::CommandBufferUsageFlags flags, 
            const RenderPassCPP* render_pass, 
            const FramebufferCPP* framebuffer, 
            uint32_t subpass_index)
        {
            m_pipeline_state.reset();
            m_resource_binding_state.reset();
            m_descriptor_set_layout_binding_state.clear();
            m_stored_push_constants.clear();

            vk::CommandBufferBeginInfo begin_info(flags);
            vk::CommandBufferInheritanceInfo inheritance;

            if (m_level == vk::CommandBufferLevel::eSecondary)
            {
                assert((render_pass && framebuffer) && "Render pass and framebuffer must be provided when calling begin from a secondary one");

                m_current_render_pass.render_pass = render_pass;
                m_current_render_pass.framebuffer = framebuffer;

                inheritance.renderPass = m_current_render_pass.render_pass->getHandle();
                inheritance.framebuffer = m_current_render_pass.framebuffer->getHandle();
                inheritance.subpass = subpass_index;

                begin_info.pInheritanceInfo = &inheritance;
            }

            getHandle().begin(begin_info);
            return vk::Result::eSuccess;
        }

        void CommandBuffer::beginQuery(const QueryPool& query_pool, uint32_t query, vk::QueryControlFlags flags) {
            getHandle().beginQuery(query_pool.getHandle(), query, flags);
        }

        void CommandBuffer::beginRenderPass(const rendering::RenderTarget& render_target,
            const std::vector<rendering::LoadStoreInfo>& load_store_infos,
            const std::vector<vk::ClearValue>& clear_values,
            const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses,
            vk::SubpassContents contents)
        {
            m_pipeline_state.reset();
            m_resource_binding_state.reset();
            m_descriptor_set_layout_binding_state.clear();

            auto& render_pass = getRenderPass(render_target, load_store_infos, subpasses);
            auto& framebuffer = getDevice().getResourceCache().requestFramebuffer(render_target, render_pass);

            beginRenderPass(render_target, render_pass, framebuffer, clear_values, contents);
        }

        void CommandBuffer::beginRenderPass(const rendering::RenderTarget& render_target,
            const RenderPassCPP& render_pass,
            const FramebufferCPP& framebuffer,
            const std::vector<vk::ClearValue>& clear_values,
            vk::SubpassContents contents)
        {
            m_current_render_pass.render_pass = &render_pass;
            m_current_render_pass.framebuffer = &framebuffer;

            vk::RenderPassBeginInfo begin_info(
                m_current_render_pass.render_pass->getHandle(),
                m_current_render_pass.framebuffer->getHandle(),
                { {}, render_target.getExtent() },
                clear_values);

            const auto& framebuffer_extent = m_current_render_pass.framebuffer->getExtent();

            if (!isRenderSizeOptimal(framebuffer_extent, begin_info.renderArea)) {
                if ((framebuffer_extent != m_last_framebuffer_extent) || (begin_info.renderArea.extent != m_last_render_area_extent)) {
                    LOGW("Render target extent is not an optimal size, this may result in reduced performance.");
                }

                m_last_framebuffer_extent = framebuffer_extent;
                m_last_render_area_extent = begin_info.renderArea.extent;
            }

            getHandle().beginRenderPass(begin_info, contents);

            auto blend_state = m_pipeline_state.getColorBlendState();
            blend_state.attachments.resize(m_current_render_pass.render_pass->getColorOutputCount(m_pipeline_state.getSubpassIndex()));
            m_pipeline_state.setColorBlendState(blend_state);
        }

        void CommandBuffer::bindBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            m_resource_binding_state.bindBuffer(buffer, offset, range, set, binding, array_element);
        }

        void CommandBuffer::bindImage(const ImageViewCPP& image_view, const Sampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            m_resource_binding_state.bindImage(image_view, sampler, set, binding, array_element);
        }

        void CommandBuffer::bindImage(const ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            m_resource_binding_state.bindImage(image_view, set, binding, array_element);
        }

        void CommandBuffer::bindIndexBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::IndexType index_type)
        {
            getHandle().bindIndexBuffer(buffer.getHandle(), offset, index_type);
        }

        void CommandBuffer::bindInput(const ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            m_resource_binding_state.bindInput(image_view, set, binding, array_element);
        }

        void CommandBuffer::bindLighting(rendering::LightingState& lighting_state, uint32_t set, uint32_t binding)
        {
            bindBuffer(lighting_state.light_buffer.getBuffer(), lighting_state.light_buffer.getOffset(), lighting_state.light_buffer.getSize(), set, binding, 0);

            setSpecializationConstant(0, common::toU32(lighting_state.directional_lights.size()));
            setSpecializationConstant(1, common::toU32(lighting_state.point_lights.size()));
            setSpecializationConstant(2, common::toU32(lighting_state.spot_lights.size()));
        }

        void CommandBuffer::bindPipelineLayout(PipelineLayoutCPP& pipeline_layout)
        {
            m_pipeline_state.setPipelineLayout(pipeline_layout);
        }

        void CommandBuffer::bindVertexBuffers(uint32_t first_binding,
            const std::vector<std::reference_wrapper<const common::Buffer>>& buffers,
            const std::vector<vk::DeviceSize>& offsets)
        {
            std::vector<vk::Buffer> buffer_handles(buffers.size(), nullptr);
            std::transform(buffers.begin(), buffers.end(), buffer_handles.begin(),
                [](const common::Buffer& buffer) { return buffer.getHandle(); });
            getHandle().bindVertexBuffers(first_binding, buffer_handles, offsets);
        }

        void CommandBuffer::blitImage(const ImageCPP& src_img, const ImageCPP& dst_img, const std::vector<vk::ImageBlit>& regions)
        {
            getHandle().blitImage(
                src_img.getHandle(), vk::ImageLayout::eTransferSrcOptimal,
                dst_img.getHandle(), vk::ImageLayout::eTransferDstOptimal,
                regions, vk::Filter::eNearest);
        }

        void CommandBuffer::bufferMemoryBarrier(const common::Buffer& buffer,
            vk::DeviceSize offset,
            vk::DeviceSize size,
            const common::BufferMemoryBarrier& memory_barrier)
        {
            vk::BufferMemoryBarrier buffer_memory_barrier(
                memory_barrier.m_src_access_mask, memory_barrier.m_dst_access_mask,
                {}, {}, buffer.getHandle(), offset, size);

            vk::PipelineStageFlags src_stage_mask = memory_barrier.m_src_stage_mask;
            vk::PipelineStageFlags dst_stage_mask = memory_barrier.m_dst_stage_mask;

            getHandle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, buffer_memory_barrier, {});
        }

        void CommandBuffer::clear(vk::ClearAttachment attachment, vk::ClearRect rect)
        {
            getHandle().clearAttachments(attachment, rect);
        }

        void CommandBuffer::copyBuffer(const common::Buffer& src_buffer, const common::Buffer& dst_buffer, vk::DeviceSize size)
        {
            vk::BufferCopy copy_region({}, {}, size);
            getHandle().copyBuffer(src_buffer.getHandle(), dst_buffer.getHandle(), copy_region);
        }

        void CommandBuffer::copyBufferToImage(const common::Buffer& buffer,
            const ImageCPP& image,
            const std::vector<vk::BufferImageCopy>& regions)
        {
            getHandle().copyBufferToImage(buffer.getHandle(), image.getHandle(), vk::ImageLayout::eTransferDstOptimal, regions);
        }

        void CommandBuffer::copyImage(const ImageCPP& src_img, const ImageCPP& dst_img, const std::vector<vk::ImageCopy>& regions)
        {
            getHandle().copyImage(
                src_img.getHandle(), vk::ImageLayout::eTransferSrcOptimal,
                dst_img.getHandle(), vk::ImageLayout::eTransferDstOptimal,
                regions);
        }

        void CommandBuffer::copyImageToBuffer(const ImageCPP& image,
            vk::ImageLayout image_layout,
            const common::Buffer& buffer,
            const std::vector<vk::BufferImageCopy>& regions)
        {
            getHandle().copyImageToBuffer(image.getHandle(), image_layout, buffer.getHandle(), regions);
        }

        void CommandBuffer::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
        {
            flush(vk::PipelineBindPoint::eCompute);
            getHandle().dispatch(group_count_x, group_count_y, group_count_z);
        }

        void CommandBuffer::dispatchIndirect(const common::Buffer& buffer, vk::DeviceSize offset)
        {
            flush(vk::PipelineBindPoint::eCompute);
            getHandle().dispatchIndirect(buffer.getHandle(), offset);
        }

        void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
        {
            flush(vk::PipelineBindPoint::eGraphics);
            getHandle().draw(vertex_count, instance_count, first_vertex, first_instance);
        }

        void CommandBuffer::drawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
        {
            flush(vk::PipelineBindPoint::eGraphics);
            getHandle().drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
        }

        void CommandBuffer::drawIndexedIndirect(const common::Buffer& buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride)
        {
            flush(vk::PipelineBindPoint::eGraphics);
            getHandle().drawIndexedIndirect(buffer.getHandle(), offset, draw_count, stride);
        }

        vk::Result CommandBuffer::end()
        {
            getHandle().end();
            return vk::Result::eSuccess;
        }

        void CommandBuffer::endQuery(const QueryPool& query_pool, uint32_t query)
        {
            getHandle().endQuery(query_pool.getHandle(), query);
        }

        void CommandBuffer::endRenderPass()
        {
            getHandle().endRenderPass();
        }

        void CommandBuffer::executeCommands(CommandBuffer& secondary_command_buffer)
        {
            getHandle().executeCommands(secondary_command_buffer.getHandle());
        }

        void CommandBuffer::executeCommands(std::vector<CommandBuffer*>& secondary_command_buffers)
        {
            std::vector<vk::CommandBuffer> sec_cmd_buf_handles(secondary_command_buffers.size(), nullptr);
            std::transform(secondary_command_buffers.begin(),
                secondary_command_buffers.end(),
                sec_cmd_buf_handles.begin(),
                [](const CommandBuffer* sec_cmd_buf) { return sec_cmd_buf->getHandle(); });
            getHandle().executeCommands(sec_cmd_buf_handles);
        }

        RenderPassCPP& CommandBuffer::getRenderPass(const rendering::RenderTarget& render_target,
            const std::vector<rendering::LoadStoreInfo>& load_store_infos,
            const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses)
        {
            assert(subpasses.size() > 0 && "Cannot create a render pass without any subpass");

            std::vector<SubpassInfo> subpass_infos(subpasses.size());
            auto subpass_info_it = subpass_infos.begin();
            for (auto& subpass : subpasses)
            {
                subpass_info_it->input_attachments = subpass->getInputAttachments();
                subpass_info_it->output_attachments = subpass->getOutputAttachments();
                subpass_info_it->color_resolve_attachments = subpass->getColorResolveAttachments();
                subpass_info_it->disable_depth_stencil_attachment = subpass->getDisableDepthStencilAttachment();
                subpass_info_it->depth_stencil_resolve_mode = subpass->getDepthStencilResolveMode();
                subpass_info_it->depth_stencil_resolve_attachment = subpass->getDepthStencilResolveAttachment();
                subpass_info_it->debug_name = subpass->getDebugName();

                ++subpass_info_it;
            }

            return getDevice().getResourceCache().requestRenderPass(render_target.getAttachments(), load_store_infos, subpass_infos);
        }

        void CommandBuffer::imageMemoryBarrier(const ImageViewCPP& image_view, const common::ImageMemoryBarrier& memory_barrier) const
        {
            auto subresource_range = image_view.getSubresourceRange();
            auto format = image_view.getFormat();
            if (common::isDepthOnlyFormat(format))
            {
                subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
            }
            else if (common::isDepthStencilFormat(format))
            {
                subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            }

            vk::ImageMemoryBarrier image_memory_barrier(
                memory_barrier.m_src_access_mask,
                memory_barrier.m_dst_access_mask,
                memory_barrier.m_old_layout,
                memory_barrier.m_new_layout,
                memory_barrier.m_old_queue_family,
                memory_barrier.m_new_queue_family,
                image_view.getImage().getHandle(),
                subresource_range);

            vk::PipelineStageFlags src_stage_mask = memory_barrier.m_src_stage_mask;
            vk::PipelineStageFlags dst_stage_mask = memory_barrier.m_dst_stage_mask;

            getHandle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, image_memory_barrier);
        }

        void CommandBuffer::nextSubpass()
        {
            m_pipeline_state.setSubpassIndex(m_pipeline_state.getSubpassIndex() + 1);

            auto blend_state = m_pipeline_state.getColorBlendState();
            blend_state.attachments.resize(m_current_render_pass.render_pass->getColorOutputCount(m_pipeline_state.getSubpassIndex()));
            m_pipeline_state.setColorBlendState(blend_state);

            m_resource_binding_state.reset();
            m_descriptor_set_layout_binding_state.clear();
            m_stored_push_constants.clear();

            getHandle().nextSubpass(vk::SubpassContents::eInline);
        }

        void CommandBuffer::pushConstants(const std::vector<uint8_t>& values)
        {
            uint32_t push_constant_size = common::toU32(m_stored_push_constants.size() + values.size());

            if (push_constant_size > m_max_push_constants_size)
            {
                LOGE("Push constant limit of {} exceeded (pushing {} bytes for a total of {} bytes)",
                    m_max_push_constants_size, values.size(), push_constant_size);
                throw std::runtime_error("Push constant limit exceeded.");
            }
            else
            {
                m_stored_push_constants.insert(m_stored_push_constants.end(), values.begin(), values.end());
            }
        }

        vk::Result CommandBuffer::reset(ResetMode reset_mode)
        {
            assert(reset_mode == m_command_pool.getResetMode() &&
                "Command buffer reset mode must match the one used by the pool to allocate it");

            if (reset_mode == ResetMode::ResetIndividually)
            {
                getHandle().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
            }

            return vk::Result::eSuccess;
        }

        void CommandBuffer::resetQueryPool(const QueryPool& query_pool, uint32_t first_query, uint32_t query_count)
        {
            getHandle().resetQueryPool(query_pool.getHandle(), first_query, query_count);
        }

        void CommandBuffer::resolveImage(const ImageCPP& src_img, const ImageCPP& dst_img, const std::vector<vk::ImageResolve>& regions)
        {
            getHandle().resolveImage(
                src_img.getHandle(), vk::ImageLayout::eTransferSrcOptimal,
                dst_img.getHandle(), vk::ImageLayout::eTransferDstOptimal,
                regions);
        }

        void CommandBuffer::setBlendConstants(const std::array<float, 4>& blend_constants)
        {
            getHandle().setBlendConstants(blend_constants.data());
        }

        void CommandBuffer::setColorBlendState(const rendering::ColorBlendState& state_info)
        {
            m_pipeline_state.setColorBlendState(state_info);
        }

        void CommandBuffer::setDepthBias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor)
        {
            getHandle().setDepthBias(depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
        }

        void CommandBuffer::setDepthBounds(float min_depth_bounds, float max_depth_bounds)
        {
            getHandle().setDepthBounds(min_depth_bounds, max_depth_bounds);
        }

        void CommandBuffer::setDepthStencilState(const rendering::DepthStencilState& state_info)
        {
            m_pipeline_state.setDepthStencilState(state_info);
        }

        void CommandBuffer::setInputAssemblyState(const rendering::InputAssemblyState& state_info)
        {
            m_pipeline_state.setInputAssemblyState(state_info);
        }

        void CommandBuffer::setLineWidth(float line_width)
        {
            getHandle().setLineWidth(line_width);
        }

        void CommandBuffer::setMultisampleState(const rendering::MultisampleState& state_info)
        {
            m_pipeline_state.setMultisampleState(state_info);
        }

        void CommandBuffer::setRasterizationState(const rendering::RasterizationState& state_info)
        {
            m_pipeline_state.setRasterizationState(state_info);
        }

        void CommandBuffer::setScissor(uint32_t first_scissor, const std::vector<vk::Rect2D>& scissors)
        {
            getHandle().setScissor(first_scissor, scissors);
        }

        void CommandBuffer::setSpecializationConstant(uint32_t constant_id, const std::vector<uint8_t>& data)
        {
            m_pipeline_state.setSpecializationConstant(constant_id, data);
        }

        void CommandBuffer::setUpdateAfterBind(bool update_after_bind_)
        {
            m_update_after_bind = update_after_bind_;
        }

        void CommandBuffer::setVertexInputState(const rendering::VertexInputState& state_info)
        {
            m_pipeline_state.setVertexInputState(state_info);
        }

        void CommandBuffer::setViewport(uint32_t first_viewport, const std::vector<vk::Viewport>& viewports)
        {
            getHandle().setViewport(first_viewport, viewports);
        }

        void CommandBuffer::setViewportState(const rendering::ViewportState& state_info)
        {
            m_pipeline_state.setViewportState(state_info);
        }

        void CommandBuffer::updateBuffer(const common::Buffer& buffer, vk::DeviceSize offset, const std::vector<uint8_t>& data)
        {
            getHandle().updateBuffer<uint8_t>(buffer.getHandle(), offset, data);
        }

        void CommandBuffer::writeTimestamp(vk::PipelineStageFlagBits pipeline_stage, const QueryPool& query_pool, uint32_t query)
        {
            getHandle().writeTimestamp(pipeline_stage, query_pool.getHandle(), query);
        }

        void CommandBuffer::setPipelineState(rendering::PipelineState pipeline_state)
        {
            m_pipeline_state = pipeline_state;
        }

        void CommandBuffer::flush(vk::PipelineBindPoint pipeline_bind_point)
        {
            flushPipelineState(pipeline_bind_point);
            flushPushConstants();
            flushDescriptorState(pipeline_bind_point);
        }

        void CommandBuffer::flushDescriptorState(vk::PipelineBindPoint pipeline_bind_point)
        {
            assert(m_command_pool.getRenderFrame() && "The command pool must be associated to a render frame");

            const auto& pipeline_layout = m_pipeline_state.getPipelineLayout();
            std::unordered_set<uint32_t> update_descriptor_sets;

            for (auto& set_it : pipeline_layout.getShaderSets())
            {
                uint32_t descriptor_set_id = set_it.first;
                auto descriptor_set_layout_it = m_descriptor_set_layout_binding_state.find(descriptor_set_id);

                if (descriptor_set_layout_it != m_descriptor_set_layout_binding_state.end())
                {
                    if (descriptor_set_layout_it->second->getHandle() != pipeline_layout.getDescriptorSetLayout(descriptor_set_id).getHandle())
                    {
                        update_descriptor_sets.emplace(descriptor_set_id);
                    }
                }
            }

            for (auto set_it = m_descriptor_set_layout_binding_state.begin(); set_it != m_descriptor_set_layout_binding_state.end();)
            {
                if (!pipeline_layout.hasDescriptorSetLayout(set_it->first))
                {
                    set_it = m_descriptor_set_layout_binding_state.erase(set_it);
                }
                else
                {
                    ++set_it;
                }
            }

            if (m_resource_binding_state.isDirty() || !update_descriptor_sets.empty())
            {
                m_resource_binding_state.clearDirty();

                for (auto& resource_set_it : m_resource_binding_state.getResourceSets())
                {
                    uint32_t descriptor_set_id = resource_set_it.first;
                    auto& resource_set = resource_set_it.second;

                    if (!resource_set.isDirty() && (update_descriptor_sets.find(descriptor_set_id) == update_descriptor_sets.end()))
                    {
                        continue;
                    }

                    m_resource_binding_state.clearDirty(descriptor_set_id);

                    if (!pipeline_layout.hasDescriptorSetLayout(descriptor_set_id))
                    {
                        continue;
                    }

                    auto& descriptor_set_layout = pipeline_layout.getDescriptorSetLayout(descriptor_set_id);
                    m_descriptor_set_layout_binding_state[descriptor_set_id] = &descriptor_set_layout;

                    BindingMap<vk::DescriptorBufferInfo> buffer_infos;
                    BindingMap<vk::DescriptorImageInfo> image_infos;
                    std::vector<uint32_t> dynamic_offsets;

                    for (auto& binding_it : resource_set.getResourceBindings())
                    {
                        auto binding_index = binding_it.first;
                        auto& binding_resources = binding_it.second;

                        if (auto binding_info = descriptor_set_layout.getLayoutBinding(binding_index))
                        {
                            for (auto& element_it : binding_resources)
                            {
                                auto array_element = element_it.first;
                                auto& resource_info = element_it.second;

                                auto& buffer = resource_info.m_buffer;
                                auto& sampler = resource_info.m_sampler;
                                auto& image_view = resource_info.m_image_view;

                                if (buffer != nullptr && common::isBufferDescriptorType(binding_info->descriptorType))
                                {
                                    vk::DescriptorBufferInfo buffer_info(resource_info.m_buffer->getHandle(), resource_info.m_offset, resource_info.m_range);

                                    if (common::isDynamicBufferDescriptorType(binding_info->descriptorType))
                                    {
                                        dynamic_offsets.push_back(common::toU32(buffer_info.offset));
                                        buffer_info.offset = 0;
                                    }

                                    buffer_infos[binding_index][array_element] = buffer_info;
                                }
                                else if (image_view != nullptr || sampler != nullptr)
                                {
                                    vk::DescriptorImageInfo image_info(sampler ? sampler->getHandle() : nullptr, image_view->getHandle());

                                    if (image_view != nullptr)
                                    {
                                        switch (binding_info->descriptorType)
                                        {
                                        case vk::DescriptorType::eCombinedImageSampler:
                                            image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                                            break;
                                        case vk::DescriptorType::eInputAttachment:
                                            image_info.imageLayout = common::isDepthFormat(image_view->getFormat()) ?
                                                vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
                                            break;
                                        case vk::DescriptorType::eStorageImage:
                                            image_info.imageLayout = vk::ImageLayout::eGeneral;
                                            break;
                                        default:
                                            continue;
                                        }
                                    }

                                    image_infos[binding_index][array_element] = image_info;
                                }
                            }

                            assert((!m_update_after_bind ||
                                (buffer_infos.count(binding_index) > 0 || (image_infos.count(binding_index) > 0))) &&
                                "binding index with no buffer or image infos can't be checked for adding to bindings_to_update");
                        }
                    }

                    vk::DescriptorSet descriptor_set_handle = m_command_pool.getRenderFrame()->requestDescriptorSet(
                        descriptor_set_layout, buffer_infos, image_infos, m_update_after_bind, m_command_pool.getThreadIndex());

                    getHandle().bindDescriptorSets(
                        pipeline_bind_point, pipeline_layout.getHandle(), descriptor_set_id, descriptor_set_handle, dynamic_offsets);
                }
            }
        }

        void CommandBuffer::flushPipelineState(vk::PipelineBindPoint pipeline_bind_point)
        {
            if (!m_pipeline_state.isDirty())
            {
                return;
            }

            m_pipeline_state.clearDirty();

            if (pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
            {
                m_pipeline_state.setRenderPass(*m_current_render_pass.render_pass);
                auto& pipeline = getDevice().getResourceCache().requestGraphicsPipeline(m_pipeline_state);
                getHandle().bindPipeline(pipeline_bind_point, pipeline.getHandle());
            }
            else if (pipeline_bind_point == vk::PipelineBindPoint::eCompute)
            {
                auto& pipeline = getDevice().getResourceCache().requestComputePipeline(m_pipeline_state);
                getHandle().bindPipeline(pipeline_bind_point, pipeline.getHandle());
            }
            else
            {
                throw "Only graphics and compute pipeline bind points are supported now";
            }
        }

        void CommandBuffer::flushPushConstants()
        {
            if (m_stored_push_constants.empty())
            {
                return;
            }

            const PipelineLayoutCPP& pipeline_layout = m_pipeline_state.getPipelineLayout();
            vk::ShaderStageFlags shader_stage = pipeline_layout.getPushConstantRangeStage(common::toU32(m_stored_push_constants.size()));

            if (shader_stage)
            {
                getHandle().pushConstants<uint8_t>(pipeline_layout.getHandle(), shader_stage, 0, m_stored_push_constants);
            }
            else
            {
                LOGW("Push constant range [{}, {}] not found", 0, m_stored_push_constants.size());
            }

            m_stored_push_constants.clear();
        }

        const CommandBuffer::RenderPassBinding& CommandBuffer::getCurrentRenderPass() const
        {
            return m_current_render_pass;
        }

        const uint32_t CommandBuffer::getCurrentSubpassIndex() const
        {
            return m_pipeline_state.getSubpassIndex();
        }

        const bool CommandBuffer::isRenderSizeOptimal(const vk::Extent2D& framebuffer_extent, const vk::Rect2D& render_area)
        {
            auto render_area_granularity = m_current_render_pass.render_pass->getRenderAreaGranularity();

            return ((render_area.offset.x % render_area_granularity.width == 0) &&
                (render_area.offset.y % render_area_granularity.height == 0) &&
                ((render_area.extent.width % render_area_granularity.width == 0) ||
                    (render_area.offset.x + render_area.extent.width == framebuffer_extent.width)) &&
                ((render_area.extent.height % render_area_granularity.height == 0) ||
                    (render_area.offset.y + render_area.extent.height == framebuffer_extent.height)));
        }

    }
}