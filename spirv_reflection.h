/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include <string>
#include <unordered_map>
#include <vector>

#include "spirv_cross/spirv_glsl.hpp"

#include "common/common.h"
#include "core/shader_module.h"

namespace frame {
    namespace core {
        class SPIRVReflection {
        public:
            bool reflectShaderResources(vk::ShaderStageFlagBits stage,
                const std::vector<uint32_t>& spirv,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant);

        private:
            void parseShaderResources(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant);

            void parsePushConstants(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant);

            void parseSpecializationConstants(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant);
        };
    }
}