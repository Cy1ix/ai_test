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

#include "common/gui.h"
#include "vulkan_sample.h"
#include "scene/utils.h"
#include "core/command_pool.h"
#include "core/queue.h"
#include "core/sampler.h"
#include <imgui/imgui_internal.h>

#include <numeric>

namespace frame {
	namespace gui {
		namespace {
			void uploadDrawData(const ImDrawData* draw_data, uint8_t* vertex_data, uint8_t* index_data) {
				ImDrawVert* vtx_dst = reinterpret_cast<ImDrawVert*>(vertex_data);
				ImDrawIdx* idx_dst = reinterpret_cast<ImDrawIdx*>(index_data);

				for (int n = 0; n < draw_data->CmdListsCount; n++)
				{
					const ImDrawList* cmd_list = draw_data->CmdLists[n];
					memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
					memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
					vtx_dst += cmd_list->VtxBuffer.Size;
					idx_dst += cmd_list->IdxBuffer.Size;
				}
			}

			void resetGraphMaxValue(stats::StatGraphData& graph_data) {
				if (!graph_data.m_has_fixed_max) {
					graph_data.m_max_value = 0.0f;
				}
			}

			ImGuiKey mapKeyCodeToImGuiKey(platform::KeyCode key_code) {
				switch (key_code) {

				case platform::KeyCode::Unknown: return ImGuiKey_None;
				case platform::KeyCode::Space: return ImGuiKey_Space;
				case platform::KeyCode::Apostrophe: return ImGuiKey_Apostrophe;
				case platform::KeyCode::Comma: return ImGuiKey_Comma;
				case platform::KeyCode::Minus: return ImGuiKey_Minus;
				case platform::KeyCode::Period: return ImGuiKey_Period;
				case platform::KeyCode::Slash: return ImGuiKey_Slash;
					
				case platform::KeyCode::_0: return ImGuiKey_0;
				case platform::KeyCode::_1: return ImGuiKey_1;
				case platform::KeyCode::_2: return ImGuiKey_2;
				case platform::KeyCode::_3: return ImGuiKey_3;
				case platform::KeyCode::_4: return ImGuiKey_4;
				case platform::KeyCode::_5: return ImGuiKey_5;
				case platform::KeyCode::_6: return ImGuiKey_6;
				case platform::KeyCode::_7: return ImGuiKey_7;
				case platform::KeyCode::_8: return ImGuiKey_8;
				case platform::KeyCode::_9: return ImGuiKey_9;
					
				case platform::KeyCode::Semicolon: return ImGuiKey_Semicolon;
				case platform::KeyCode::Equal: return ImGuiKey_Equal;
				case platform::KeyCode::LeftBracket: return ImGuiKey_LeftBracket;
				case platform::KeyCode::Backslash: return ImGuiKey_Backslash;
				case platform::KeyCode::RightBracket: return ImGuiKey_RightBracket;
				case platform::KeyCode::GraveAccent: return ImGuiKey_GraveAccent;
					
				case platform::KeyCode::A: return ImGuiKey_A;
				case platform::KeyCode::B: return ImGuiKey_B;
				case platform::KeyCode::C: return ImGuiKey_C;
				case platform::KeyCode::D: return ImGuiKey_D;
				case platform::KeyCode::E: return ImGuiKey_E;
				case platform::KeyCode::F: return ImGuiKey_F;
				case platform::KeyCode::G: return ImGuiKey_G;
				case platform::KeyCode::H: return ImGuiKey_H;
				case platform::KeyCode::I: return ImGuiKey_I;
				case platform::KeyCode::J: return ImGuiKey_J;
				case platform::KeyCode::K: return ImGuiKey_K;
				case platform::KeyCode::L: return ImGuiKey_L;
				case platform::KeyCode::M: return ImGuiKey_M;
				case platform::KeyCode::N: return ImGuiKey_N;
				case platform::KeyCode::O: return ImGuiKey_O;
				case platform::KeyCode::P: return ImGuiKey_P;
				case platform::KeyCode::Q: return ImGuiKey_Q;
				case platform::KeyCode::R: return ImGuiKey_R;
				case platform::KeyCode::S: return ImGuiKey_S;
				case platform::KeyCode::T: return ImGuiKey_T;
				case platform::KeyCode::U: return ImGuiKey_U;
				case platform::KeyCode::V: return ImGuiKey_V;
				case platform::KeyCode::W: return ImGuiKey_W;
				case platform::KeyCode::X: return ImGuiKey_X;
				case platform::KeyCode::Y: return ImGuiKey_Y;
				case platform::KeyCode::Z: return ImGuiKey_Z;
					
				case platform::KeyCode::Escape: return ImGuiKey_Escape;
				case platform::KeyCode::Enter: return ImGuiKey_Enter;
				case platform::KeyCode::Tab: return ImGuiKey_Tab;
				case platform::KeyCode::Backspace: return ImGuiKey_Backspace;
				case platform::KeyCode::Insert: return ImGuiKey_Insert;
				case platform::KeyCode::DelKey: return ImGuiKey_Delete;
				case platform::KeyCode::Right: return ImGuiKey_RightArrow;
				case platform::KeyCode::Left: return ImGuiKey_LeftArrow;
				case platform::KeyCode::Down: return ImGuiKey_DownArrow;
				case platform::KeyCode::Up: return ImGuiKey_UpArrow;
				case platform::KeyCode::PageUp: return ImGuiKey_PageUp;
				case platform::KeyCode::PageDown: return ImGuiKey_PageDown;
				case platform::KeyCode::Home: return ImGuiKey_Home;
				case platform::KeyCode::End: return ImGuiKey_End;
				case platform::KeyCode::Back: return ImGuiKey_Backspace;
				case platform::KeyCode::CapsLock: return ImGuiKey_CapsLock;
				case platform::KeyCode::ScrollLock: return ImGuiKey_ScrollLock;
				case platform::KeyCode::NumLock: return ImGuiKey_NumLock;
				case platform::KeyCode::PrintScreen: return ImGuiKey_PrintScreen;
				case platform::KeyCode::Pause: return ImGuiKey_Pause;
					
				case platform::KeyCode::F1: return ImGuiKey_F1;
				case platform::KeyCode::F2: return ImGuiKey_F2;
				case platform::KeyCode::F3: return ImGuiKey_F3;
				case platform::KeyCode::F4: return ImGuiKey_F4;
				case platform::KeyCode::F5: return ImGuiKey_F5;
				case platform::KeyCode::F6: return ImGuiKey_F6;
				case platform::KeyCode::F7: return ImGuiKey_F7;
				case platform::KeyCode::F8: return ImGuiKey_F8;
				case platform::KeyCode::F9: return ImGuiKey_F9;
				case platform::KeyCode::F10: return ImGuiKey_F10;
				case platform::KeyCode::F11: return ImGuiKey_F11;
				case platform::KeyCode::F12: return ImGuiKey_F12;
					
				case platform::KeyCode::KP_0: return ImGuiKey_Keypad0;
				case platform::KeyCode::KP_1: return ImGuiKey_Keypad1;
				case platform::KeyCode::KP_2: return ImGuiKey_Keypad2;
				case platform::KeyCode::KP_3: return ImGuiKey_Keypad3;
				case platform::KeyCode::KP_4: return ImGuiKey_Keypad4;
				case platform::KeyCode::KP_5: return ImGuiKey_Keypad5;
				case platform::KeyCode::KP_6: return ImGuiKey_Keypad6;
				case platform::KeyCode::KP_7: return ImGuiKey_Keypad7;
				case platform::KeyCode::KP_8: return ImGuiKey_Keypad8;
				case platform::KeyCode::KP_9: return ImGuiKey_Keypad9;
				case platform::KeyCode::KP_Decimal: return ImGuiKey_KeypadDecimal;
				case platform::KeyCode::KP_Divide: return ImGuiKey_KeypadDivide;
				case platform::KeyCode::KP_Multiply: return ImGuiKey_KeypadMultiply;
				case platform::KeyCode::KP_Subtract: return ImGuiKey_KeypadSubtract;
				case platform::KeyCode::KP_Add: return ImGuiKey_KeypadAdd;
				case platform::KeyCode::KP_Enter: return ImGuiKey_KeypadEnter;
				case platform::KeyCode::KP_Equal: return ImGuiKey_KeypadEqual;
					
				case platform::KeyCode::LeftShift: return ImGuiKey_LeftShift;
				case platform::KeyCode::LeftControl: return ImGuiKey_LeftCtrl;
				case platform::KeyCode::LeftAlt: return ImGuiKey_LeftAlt;
				case platform::KeyCode::RightShift: return ImGuiKey_RightShift;
				case platform::KeyCode::RightControl: return ImGuiKey_RightCtrl;
				case platform::KeyCode::RightAlt: return ImGuiKey_RightAlt;

				default: return ImGuiKey_None;
				}
			}
		}

		bool Gui::m_visible = true;
		const double Gui::m_press_time_ms = 200.0f;
		const float Gui::m_overlay_alpha = 0.3f;
		const std::string Gui::m_default_font = "Roboto-Regular";
		const ImGuiWindowFlags Gui::m_common_flags = ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing;
		const ImGuiWindowFlags Gui::m_options_flags = Gui::m_common_flags;
		const ImGuiWindowFlags Gui::m_info_flags = Gui::m_common_flags | ImGuiWindowFlags_NoInputs;

		Gui::Gui(VulkanSample& sample_, 
			const platform::Window& window, 
			const stats::Stats* stats, 
			float font_size, 
			bool explicit_update) :
		m_sample{ sample_ },
		m_content_scale_factor{ window.getContentScaleFactor() },
		m_dpi_factor{ window.getDpiFactor() * m_content_scale_factor },
		m_explicit_update{ explicit_update },
		m_stats_view(stats)
		{
			ImGui::CreateContext();

			ImGuiStyle& style = ImGui::GetStyle();
			
			style.Colors[ImGuiCol_WindowBg] = ImVec4(0.005f, 0.005f, 0.005f, 0.94f);
			style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
			style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
			style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
			style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
			style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			
			style.WindowBorderSize = 0.0f;
			
			style.ScaleAllSizes(m_dpi_factor);
			
			ImGuiIO& io = ImGui::GetIO();
			auto const& extent = m_sample.getRenderContext().getSurfaceExtent();
			io.DisplaySize.x = static_cast<float>(extent.width);
			io.DisplaySize.y = static_cast<float>(extent.height);
			io.FontGlobalScale = 1.0f;
			io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
			
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

			// No longer need to set up KeyMap[] array
			// The old code was:
			//io.KeyMap[ImGuiKey_Space] = static_cast<int>(platform::KeyCode::Space);
			//io.KeyMap[ImGuiKey_Enter] = static_cast<int>(platform::KeyCode::Enter);
			//io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(platform::KeyCode::Left);
			//io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(platform::KeyCode::Right);
			//io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(platform::KeyCode::Up);
			//io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(platform::KeyCode::Down);
			//io.KeyMap[ImGuiKey_Tab] = static_cast<int>(platform::KeyCode::Tab);
			//io.KeyMap[ImGuiKey_Escape] = static_cast<int>(platform::KeyCode::Backspace);

			// The mapping is no longer needed as ImGui internally handles this

			m_fonts.emplace_back(m_default_font, font_size * m_dpi_factor);
			
			m_fonts.emplace_back("RobotoMono-Regular", (font_size / 2) * m_dpi_factor);
			
			unsigned char* font_data;
			int tex_width, tex_height;
			io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
			size_t upload_size = tex_width * tex_height * 4 * sizeof(char);

			auto& device = m_sample.getRenderContext().getDevice();
			
			m_font_image =
				core::ImageCPPBuilder(common::toU32(tex_width), common::toU32(tex_height))
				.withFormat(vk::Format::eR8G8B8A8Unorm)
				.withUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
				.withDebugName("GUI font image")
				.buildUnique(device);

			m_font_image_view = std::make_unique<core::ImageViewCPP>(*m_font_image, vk::ImageViewType::e2D);
			m_font_image_view->setDebugName("View on GUI font image");
			
			{
				common::Buffer stage_buffer = common::Buffer::createStagingBuffer(device, upload_size, font_data);

				auto& command_buffer = device.getCommandPool().requestCommandBuffer();

				core::FencePool fence_pool(device);
				
				command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

				{
					common::ImageMemoryBarrier memory_barrier;
					memory_barrier.m_old_layout = vk::ImageLayout::eUndefined;
					memory_barrier.m_new_layout = vk::ImageLayout::eTransferDstOptimal;
					memory_barrier.m_src_access_mask = {};
					memory_barrier.m_dst_access_mask = vk::AccessFlagBits::eTransferWrite;
					memory_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eHost;
					memory_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;

					command_buffer.imageMemoryBarrier(*m_font_image_view, memory_barrier);
				}
				
				vk::BufferImageCopy buffer_copy_region;
				buffer_copy_region.imageSubresource.layerCount = m_font_image_view->getSubresourceRange().layerCount;
				buffer_copy_region.imageSubresource.aspectMask = m_font_image_view->getSubresourceRange().aspectMask;
				buffer_copy_region.imageExtent = m_font_image->getExtent();

				command_buffer.copyBufferToImage(stage_buffer, *m_font_image, { buffer_copy_region });

				{
					common::ImageMemoryBarrier memory_barrier{};
					memory_barrier.m_old_layout = vk::ImageLayout::eTransferDstOptimal;
					memory_barrier.m_new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
					memory_barrier.m_src_access_mask = vk::AccessFlagBits::eTransferWrite;
					memory_barrier.m_dst_access_mask = vk::AccessFlagBits::eShaderRead;
					memory_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eTransfer;
					memory_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eFragmentShader;

					command_buffer.imageMemoryBarrier(*m_font_image_view, memory_barrier);
				}
				
				command_buffer.end();

				auto& queue = device.getQueueByFlags(vk::QueueFlagBits::eGraphics, 0);

				queue.submit(command_buffer, device.getFencePool().requestFence());
				
				device.getFencePool().wait();
				device.getFencePool().reset();
				device.getCommandPool().resetPool();
			}

			core::ShaderSource vert_shader("imgui.vert");
			core::ShaderSource frag_shader("imgui.frag");

			std::vector<core::ShaderModuleCPP*> shader_modules;
			shader_modules.push_back(&device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eVertex, vert_shader, {}));
			shader_modules.push_back(&device.getResourceCache().requestShaderModule(vk::ShaderStageFlagBits::eFragment, frag_shader, {}));

			m_pipeline_layout = &device.getResourceCache().requestPipelineLayout(shader_modules);
			
			const vk::FormatProperties fmt_props = device.getPhysicalDevice().getHandle().getFormatProperties(m_font_image_view->getFormat());

			vk::Filter filter = (fmt_props.optimalTilingFeatures & 
				vk::FormatFeatureFlagBits::eSampledImageFilterLinear) ? vk::Filter::eLinear : vk::Filter::eNearest;
			
			vk::SamplerCreateInfo sampler_info;
			sampler_info.maxAnisotropy = 1.0f;
			sampler_info.magFilter = filter;
			sampler_info.minFilter = filter;
			sampler_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
			sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
			sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
			sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
			sampler_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;

			m_sampler = std::make_unique<core::Sampler>(device, sampler_info);
			m_sampler->setDebugName("GUI sampler");

			if (m_explicit_update) {
				auto& device = m_sample.getRenderContext().getDevice();

				m_vertex_buffer =
					common::BufferBuilder(1)
					.withUsage(vk::BufferUsageFlagBits::eVertexBuffer)
					.withVmaUsage(VMA_MEMORY_USAGE_GPU_TO_CPU)
					.withDebugName("GUI vertex buffer")
					.buildUnique(device);

				m_index_buffer =
					common::BufferBuilder(1)
					.withUsage(vk::BufferUsageFlagBits::eIndexBuffer)
					.withVmaUsage(VMA_MEMORY_USAGE_GPU_TO_CPU)
					.withDebugName("GUI index buffer")
					.buildUnique(device);
			}
		}

		void Gui::prepare(vk::PipelineCache pipeline_cache, vk::RenderPass render_pass, const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages) {

			vk::Device device = m_sample.getRenderContext().getDevice().getHandle();
			
			vk::DescriptorPoolSize pool_size(vk::DescriptorType::eCombinedImageSampler, 1);
			vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_size);
			m_descriptor_pool = device.createDescriptorPool(descriptor_pool_create_info);
			
			vk::DescriptorSetLayoutBinding    layout_binding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
			vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, layout_binding);
			m_descriptor_set_layout = device.createDescriptorSetLayout(descriptor_set_layout_create_info);
			
#if defined(ANDROID)
			vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(m_descriptor_pool, 1, &m_descriptor_set_layout);
#else
			vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(m_descriptor_pool, m_descriptor_set_layout);
#endif
			m_descriptor_set = device.allocateDescriptorSets(descriptor_set_allocate_info).front();

			vk::DescriptorImageInfo font_descriptor(m_sampler->getHandle(), m_font_image_view->getHandle(), vk::ImageLayout::eShaderReadOnlyOptimal);
			vk::WriteDescriptorSet  write_descriptor_set(m_descriptor_set, 0, 0, vk::DescriptorType::eCombinedImageSampler, font_descriptor);
			device.updateDescriptorSets(write_descriptor_set, {});
			
			vk::VertexInputBindingDescription vertex_input_binding(0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex);
			std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = { {{0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)},
																						   {1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)},
																						   {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)}} };
			vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

			vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::eTriangleList, false);

			vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

			vk::PipelineRasterizationStateCreateInfo rasterization_state(
				{}, false, {}, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, {}, {}, {}, {}, 1.0f);

			vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

			vk::PipelineDepthStencilStateCreateInfo depth_stencil_state({}, false, false, vk::CompareOp::eAlways, {}, {}, {}, { {}, {}, {}, vk::CompareOp::eAlways });
			
			vk::PipelineColorBlendAttachmentState blend_attachment_state(true,
				vk::BlendFactor::eSrcAlpha,
				vk::BlendFactor::eOneMinusSrcAlpha,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eOneMinusSrcAlpha,
				vk::BlendFactor::eZero,
				vk::BlendOp::eAdd,
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
			vk::PipelineColorBlendStateCreateInfo color_blend_state({}, {}, {}, blend_attachment_state);

			std::array<vk::DynamicState, 2>    dynamic_state_enables = { {vk::DynamicState::eViewport, vk::DynamicState::eScissor} };
			vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

			vk::GraphicsPipelineCreateInfo pipeline_create_info({},
				shader_stages,
				&vertex_input_state,
				&input_assembly_state,
				nullptr,
				&viewport_state,
				&rasterization_state,
				&multisample_state,
				&depth_stencil_state,
				&color_blend_state,
				&dynamic_state,
				m_pipeline_layout->getHandle(),
				render_pass,
				0,
				nullptr,
				-1);

			m_pipeline = device.createGraphicsPipeline(pipeline_cache, pipeline_create_info).value;
		}

		void Gui::update(const float delta_time) {
			if (m_visible != m_prev_visible) {
				m_drawer.setDirty(true);
				m_prev_visible = m_visible;
			}

			if (!m_visible) {
				ImGui::EndFrame();
				return;
			}
			
			ImGuiIO& io = ImGui::GetIO();
			auto extent = m_sample.getRenderContext().getSurfaceExtent();
			resize(extent.width, extent.height);
			io.DeltaTime = delta_time;
			
			ImGui::Render();
		}

		bool Gui::updateBuffers() {
			ImDrawData* draw_data = ImGui::GetDrawData();
			bool        updated = false;

			if (!draw_data) {
				return false;
			}

			size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
			size_t index_buffer_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

			if ((vertex_buffer_size == 0) || (index_buffer_size == 0)) {
				return false;
			}

			if ((!m_vertex_buffer->getHandle()) || (vertex_buffer_size != m_last_vertex_buffer_size)) {
				m_last_vertex_buffer_size = vertex_buffer_size;
				updated = true;

				m_vertex_buffer = std::make_unique<common::Buffer>(m_sample.getRenderContext().getDevice(), vertex_buffer_size,
					vk::BufferUsageFlagBits::eVertexBuffer,
					VMA_MEMORY_USAGE_GPU_TO_CPU);
				m_vertex_buffer->setDebugName("GUI vertex buffer");
			}

			if ((!m_index_buffer->getHandle()) || (index_buffer_size != m_last_index_buffer_size)) {
				m_last_index_buffer_size = index_buffer_size;
				updated = true;

				m_index_buffer = std::make_unique<common::Buffer>(m_sample.getRenderContext().getDevice(), index_buffer_size,
					vk::BufferUsageFlagBits::eIndexBuffer,
					VMA_MEMORY_USAGE_GPU_TO_CPU);
				m_index_buffer->setDebugName("GUI index buffer");
			}
			
			uploadDrawData(draw_data, m_vertex_buffer->map(), m_index_buffer->map());

			m_vertex_buffer->flush();
			m_index_buffer->flush();

			m_vertex_buffer->unmap();
			m_index_buffer->unmap();

			return updated;
		}

		common::BufferAllocation Gui::updateBuffers(core::CommandBuffer& command_buffer) const {
			ImDrawData* draw_data = ImGui::GetDrawData();
			rendering::RenderFrame& render_frame = m_sample.getRenderContext().getActiveFrame();

			if (!draw_data || (draw_data->TotalVtxCount == 0) || (draw_data->TotalIdxCount == 0)) {
				return common::BufferAllocation{};
			}

			size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
			size_t index_buffer_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

			std::vector<uint8_t> vertex_data(vertex_buffer_size);
			std::vector<uint8_t> index_data(index_buffer_size);

			uploadDrawData(draw_data, vertex_data.data(), index_data.data());

			auto vertex_allocation = render_frame.allocateBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertex_buffer_size);

			vertex_allocation.update(vertex_data);

			std::vector<std::reference_wrapper<const common::Buffer>> buffers;
			buffers.emplace_back(std::ref(vertex_allocation.getBuffer()));

			command_buffer.bindVertexBuffers(0, buffers, { vertex_allocation.getOffset() });

			auto index_allocation = render_frame.allocateBuffer(vk::BufferUsageFlagBits::eIndexBuffer, index_buffer_size);

			index_allocation.update(index_data);

			command_buffer.bindIndexBuffer(index_allocation.getBuffer(), index_allocation.getOffset(), vk::IndexType::eUint16);

			return vertex_allocation;
		}

		void Gui::resize(uint32_t width, uint32_t height) const {
			auto& io = ImGui::GetIO();
			io.DisplaySize.x = static_cast<float>(width);
			io.DisplaySize.y = static_cast<float>(height);
		}

		void Gui::newFrame() const {
			ImGui::NewFrame();
		}

		void Gui::draw(core::CommandBuffer& command_buffer) {
			if (!m_visible) {
				return;
			}

			core::ScopedDebugLabel debug_label(command_buffer, "GUI");
			
			vk::VertexInputBindingDescription vertex_input_binding({}, common::toU32(sizeof(ImDrawVert)));
			
			vk::VertexInputAttributeDescription pos_attr(0, 0, vk::Format::eR32G32Sfloat, common::toU32(offsetof(ImDrawVert, pos)));
			
			vk::VertexInputAttributeDescription uv_attr(1, 0, vk::Format::eR32G32Sfloat, common::toU32(offsetof(ImDrawVert, uv)));
			
			vk::VertexInputAttributeDescription col_attr(2, 0, vk::Format::eR8G8B8A8Unorm, common::toU32(offsetof(ImDrawVert, col)));

			rendering::VertexInputState vertex_input_state;
			vertex_input_state.bindings = { vertex_input_binding };
			vertex_input_state.attributes = { pos_attr, uv_attr, col_attr };

			command_buffer.setVertexInputState(vertex_input_state);
			
			rendering::ColorBlendAttachmentState color_attachment;
			color_attachment.blend_enable = true;
			color_attachment.color_write_mask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
			color_attachment.src_color_blend_factor = vk::BlendFactor::eSrcAlpha;
			color_attachment.dst_color_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;
			color_attachment.src_alpha_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;

			rendering::ColorBlendState blend_state{};
			blend_state.attachments = { color_attachment };

			command_buffer.setColorBlendState(blend_state);

			rendering::RasterizationState rasterization_state{};
			rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
			command_buffer.setRasterizationState(rasterization_state);

			rendering::DepthStencilState depth_state{};
			depth_state.depth_test_enable = false;
			depth_state.depth_write_enable = false;
			command_buffer.setDepthStencilState(depth_state);
			
			command_buffer.bindPipelineLayout(*m_pipeline_layout);

			command_buffer.bindImage(*m_font_image_view, *m_sampler, 0, 0, 0);
			
			auto& io = ImGui::GetIO();
			auto  push_transform = glm::mat4(1.0f);

			if (m_sample.getRenderContext().hasSwapchain()) {
				auto transform = m_sample.getRenderContext().getSwapchain().getTransform();

				glm::vec3 rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f);
				if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate90) {
					push_transform = glm::rotate(push_transform, glm::radians(90.0f), rotation_axis);
				}
				else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate270) {
					push_transform = glm::rotate(push_transform, glm::radians(270.0f), rotation_axis);
				}
				else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate180) {
					push_transform = glm::rotate(push_transform, glm::radians(180.0f), rotation_axis);
				}
			}
			
			push_transform = glm::translate(push_transform, glm::vec3(-1.0f, -1.0f, 0.0f));
			push_transform = glm::scale(push_transform, glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));
			
			command_buffer.pushConstants(push_transform);

			std::vector<std::reference_wrapper<const common::Buffer>> vertex_buffers;
			std::vector<vk::DeviceSize> vertex_offsets;
			
			if (!m_explicit_update) {
				auto vertex_allocation = updateBuffers(command_buffer);
				if (!vertex_allocation.isEmpty()) {
					vertex_buffers.push_back(vertex_allocation.getBuffer());
					vertex_offsets.push_back(vertex_allocation.getOffset());
				}
			}
			else {
				vertex_buffers.push_back(*m_vertex_buffer);
				vertex_offsets.push_back(0);
				command_buffer.bindVertexBuffers(0, vertex_buffers, vertex_offsets);

				command_buffer.bindIndexBuffer(*m_index_buffer, 0, vk::IndexType::eUint16);
			}
			
			ImDrawData* draw_data = ImGui::GetDrawData();
			int32_t vertex_offset = 0;
			uint32_t index_offset = 0;

			if (!draw_data || draw_data->CmdListsCount == 0) {
				return;
			}

			for (int32_t i = 0; i < draw_data->CmdListsCount; i++) {
				const ImDrawList* cmd_list = draw_data->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
					const ImDrawCmd* cmd = &cmd_list->CmdBuffer[j];
					vk::Rect2D       scissor_rect;
					scissor_rect.offset.x = std::max(static_cast<int32_t>(cmd->ClipRect.x), 0);
					scissor_rect.offset.y = std::max(static_cast<int32_t>(cmd->ClipRect.y), 0);
					scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
					
					if (m_sample.getRenderContext().hasSwapchain()) {
						auto transform = m_sample.getRenderContext().getSwapchain().getTransform();
						if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate90) {
							scissor_rect.offset.x = static_cast<uint32_t>(io.DisplaySize.y - cmd->ClipRect.w);
							scissor_rect.offset.y = static_cast<uint32_t>(cmd->ClipRect.x);
							scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
							scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
						}
						else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate180) {
							scissor_rect.offset.x = static_cast<uint32_t>(io.DisplaySize.x - cmd->ClipRect.z);
							scissor_rect.offset.y = static_cast<uint32_t>(io.DisplaySize.y - cmd->ClipRect.w);
							scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
							scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
						}
						else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate270) {
							scissor_rect.offset.x = static_cast<uint32_t>(cmd->ClipRect.y);
							scissor_rect.offset.y = static_cast<uint32_t>(io.DisplaySize.x - cmd->ClipRect.z);
							scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
							scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
						}
					}

					command_buffer.setScissor(0, { scissor_rect });
					command_buffer.drawIndexed(cmd->ElemCount, 1, index_offset, vertex_offset, 0);
					index_offset += cmd->ElemCount;
				}
#if	defined(PLATFORM__MACOS) && TARGET_OS_IOS && TARGET_OS_SIMULATOR
				// iOS Simulator does not support vkCmdDrawIndexed() with vertex_offset > 0, so rebind vertex buffer instead
				if (!vertex_offsets.empty()) {
					vertex_offsets.back() += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
					command_buffer.bind_vertex_buffers(0, vertex_buffers, vertex_offsets);
				}
#else
				vertex_offset += cmd_list->VtxBuffer.Size;
#endif
			}
		}

		void Gui::draw(vk::CommandBuffer command_buffer) const {
			if (!m_visible) {
				return;
			}

			ImDrawData* draw_data = ImGui::GetDrawData();

			if ((!draw_data) || (draw_data->CmdListsCount == 0)) {
				return;
			}

			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout->getHandle(), 0, m_descriptor_set, {});
			
			auto& io = ImGui::GetIO();
			glm::mat4 push_transform = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)), glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));
			command_buffer.pushConstants<glm::mat4>(m_pipeline_layout->getHandle(), vk::ShaderStageFlagBits::eVertex, 0, push_transform);

			vk::DeviceSize vertex_offsets[1] = { 0 };
			vk::Buffer     vertex_buffer_handle = m_vertex_buffer->getHandle();
			command_buffer.bindVertexBuffers(0, vertex_buffer_handle, vertex_offsets);

			command_buffer.bindIndexBuffer(m_index_buffer->getHandle(), 0, vk::IndexType::eUint16);

			int32_t vertex_offset = 0;
			int32_t index_offset = 0;
			for (int32_t i = 0; i < draw_data->CmdListsCount; i++) {

				const ImDrawList* cmd_list = draw_data->CmdLists[i];

				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {

					const ImDrawCmd* cmd = &cmd_list->CmdBuffer[j];
					vk::Rect2D       scissor_rect;
					scissor_rect.offset.x = std::max(static_cast<int32_t>(cmd->ClipRect.x), 0);
					scissor_rect.offset.y = std::max(static_cast<int32_t>(cmd->ClipRect.y), 0);
					scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);

					command_buffer.setScissor(0, scissor_rect);
					command_buffer.drawIndexed(cmd->ElemCount, 1, index_offset, vertex_offset, 0);
					index_offset += cmd->ElemCount;
				}
#if	defined(PLATFORM__MACOS) && TARGET_OS_IOS && TARGET_OS_SIMULATOR
				// iOS Simulator does not support vkCmdDrawIndexed() with vertex_offset > 0, so rebind vertex buffer instead
				vertex_offsets[0] += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
				command_buffer.bindVertexBuffers(0, vertex_buffer_handle, vertex_offsets);
#else
				vertex_offset += cmd_list->VtxBuffer.Size;
#endif
			}
		}

		Gui::~Gui() {
			vk::Device device = m_sample.getRenderContext().getDevice().getHandle();
			device.destroyDescriptorPool(m_descriptor_pool);
			device.destroyDescriptorSetLayout(m_descriptor_set_layout);
			device.destroyPipeline(m_pipeline);

			ImGui::DestroyContext();
		}

		void Gui::showDemoWindow() const {
			ImGui::ShowDemoWindow();
		}

		const Gui::StatsView& Gui::getStatsView() const {
			return m_stats_view;
		}

		platform::ImguiDrawer& Gui::getDrawer() {
			return m_drawer;
		}

		Font const& Gui::getFont(const std::string& font_name) const {
			assert(!m_fonts.empty() && "No fonts exist");

			auto it = std::find_if(m_fonts.begin(), m_fonts.end(), [&font_name](Font const& font) { return font.name == font_name; });

			if (it != m_fonts.end()) {
				return *it;
			}
			else {
				LOGW("Couldn't find font with name {}", font_name);
				return *m_fonts.begin();
			}
		}

		bool Gui::isDebugViewActive() const {
			return m_debug_view.active;
		}

		Gui::StatsView::StatsView(const stats::Stats* stats) {
			if (stats == nullptr) {
				return;
			}
			
			const std::set<stats::StatIndex>& indices = stats->getRequestedStats();

			for (stats::StatIndex i : indices) {
				m_graph_map[i] = stats->getGraphData(i);
			}
		}

		void Gui::StatsView::resetMaxValue(const stats::StatIndex index) {
			auto pr = m_graph_map.find(index);
			if (pr != m_graph_map.end()) {
				resetGraphMaxValue(pr->second);
			}
		}

		void Gui::StatsView::resetMaxValues() {
			std::for_each(m_graph_map.begin(), m_graph_map.end(), [](auto& pr) { resetGraphMaxValue(pr.second); });
		}

		void Gui::showTopWindow(const std::string& app_name, const stats::Stats* stats, const common::DebugInfo* debug_info) {

			ImGui::SetNextWindowBgAlpha(m_overlay_alpha);
			ImVec2 size{ ImGui::GetIO().DisplaySize.x, 0.0f };
			ImGui::SetNextWindowSize(size, ImGuiCond_Always);
			
			ImVec2 pos{ 0.0f, 0.0f };
			ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

			bool is_open = true;
			ImGui::Begin("Top", &is_open, m_common_flags);

			showAppInfo(app_name);

			if (stats) {
				showStats(*stats);
				
				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0 /* left */)) {
					m_stats_view.resetMaxValues();
				}
			}

			if (debug_info) {
				if (m_debug_view.active) {
					showDebugWindow(*debug_info, ImVec2{ 0, ImGui::GetWindowSize().y });
				}
			}

			ImGui::End();
		}

		void Gui::showAppInfo(const std::string& app_name) const {

			ImGui::Text("%s", app_name.c_str());
			
			auto& device = m_sample.getRenderContext().getDevice();
			auto  device_name_label = "GPU: " + std::string(device.getPhysicalDevice().getProperties().deviceName.data());
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(device_name_label.c_str()).x);
			ImGui::Text("%s", device_name_label.c_str());
		}

		void Gui::showDebugWindow(const common::DebugInfo& debug_info, const ImVec2& position) {
			auto& io = ImGui::GetIO();
			auto& style = ImGui::GetStyle();
			auto& font = getFont("RobotoMono-Regular");
			
			if (m_debug_view.label_column_width == 0) {
				m_debug_view.label_column_width = style.ItemInnerSpacing.x + debug_info.getLongestLabel() * font.size / m_debug_view.scale;
			}

			ImGui::SetNextWindowBgAlpha(m_overlay_alpha);
			ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowContentSize(ImVec2{ io.DisplaySize.x, 0.0f });

			bool is_open = true;
			const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoNav;

			ImGui::Begin("Debug Window", &is_open, flags);
			ImGui::PushFont(font.handle);

			auto field_count = debug_info.getFields().size() > m_debug_view.max_fields ? m_debug_view.max_fields : debug_info.getFields().size();

			ImGui::BeginChild("Table", ImVec2(0, field_count * (font.size + style.ItemSpacing.y)), false);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, m_debug_view.label_column_width);
			ImGui::SetColumnWidth(1, io.DisplaySize.x - m_debug_view.label_column_width);

			for (auto& field : debug_info.getFields()) {
				const std::string& label = field->m_label;
				const std::string& value = field->toString();
				ImGui::Text("%s", label.c_str());
				ImGui::NextColumn();
				ImGui::Text(" %s", value.c_str());
				ImGui::NextColumn();
			}
			ImGui::Columns(1);
			ImGui::EndChild();

			ImGui::PopFont();
			ImGui::End();
		}

		void Gui::showStats(const stats::Stats& stats) {
			for (const auto& stat_index : stats.getRequestedStats()) {
				auto pr = m_stats_view.m_graph_map.find(stat_index);

				assert(pr != m_stats_view.m_graph_map.end() && "StatIndex not implemented in gui graph_map");
				
				auto& graph_data = pr->second;
				const auto& graph_elements = stats.getData(stat_index);
				float graph_min = 0.0f;
				float& graph_max = graph_data.m_max_value;

				if (!graph_data.m_has_fixed_max) {
					auto new_max = *std::max_element(graph_elements.begin(), graph_elements.end()) * m_stats_view.m_top_padding;
					if (new_max > graph_max) {
						graph_max = new_max;
					}
				}

				const ImVec2 graph_size = ImVec2{ ImGui::GetIO().DisplaySize.x, m_stats_view.m_graph_height /* dpi */ * m_dpi_factor };

				std::stringstream graph_label;
				float avg = std::accumulate(graph_elements.begin(), graph_elements.end(), 0.0f) / graph_elements.size();
				
				if (stats.isAvailable(stat_index)) {
					graph_label << fmt::format(graph_data.m_name + ": " + graph_data.m_format, avg * graph_data.m_scale_factor);
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PlotLines("", &graph_elements[0], static_cast<int>(graph_elements.size()), 0, graph_label.str().c_str(), graph_min, graph_max, graph_size);
					ImGui::PopItemFlag();
				}
				else {
					graph_label << graph_data.m_name << ": not available";
					ImGui::Text("%s", graph_label.str().c_str());
				}
			}
		}

		void Gui::showOptionsWindow(std::function<void()> body, const uint32_t lines) const {

			const float window_padding = ImGui::CalcTextSize("T").x;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ window_padding, window_padding * 2.0f });
			auto window_height = lines * ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2.0f;
			auto window_width = ImGui::GetIO().DisplaySize.x;
			ImGui::SetNextWindowBgAlpha(m_overlay_alpha);
			const ImVec2 size = ImVec2(window_width, 0);
			ImGui::SetNextWindowSize(size, ImGuiCond_Always);
			const ImVec2 pos = ImVec2(0.0f, ImGui::GetIO().DisplaySize.y - window_height);
			ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
			const ImGuiWindowFlags flags = (ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_AlwaysUseWindowPadding |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoFocusOnAppearing);
			bool is_open = true;
			ImGui::Begin("Options", &is_open, flags);
			body();
			ImGui::End();
			ImGui::PopStyleVar();
		}

		void Gui::showSimpleWindow(const std::string& name, uint32_t last_fps, std::function<void()> body) const {
			ImGuiIO& io = ImGui::GetIO();

			ImGui::NewFrame();
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
			ImGui::SetNextWindowPos(ImVec2(10, 10));
			ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
			ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
			ImGui::TextUnformatted(name.c_str());
			ImGui::TextUnformatted(m_sample.getRenderContext().getDevice().getPhysicalDevice().getProperties().deviceName.data());
			ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / last_fps), last_fps);
			ImGui::PushItemWidth(110.0f * m_dpi_factor);

			body();

			ImGui::PopItemWidth();
			ImGui::End();
			ImGui::PopStyleVar();
		}

		bool Gui::inputEvent(const platform::InputEvent& input_event) {
			auto& io = ImGui::GetIO();
			auto  capture_move_event = false;

			if (input_event.getSource() == platform::EventSource::Keyboard) {

				const auto& key_event = static_cast<const platform::KeyInputEvent&>(input_event);
				
				ImGuiKey imgui_key = mapKeyCodeToImGuiKey(key_event.getCode());

				switch (key_event.getAction()) {
				case platform::KeyAction::Down:
				case platform::KeyAction::Repeat:
					io.AddKeyEvent(imgui_key, true);
					break;
				case platform::KeyAction::Up:
					io.AddKeyEvent(imgui_key, false);
					break;
				case platform::KeyAction::Unknown:
					break;
				}
			}
			else if (input_event.getSource() == platform::EventSource::Mouse) {
				const auto& mouse_button = static_cast<const platform::MouseButtonInputEvent&>(input_event);

				io.MousePos = ImVec2{ mouse_button.getPosX() * m_content_scale_factor,
									 mouse_button.getPosY() * m_content_scale_factor };

				auto button_id = static_cast<int>(mouse_button.getButton());

				if (mouse_button.getAction() == platform::MouseAction::Down) {
					io.MouseDown[button_id] = true;
				}
				else if (mouse_button.getAction() == platform::MouseAction::Up) {
					io.MouseDown[button_id] = false;
				}
				else if (mouse_button.getAction() == platform::MouseAction::Move) {
					capture_move_event = io.WantCaptureMouse;
				}
			}
			else if (input_event.getSource() == platform::EventSource::Touchscreen) {
				const auto& touch_event = static_cast<const platform::TouchInputEvent&>(input_event);

				io.MousePos = ImVec2{ touch_event.getPosX(), touch_event.getPosY() };

				if (touch_event.getAction() == platform::TouchAction::Down) {
					io.MouseDown[touch_event.getPointerId()] = true;
				}
				else if (touch_event.getAction() == platform::TouchAction::Up) {
					io.MouseDown[touch_event.getPointerId()] = false;
				}
				else if (touch_event.getAction() == platform::TouchAction::Move) {
					capture_move_event = io.WantCaptureMouse;
				}
			}
			
			if (!io.WantCaptureMouse) {
				bool press_down = (input_event.getSource() == platform::EventSource::Mouse && static_cast<const platform::MouseButtonInputEvent&>(input_event).getAction() == platform::MouseAction::Down) || 
					(input_event.getSource() == platform::EventSource::Touchscreen && static_cast<const platform::TouchInputEvent&>(input_event).getAction() == platform::TouchAction::Down);
				bool press_up = (input_event.getSource() == platform::EventSource::Mouse && static_cast<const platform::MouseButtonInputEvent&>(input_event).getAction() == platform::MouseAction::Up) || 
					(input_event.getSource() == platform::EventSource::Touchscreen && static_cast<const platform::TouchInputEvent&>(input_event).getAction() == platform::TouchAction::Up);

				if (press_down) {
					m_timer.start();
					if (input_event.getSource() == platform::EventSource::Touchscreen) {
						const auto& touch_event = static_cast<const platform::TouchInputEvent&>(input_event);
						if (touch_event.getTouchPoints() == 2) {
							m_two_finger_tap = true;
						}
					}
				}
				if (press_up) {
					auto press_delta = m_timer.stop<Timer::Milliseconds>();
					if (press_delta < m_press_time_ms) {
						if (input_event.getSource() == platform::EventSource::Mouse) {
							const auto& mouse_button = static_cast<const platform::MouseButtonInputEvent&>(input_event);
							if (mouse_button.getButton() == platform::MouseButton::Right) {
								m_debug_view.active = !m_debug_view.active;
							}
						}
						else if (input_event.getSource() == platform::EventSource::Touchscreen) {
							const auto& touch_event = static_cast<const platform::TouchInputEvent&>(input_event);
							if (m_two_finger_tap && touch_event.getTouchPoints() == 2) {
								m_debug_view.active = !m_debug_view.active;
							}
							else {
								m_two_finger_tap = false;
							}
						}
					}
				}
			}

			return capture_move_event;
		}
	}
}
