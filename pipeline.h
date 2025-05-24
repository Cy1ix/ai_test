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

#include "rendering/pipeline_state.h"

namespace frame {
    namespace rendering {
        class PipelineState;
    }

    namespace core {
        class Device;

        class PipelineCPP : public VulkanResource<vk::Pipeline> {
        public:
            PipelineCPP(Device& device);

            PipelineCPP(const PipelineCPP&) = delete;

            PipelineCPP(PipelineCPP&& other);

            virtual ~PipelineCPP();

            PipelineCPP& operator=(const PipelineCPP&) = delete;
            PipelineCPP& operator=(PipelineCPP&&) = delete;
            
            const rendering::PipelineState& getState() const;

        protected:
            rendering::PipelineState m_state;
        };
        
        class ComputePipelineCPP : public PipelineCPP {
        public:
            ComputePipelineCPP(ComputePipelineCPP&&) = default;
            virtual ~ComputePipelineCPP() = default;

            ComputePipelineCPP(Device& device, vk::PipelineCache pipeline_cache, rendering::PipelineState& pipeline_state);
        };

        class GraphicsPipelineCPP : public PipelineCPP {
        public:
            GraphicsPipelineCPP(GraphicsPipelineCPP&&) = default;
            virtual ~GraphicsPipelineCPP() = default;

            GraphicsPipelineCPP(Device& device, vk::PipelineCache pipeline_cache, rendering::PipelineState& pipeline_state);
        };
    }
}
