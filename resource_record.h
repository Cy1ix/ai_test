/* Copyright (c) 2019-2020, Arm Limited and Contributors
 * Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include <vector>
#include "core/pipeline.h"
#include "rendering/pipeline_state.h"

namespace frame {
    namespace common {
        struct LoadStoreInfo;
    }

    namespace rendering {
        struct Attachment;
        class PipelineState;
    }

    enum class ResourceType : uint32_t {
        ShaderModuleCPP,
        PipelineLayoutCPP,
        RenderPassCPP,
        GraphicsPipelineCPP
    };

    namespace core {
        class GraphicsPipelineCPP;
        class PipelineLayoutCPP;
        class RenderPassCPP;
        class ShaderModuleCPP;
        class ShaderSource;
        class ShaderVariant;
        struct SubpassInfo;

        class ResourceRecord {
        public:
            void setData(const std::vector<uint8_t>& data);
            std::vector<uint8_t> getData();
            const std::ostringstream& getStream();

            size_t registerShaderModule(vk::ShaderStageFlagBits stage,
                const core::ShaderSource& glsl_source,
                const std::string& entry_point,
                const core::ShaderVariant& shader_variant);

            size_t registerPipelineLayout(const std::vector<core::ShaderModuleCPP*>& shader_modules);

            size_t registerRenderPass(const std::vector<rendering::Attachment>& attachments,
                const std::vector<rendering::LoadStoreInfo>& load_store_infos,
                const std::vector<core::SubpassInfo>& subpasses);

            size_t registerGraphicsPipeline(vk::PipelineCache pipeline_cache,
                rendering::PipelineState& pipeline_state);

            void setShaderModule(size_t index, const core::ShaderModuleCPP& shader_module);
            void setPipelineLayout(size_t index, const core::PipelineLayoutCPP& pipeline_layout);
            void setRenderPass(size_t index, const core::RenderPassCPP& render_pass);
            void setGraphicsPipeline(size_t index, const core::GraphicsPipelineCPP& graphics_pipeline);

        private:
            std::ostringstream m_stream;
            std::vector<size_t> m_shader_module_indices;
            std::vector<size_t> m_pipeline_layout_indices;
            std::vector<size_t> m_render_pass_indices;
            std::vector<size_t> m_graphics_pipeline_indices;
            std::unordered_map<const core::ShaderModuleCPP*, size_t> m_shader_module_to_index;
            std::unordered_map<const core::PipelineLayoutCPP*, size_t> m_pipeline_layout_to_index;
            std::unordered_map<const core::RenderPassCPP*, size_t> m_render_pass_to_index;
            std::unordered_map<const core::GraphicsPipelineCPP*, size_t> m_graphics_pipeline_to_index;
        };
    }
}
