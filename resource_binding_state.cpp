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

#include "core/resource_binding_state.h"

namespace frame {
    namespace core {
        void ResourceSet::reset() {
            clearDirty();
            m_resource_bindings.clear();
        }

        bool ResourceSet::isDirty() const {
            return m_dirty;
        }

        void ResourceSet::clearDirty() {
            m_dirty = false;
        }

        void ResourceSet::clearDirty(uint32_t binding, uint32_t array_element) {
            m_resource_bindings[binding][array_element].m_dirty = false;
        }

        void ResourceSet::bindBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range,
            uint32_t binding, uint32_t array_element)
        {
            m_resource_bindings[binding][array_element].m_dirty = true;
            m_resource_bindings[binding][array_element].m_buffer = &buffer;
            m_resource_bindings[binding][array_element].m_offset = offset;
            m_resource_bindings[binding][array_element].m_range = range;

            m_dirty = true;
        }

        void ResourceSet::bindImage(const core::ImageViewCPP& image_view, const core::Sampler& sampler,
            uint32_t binding, uint32_t array_element)
        {
            m_resource_bindings[binding][array_element].m_dirty = true;
            m_resource_bindings[binding][array_element].m_image_view = &image_view;
            m_resource_bindings[binding][array_element].m_sampler = &sampler;

            m_dirty = true;
        }

        void ResourceSet::bindImage(const core::ImageViewCPP& image_view, uint32_t binding, uint32_t array_element) {
            m_resource_bindings[binding][array_element].m_dirty = true;
            m_resource_bindings[binding][array_element].m_image_view = &image_view;
            m_resource_bindings[binding][array_element].m_sampler = nullptr;

            m_dirty = true;
        }

        void ResourceSet::bindInput(const core::ImageViewCPP& image_view, uint32_t binding, uint32_t array_element) {
            m_resource_bindings[binding][array_element].m_dirty = true;
            m_resource_bindings[binding][array_element].m_image_view = &image_view;

            m_dirty = true;
        }

        const BindingMap<ResourceInfo>& ResourceSet::getResourceBindings() const {
            return m_resource_bindings;
        }

        void ResourceBindingState::reset() {
            clearDirty();
            m_resource_sets.clear();
        }

        bool ResourceBindingState::isDirty() {
            return m_dirty;
        }

        void ResourceBindingState::clearDirty() {
            m_dirty = false;
        }

        void ResourceBindingState::clearDirty(uint32_t set) {
            m_resource_sets[set].clearDirty();
        }

        void ResourceBindingState::bindBuffer(const common::Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range,
            uint32_t set, uint32_t binding, uint32_t array_element)
        {
            m_resource_sets[set].bindBuffer(buffer, offset, range, binding, array_element);
            m_dirty = true;
        }

        void ResourceBindingState::bindImage(const core::ImageViewCPP& image_view, const core::Sampler& sampler,
            uint32_t set, uint32_t binding, uint32_t array_element)
        {
            m_resource_sets[set].bindImage(image_view, sampler, binding, array_element);
            m_dirty = true;
        }

        void ResourceBindingState::bindImage(const core::ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element) {
            m_resource_sets[set].bindImage(image_view, binding, array_element);
            m_dirty = true;
        }

        void ResourceBindingState::bindInput(const core::ImageViewCPP& image_view, uint32_t set, uint32_t binding, uint32_t array_element) {
            m_resource_sets[set].bindInput(image_view, binding, array_element);
            m_dirty = true;
        }

        const std::unordered_map<uint32_t, ResourceSet>& ResourceBindingState::getResourceSets() {
            return m_resource_sets;
        }
    }
}
