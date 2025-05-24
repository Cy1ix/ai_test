/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "core/resource_record.h"

namespace frame{
    namespace core {
        class ResourceCache;

        class ResourceReplay {
        public:
            ResourceReplay();
            void play(ResourceCache& resource_cache, ResourceRecord& recorder);
        protected:
            void createShaderModule(ResourceCache& resource_cache, std::istringstream& stream);
            void createPipelineLayout(ResourceCache& resource_cache, std::istringstream& stream);
            void createRenderPass(ResourceCache& resource_cache, std::istringstream& stream);
            void createGraphicsPipeline(ResourceCache& resource_cache, std::istringstream& stream);
        private:
            using ResourceFunc = std::function<void(ResourceCache&, std::istringstream&)>;
            std::unordered_map<ResourceType, ResourceFunc> m_stream_resources;
            std::vector<ShaderModuleCPP*> m_shader_modules;
            std::vector<PipelineLayoutCPP*> m_pipeline_layouts;
            std::vector<const RenderPassCPP*> m_render_passes;
            std::vector<const GraphicsPipelineCPP*> m_graphics_pipelines;
        };
    }
}
