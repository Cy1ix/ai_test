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

#include <imgui/imgui.h>

#include "buffer_pool.h"
#include "core/command_buffer.h"
#include "core/image_view.h"
#include "core/pipeline_layout.h"
#include "platform/imgui_drawer.h"
#include "filesystem/filesystem.h"
#include "platform/input.h"
#include "stats/stats.h"
#include "common/debug_info.h"

namespace frame {
	namespace platform {
		class Window;
	}

	class VulkanSample;

	namespace gui {
		struct Font {
			Font(const std::string& name, float size) :
				name{ name },
				data{ filesystem::readAsset("fonts/" + name + ".ttf") },
				size{ size }
			{
				ImFontConfig font_config{};
				font_config.FontDataOwnedByAtlas = false;

				if (size < 1.0f) {
					size = 20.0f;
				}

				ImGuiIO& io = ImGui::GetIO();
				handle = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
			}

			std::vector<uint8_t> data;
			ImFont* handle = nullptr;
			std::string name;
			float size = 0.0f;
		};

		class Gui {
		public:
			class StatsView {
			public:
				StatsView(const stats::Stats* stats);
				void resetMaxValues();
				void resetMaxValue(stats::StatIndex index);

			public:
				std::map<stats::StatIndex, stats::StatGraphData> m_graph_map;
				float m_graph_height = 50.0f;
				float m_top_padding = 1.1f;
			};

		public:
			static const std::string m_default_font;
			static bool m_visible;

		public:
			Gui(VulkanSample& sample,
				const platform::Window& window,
				const stats::Stats* stats = nullptr,
				float font_size = 21.0f,
				bool explicit_update = false);

			~Gui();

			void prepare(vk::PipelineCache pipeline_cache,
				vk::RenderPass render_pass,
				const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages);
			void resize(uint32_t width, uint32_t height) const;
			void newFrame() const;
			void update(float delta_time);
			bool updateBuffers();
			void draw(core::CommandBuffer& command_buffer);
			void draw(vk::CommandBuffer command_buffer) const;
			void showTopWindow(const std::string& app_name, const stats::Stats* stats = nullptr, const common::DebugInfo* debug_info = nullptr);
			void showDemoWindow() const;
			void showAppInfo(const std::string& app_name) const;
			void showDebugWindow(const common::DebugInfo& debug_info, const ImVec2& position);
			void showStats(const stats::Stats& stats);
			void showOptionsWindow(std::function<void()> body, const uint32_t lines = 3) const;
			void showSimpleWindow(const std::string& name, uint32_t last_fps, std::function<void()> body) const;
			bool inputEvent(const platform::InputEvent& input_event);
			const StatsView& getStatsView() const;
			platform::ImguiDrawer& getDrawer();
			Font const& getFont(const std::string& font_name = Gui::m_default_font) const;
			bool isDebugViewActive() const;

		private:
			common::BufferAllocation updateBuffers(core::CommandBuffer& command_buffer) const;

		private:
			struct DebugView {
				bool active = false;
				uint32_t max_fields = 8;
				float label_column_width = 0;
				float scale = 1.7f;
			};

			struct PushConstBlock {
				glm::vec2 scale;
				glm::vec2 translate;
			};

		private:
			static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

			static const double m_press_time_ms;
			static const float m_overlay_alpha;
			static const ImGuiWindowFlags m_common_flags;
			static const ImGuiWindowFlags m_options_flags;
			static const ImGuiWindowFlags m_info_flags;

		private:
			PushConstBlock m_push_const_block;
			VulkanSample& m_sample;
			std::unique_ptr<common::Buffer> m_vertex_buffer;
			std::unique_ptr<common::Buffer> m_index_buffer;
			size_t m_last_vertex_buffer_size = 0;
			size_t m_last_index_buffer_size = 0;
			float m_content_scale_factor = 1.0f;
			float m_dpi_factor = 1.0f;
			bool m_explicit_update = false;
			platform::ImguiDrawer m_drawer;
			std::vector<Font> m_fonts;
			std::unique_ptr<core::ImageCPP> m_font_image;
			std::unique_ptr<core::ImageViewCPP> m_font_image_view;
			std::unique_ptr<core::Sampler> m_sampler;
			core::PipelineLayoutCPP* m_pipeline_layout = nullptr;
			StatsView m_stats_view;
			DebugView m_debug_view;
			vk::DescriptorPool m_descriptor_pool = nullptr;
			vk::DescriptorSetLayout m_descriptor_set_layout = nullptr;
			vk::DescriptorSet m_descriptor_set = nullptr;
			vk::Pipeline m_pipeline = nullptr;
			Timer m_timer;
			bool m_prev_visible = true;
			bool m_two_finger_tap = false;
			bool m_show_graph_file_output = false;
			uint32_t m_subpass = 0;
		};
	}
}
