/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "core/shader_module.h"
#include "core/device.h"
#include "common/glsl_compiler.h"
#include "core/spirv_reflection.h"
#include "filesystem/filesystem.h"
#include "utils/logger.h"

#include <filesystem>
#include <functional>
#include <set>

namespace frame {
    namespace core {
        namespace {
            std::vector<std::string> split(const std::string& input, char delim) {
                std::vector<std::string> tokens;

                std::stringstream sstream(input);
                std::string       token;
                while (std::getline(sstream, token, delim)) {
                    tokens.push_back(token);
                }

                return tokens;
            }
            /*
            std::vector<std::string> precompileShader(const std::string& source) {

                std::vector<std::string> final_file;

                auto lines = split(source, '\n');

                for (auto& line : lines) {
                    if(line.find("#include \"") == 0) {
                        std::string include_path = line.substr(10);
                        size_t last_quote = include_path.find("\"");
                        if(!include_path.empty() && last_quote != std::string::npos)
                        {
                            include_path = include_path.substr(0, last_quote);
                        }

                        auto include_file = precompileShader(frame::filesystem::readShader(include_path));
                        for (auto& include_file_line : include_file)
                        {
                            final_file.push_back(include_file_line);
                        }
                    }
                    else
                    {
                        final_file.push_back(line);
                    }
                }

                return final_file;
            }

            std::vector<std::string> precompileShader(const std::string& source,
                frame::common::SimpleIncluder& includer,
                const std::string& base_dir = "")
            {
                std::vector<std::string> final_file;
                auto lines = split(source, '\n');

                for (auto& line : lines) {
                    if(line.find("#include \"") == 0) {
                        std::string include_path = line.substr(10);
                        size_t last_quote = include_path.find('\"');
                        if(last_quote != std::string::npos) {
                            include_path = include_path.substr(0, last_quote);
                        }

                        auto include_result = includer.includeLocal(
                            include_path.c_str(),
                            base_dir.empty() ? nullptr : base_dir.c_str(),
                            0
                        );

                        if(!include_result) {
                            throw std::runtime_error("[ShaderModuleCPP] ERROR: Failed to include file: " + include_path);
                        }

                        std::string included_content(static_cast<char*>(include_result->userData));
                        auto included_lines = precompileShader(
                            included_content,
                            includer,
                            std::filesystem::path(base_dir).parent_path().string()
                        );

                        final_file.insert(final_file.end(), included_lines.begin(), included_lines.end());

                        includer.releaseInclude(include_result);
                    }
                    else {
                        final_file.push_back(line);
                    }
                }

                return final_file;
            }

            std::vector<uint8_t> convertToBytes(std::vector<std::string>& lines)
            {
                std::vector<uint8_t> bytes;

                for (auto& line : lines)
                {
                    line += "\n";
                    std::vector<uint8_t> line_bytes(line.begin(), line.end());
                    bytes.insert(bytes.end(), line_bytes.begin(), line_bytes.end());
                }

                return bytes;
            }
            */
        }

        ShaderVariant::ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes) :
            m_preamble{ std::move(preamble) },
            m_processes{ std::move(processes) }
        {
            updateId();
        }

        size_t ShaderVariant::getId() const { return m_id; }

        void ShaderVariant::addDefinitions(const std::vector<std::string>& definitions) {
            for (auto& definition : definitions) {
                addDefine(definition);
            }
        }

        void ShaderVariant::addDefine(const std::string& def) {
            m_processes.push_back("D" + def);

            std::string tmp_def = def;

            size_t pos_equal = tmp_def.find_first_of("=");
            if(pos_equal != std::string::npos) {
                tmp_def[pos_equal] = ' ';
            }

            m_preamble.append("#define " + tmp_def + "\n");

            updateId();
        }

        void ShaderVariant::addUndefine(const std::string& undef) {
            m_processes.push_back("U" + undef);
            m_preamble.append("#undef " + undef + "\n");
            updateId();
        }

        void ShaderVariant::addRuntimeArraySize(const std::string& runtime_array_name, size_t size) {
            if(m_runtime_array_sizes.find(runtime_array_name) == m_runtime_array_sizes.end()) {
                m_runtime_array_sizes.insert({ runtime_array_name, size });
            }
            else {
                m_runtime_array_sizes[runtime_array_name] = size;
            }
        }

        void ShaderVariant::setRuntimeArraySizes(const std::unordered_map<std::string, size_t>& sizes) {
            m_runtime_array_sizes = sizes;
        }

        const std::string& ShaderVariant::getPreamble() const {
            return m_preamble;
        }

        const std::vector<std::string>& ShaderVariant::getProcesses() const {
            return m_processes;
        }

        const std::unordered_map<std::string, size_t>& ShaderVariant::getRuntimeArraySizes() const {
            return m_runtime_array_sizes;
        }

        void ShaderVariant::clear() {
            m_preamble.clear();
            m_processes.clear();
            m_runtime_array_sizes.clear();
            updateId();
        }

        void ShaderVariant::updateId() {
            m_id = std::hash<std::string>{}(m_preamble);
        }
        
        ShaderSource::ShaderSource(const std::string& filepath) :
            m_filename{ std::filesystem::path(filepath).filename().string() },
    		m_filepath{ GLSL_SHADER_DIR + filepath },
            m_source{ filesystem::readShader(filepath) }
        {
            m_id = std::hash<std::string>{}(std::string{ m_source.cbegin(), m_source.cend() });
            m_stage = common::findShaderStage(filepath);
        }

        size_t ShaderSource::getId() const {
            return m_id;
        }

        const std::string& ShaderSource::getFilename() const {
            return m_filename;
        }

        const std::string& ShaderSource::getFilepath() const {
            return m_filepath;
        }

        const vk::ShaderStageFlagBits ShaderSource::getStage() const
        {
            return m_stage;
        }

        const std::string& ShaderSource::getSource() const {
            return m_source;
        }

        void ShaderSource::setSource(const std::string& source) {
            m_source = source;
            m_id = std::hash<std::string>{}(std::string{ m_source.cbegin(), m_source.cend() });
        }

        ShaderModuleCPP::ShaderModuleCPP(
            Device& device, 
            vk::ShaderStageFlagBits stage, 
            const ShaderSource& glsl_source,
            const std::string& entry_point, 
            const ShaderVariant& shader_variant) :
    	VulkanResource{VK_NULL_HANDLE, &device },
    	m_stage{ stage },
    	m_entry_point{ entry_point }
        {
            m_debug_name = fmt::format("{} [variant {:X}] [entrypoint {}]",
                glsl_source.getFilename(), shader_variant.getId(), entry_point);
            
            if(entry_point.empty()) {
                throw std::runtime_error("[ShaderModuleCPP] ERROR: Shader entry point is empty");
            }

            auto& source = glsl_source.getSource();
            
            if(source.empty()) {
                throw std::runtime_error("[ShaderModuleCPP] ERROR: GLSL source code is empty");
            }

            common::GLSLCompiler glsl_compiler;

            glsl_compiler.addIncludePath(std::filesystem::path(glsl_source.getFilepath()).parent_path().string());

            if(!glsl_compiler.compileToSpirv(source, m_spirv, stage, entry_point, shader_variant, m_info_log)) {
                LOGE("Shader compilation failed for shader \"{}\"", glsl_source.getFilename());
                LOGE("{}", m_info_log);
            	throw std::runtime_error("[ShaderModuleCPP] ERROR: Shader compile fail");
            }

            SPIRVReflection spirv_reflection;
            
            if(!spirv_reflection.reflectShaderResources(stage, m_spirv, m_resources, shader_variant)) {
                throw std::runtime_error("[ShaderModuleCPP] ERROR: Shader reflect fail");
            }
            
            m_id = std::hash<std::string>{}(std::string{ reinterpret_cast<const char*>(m_spirv.data()), reinterpret_cast<const char*>(m_spirv.data() + m_spirv.size()) });

            vk::ShaderModuleCreateInfo create_info{
                {},
                m_spirv.size() * sizeof(uint32_t),
                m_spirv.data(),
                nullptr
            };
            setHandle(getDevice().getHandle().createShaderModule(create_info));
        }

        ShaderModuleCPP::ShaderModuleCPP(ShaderModuleCPP&& other) :
            VulkanResource{ std::move(other) },
            m_id{ other.m_id },
            m_stage{ other.m_stage },
            m_entry_point{ other.m_entry_point },
            m_debug_name{ other.m_debug_name },
            m_spirv{ other.m_spirv },
            m_resources{ other.m_resources },
            m_info_log{ other.m_info_log }
        {
            other.m_stage = {};
        }

        size_t ShaderModuleCPP::getId() const { return m_id; }

        vk::ShaderStageFlagBits ShaderModuleCPP::getStage() const { return m_stage; }

        const std::string& ShaderModuleCPP::getEntryPoint() const { return m_entry_point; }

        const std::vector<ShaderResource>& ShaderModuleCPP::getResources() const { return m_resources; }

        const std::string& ShaderModuleCPP::getInfoLog() const { return m_info_log; }

        const std::vector<uint32_t>& ShaderModuleCPP::getBinary() const { return m_spirv; }

        void ShaderModuleCPP::setResourceMode(const std::string& resource_name, const ShaderResourceMode& resource_mode) {
            auto it = std::find_if(m_resources.begin(), m_resources.end(),
                [&resource_name](const ShaderResource& resource) { return resource.name == resource_name; });

            if(it != m_resources.end()) {
                if(resource_mode == ShaderResourceMode::Dynamic) {
                    if(it->type == ShaderResourceType::BufferUniform || it->type == ShaderResourceType::BufferStorage) {
                        it->mode = resource_mode;
                    }
                    else {
                        LOGW("Resource `{}` does not support dynamic.", resource_name);
                    }
                }
                else {
                    it->mode = resource_mode;
                }
            }
            else {
                LOGW("Resource `{}` not found for shader.", resource_name);
            }
        }
    }
}