/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "core/shader_module.h"
#include "rendering/subpass.h"
#include "rendering/render_context.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"

namespace frame {
	namespace rendering {

		glm::mat4 vulkanStyleProjection(const glm::mat4& proj) {
			glm::mat4 mat = proj;
			mat[1][1] *= -1;

			return mat;
		}

		Subpass::Subpass(RenderContext& render_context, core::ShaderSource&& vertex_source, core::ShaderSource&& fragment_source) :
			m_fragment_shader{ std::move(fragment_source) },
			m_render_context{ render_context },
			m_vertex_shader{ std::move(vertex_source) }
		{
		}

		const std::vector<uint32_t>& Subpass::getInputAttachments() const {
			return m_input_attachments;
		}

		LightingState& Subpass::getLightingState() {
			return m_lighting_state;
		}

		const std::vector<uint32_t>& Subpass::getOutputAttachments() const {
			return m_output_attachments;
		}

		RenderContext& Subpass::getRenderContext() {
			return m_render_context;
		}

		std::unordered_map<std::string, core::ShaderResourceMode> const& Subpass::getResourceModeMap() const {
			return m_resource_mode_map;
		}

		vk::SampleCountFlagBits Subpass::getSampleCount() const {
			return m_sample_count;
		}

		const core::ShaderSource& Subpass::getVertexShader() const {
			return m_vertex_shader;
		}

		const std::vector<uint32_t>& Subpass::getColorResolveAttachments() const {
			return m_color_resolve_attachments;
		}

		const std::string& Subpass::getDebugName() const {
			return m_debug_name;
		}

		const uint32_t& Subpass::getDepthStencilResolveAttachment() const {
			return m_depth_stencil_resolve_attachment;
		}

		vk::ResolveModeFlagBits Subpass::getDepthStencilResolveMode() const {
			return m_depth_stencil_resolve_mode;
		}

		DepthStencilState& Subpass::getDepthStencilState() {
			return m_depth_stencil_state;
		}

		const bool& Subpass::getDisableDepthStencilAttachment() const {
			return m_disable_depth_stencil_attachment;
		}

		const core::ShaderSource& Subpass::getFragmentShader() const {
			return m_fragment_shader;
		}

		void Subpass::setColorResolveAttachments(std::vector<uint32_t> const& color_resolve) {
			m_color_resolve_attachments = color_resolve;
		}

		void Subpass::setDepthStencilResolveAttachment(uint32_t depth_stencil_resolve) {
			m_depth_stencil_resolve_attachment = depth_stencil_resolve;
		}

		void Subpass::setDebugName(const std::string& name) {
			m_debug_name = name;
		}

		void Subpass::setDisableDepthStencilAttachment(bool disable_depth_stencil) {
			m_disable_depth_stencil_attachment = disable_depth_stencil;
		}

		void Subpass::setDepthStencilResolveMode(vk::ResolveModeFlagBits mode) {
			m_depth_stencil_resolve_mode = mode;
		}

		void Subpass::setInputAttachments(std::vector<uint32_t> const& input) {
			m_input_attachments = input;
		}

		void Subpass::setOutputAttachments(std::vector<uint32_t> const& output) {
			m_output_attachments = output;
		}

		void Subpass::setSampleCount(vk::SampleCountFlagBits sample_count) {
			m_sample_count = sample_count;
		}

		void Subpass::updateRenderTargetAttachments(RenderTarget& render_target) {
			render_target.setInputAttachments(m_input_attachments);
			render_target.setOutputAttachments(m_output_attachments);
		}
	}
}
