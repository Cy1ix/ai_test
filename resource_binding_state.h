/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "common/buffer.h"
#include "core/image_view.h"
#include "common/common.h"

namespace frame {
	namespace common {
		class Buffer;
	}
    namespace core {
        class ImageViewCPP;
        class Sampler;
        
        struct ResourceInfo {
            bool m_dirty{ false };
            const common::Buffer* m_buffer{ nullptr };
            vk::DeviceSize m_offset{ 0 };
            vk::DeviceSize m_range{ 0 };
            const ImageViewCPP* m_image_view{ nullptr };
            const Sampler* m_sampler{ nullptr };
        };

        class ResourceSet {
        public:
            void reset();
            bool isDirty() const;
            void clearDirty();
            void clearDirty(uint32_t binding, uint32_t array_element);

            void bindBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range,
                uint32_t binding, uint32_t array_element);

            void bindImage(const ImageViewCPP& image_view, const Sampler& sampler,
                uint32_t binding, uint32_t array_element);

            void bindImage(const ImageViewCPP& image_view, uint32_t binding, uint32_t array_element);

            void bindInput(const ImageViewCPP& image_view, uint32_t binding, uint32_t array_element);

            const BindingMap<ResourceInfo>& getResourceBindings() const;

        private:
            bool m_dirty{ false };
            BindingMap<ResourceInfo> m_resource_bindings;
        };

        class ResourceBindingState {
        public:
            void reset();
            bool isDirty();
            void clearDirty();
            void clearDirty(uint32_t set);

            void bindBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range,
                uint32_t set, uint32_t binding, uint32_t array_element);

            void bindImage(const ImageViewCPP& image_view, const Sampler& sampler,
                uint32_t set, uint32_t binding, uint32_t array_element);

            void bindImage(const ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element);

            void bindInput(const ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element);

            const std::unordered_map<uint32_t, ResourceSet>& getResourceSets();

        private:
            bool m_dirty{ false };
            std::unordered_map<uint32_t, ResourceSet> m_resource_sets;
        };
    }
}
