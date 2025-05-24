/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include <vulkan/vulkan.hpp>
#include "vulkan_resource.h"
#include "common/helper.h"
#include "common/common.h"

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#	undef None
#endif

namespace frame {
    namespace core {
        class Device;

        enum class ShaderResourceType {
            Input,
            InputAttachment,
            Output,
            ImageCPP,
            ImageCPPSampler,
            ImageCPPStorage,
            Sampler,
            BufferUniform,
            BufferStorage,
            PushConstant,
            SpecializationConstant,
            All
        };

        enum class ShaderResourceMode {
            Static,
            Dynamic,
            UpdateAfterBind
        };

        struct ShaderResourceQualifiers {
            enum : uint32_t {
                None = 0,
                NonReadble = 1,
                NonWritable = 2,
            };
        };

        struct ShaderResource {
            vk::ShaderStageFlags stages;
            ShaderResourceType type;
            ShaderResourceMode mode;
            uint32_t set;
            uint32_t binding;
            uint32_t location;
            uint32_t input_attachment_index;
            uint32_t vec_size;
            uint32_t columns;
            uint32_t array_size;
            uint32_t offset;
            uint32_t size;
            uint32_t constant_id;
            uint32_t qualifiers;
            std::string name;
        };

        class ShaderVariant {
        public:
            ShaderVariant() = default;
            ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes);

            size_t getId() const;

            void addDefinitions(const std::vector<std::string>& definitions);
            void addDefine(const std::string& def);
            void addUndefine(const std::string& undef);
            void addRuntimeArraySize(const std::string& runtime_array_name, size_t size);
            void setRuntimeArraySizes(const std::unordered_map<std::string, size_t>& sizes);

            const std::string& getPreamble() const;
            const std::vector<std::string>& getProcesses() const;
            const std::unordered_map<std::string, size_t>& getRuntimeArraySizes() const;

            void clear();

        private:
            size_t m_id;
            std::string m_preamble;
            std::vector<std::string> m_processes;
            std::unordered_map<std::string, size_t> m_runtime_array_sizes;

            void updateId();
        };

        class ShaderSource {
        public:
            ShaderSource() = default;
            ShaderSource(const std::string& filepath);

            size_t getId() const;
        	const std::string& getFilename() const;
            const std::string& getFilepath() const;
            const std::string& getSource() const;
            const vk::ShaderStageFlagBits getStage() const;
            void setSource(const std::string& source);

        private:
            size_t m_id;
            std::string m_filename;
            std::string m_filepath;
            std::string m_source;
            vk::ShaderStageFlagBits m_stage;
        };

        class ShaderModuleCPP : public VulkanResource<vk::ShaderModule> {
        public:
            ShaderModuleCPP(Device& device,
                vk::ShaderStageFlagBits stage,
                const ShaderSource& glsl_source,
                const std::string& entry_point = "main",
                const ShaderVariant& shader_variant = {});

            ShaderModuleCPP(const ShaderModuleCPP&) = delete;
            ShaderModuleCPP(ShaderModuleCPP&& other);

            ShaderModuleCPP& operator=(const ShaderModuleCPP&) = delete;
            ShaderModuleCPP& operator=(ShaderModuleCPP&&) = delete;

            size_t getId() const;
            vk::ShaderStageFlagBits getStage() const;
            const std::string& getEntryPoint() const;
            const std::vector<ShaderResource>& getResources() const;
            const std::string& getInfoLog() const;
            const std::vector<uint32_t>& getBinary() const;

            const std::string& getDebugName() const { return m_debug_name; }
            void setDebugName(const std::string& name) { m_debug_name = name; }

            void setResourceMode(const std::string& resource_name, const ShaderResourceMode& resource_mode);

        private:
            size_t m_id;
            vk::ShaderStageFlagBits m_stage{};
            std::string m_entry_point;
            std::string m_debug_name;
            std::vector<uint32_t> m_spirv;
            std::vector<ShaderResource> m_resources;
            std::string m_info_log;
        };
    }
}
