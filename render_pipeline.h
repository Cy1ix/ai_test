/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "common/helper.h"
#include "scene/utils.h"
#include "common/buffer.h"
#include "rendering/render_frame.h"
#include "rendering/subpass.h"
#include "rendering/subpass/forward_subpass.h"

namespace frame {
    namespace rendering {
        class RenderPipeline {
        public:
            RenderPipeline(std::vector<std::unique_ptr<Subpass>>&& subpasses = {});

            RenderPipeline(const RenderPipeline&) = delete;
            RenderPipeline(RenderPipeline&&) = default;

            virtual ~RenderPipeline() = default;

            RenderPipeline& operator=(const RenderPipeline&) = delete;
            RenderPipeline& operator=(RenderPipeline&&) = default;

            void prepare();

            const std::vector<LoadStoreInfo>& getLoadStore() const;
            void setLoadStore(const std::vector<LoadStoreInfo>& load_store);
            const std::vector<vk::ClearValue>& getClearValue() const;
            void setClearValue(const std::vector<vk::ClearValue>& clear_values);
            void addSubpass(std::unique_ptr<Subpass>&& subpass);
            void addForwardSubpass(std::unique_ptr<subpass::ForwardSubpass>&& subpass);
            std::vector<std::unique_ptr<Subpass>>& getSubpasses();

            void draw(core::CommandBuffer& command_buffer, RenderTarget& render_target,
                      vk::SubpassContents contents = vk::SubpassContents::eInline);
            
            std::unique_ptr<Subpass>& getActiveSubpass();

        private:
            std::vector<std::unique_ptr<Subpass>> m_subpasses;
            std::vector<LoadStoreInfo> m_load_store = std::vector<LoadStoreInfo>(2);
            std::vector<vk::ClearValue> m_clear_value = std::vector<vk::ClearValue>(2);
            size_t m_active_subpass_index{ 0 };
        };
    }
}
