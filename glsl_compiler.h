/* Copyright (c) 2025, Aster Cylix Wang (@Cy1ix)
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

#include <set>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include "core/shader_module.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/Public/ResourceLimits.h"

namespace frame {
    namespace core {
        class ShaderSource;
    }
    namespace common {
        namespace {
            class SimpleIncluder : public glslang::TShader::Includer {
            public:
                std::vector<std::string> includePaths = {};

                IncludeResult* includeLocal(
                    const char* headerName,
                    const char* includerName,
                    size_t inclusionDepth) override
                {
                    namespace fs = std::filesystem;

                    if (includerName && includerName[0] != '\0') {
                        std::filesystem::path basePath = std::filesystem::path(includerName).parent_path();
                        if (auto result = loadInclude(basePath / headerName)) {
                            return result;
                        }
                    }

                    for (const auto& path : includePaths) {
                        if (auto result = loadInclude(std::filesystem::path(path) / headerName)) {
                            return result;
                        }
                    }

                    return nullptr;
                }

                void releaseInclude(IncludeResult* result) override {
                    if (result) {
                        delete[] static_cast<char*>(result->userData);
                        delete result;
                    }
                }

            private:
                IncludeResult* loadInclude(const std::filesystem::path& fullPath) {
                    std::ifstream file(fullPath, std::ios::ate | std::ios::binary);
                    if (!file.is_open()) return nullptr;

                    const size_t fileSize = static_cast<size_t>(file.tellg());
                    file.seekg(0);

                    char* content = new char[fileSize + 1];
                    file.read(content, fileSize);
                    content[fileSize] = '\0';

                    return new IncludeResult(
                        fullPath.string(),
                        content,
                        fileSize,
                        content
                    );
                }
            };
        }

        class GLSLCompiler {
        public:
            struct CompileOptions {
                EShLanguage shaderType = EShLangVertex;
                int vulkanVersion = 460;
                bool optimize = false;
                bool debugInfo = true;
            };

            GLSLCompiler() {
                static bool glslangInitialized = []() {
                    glslang::InitializeProcess();
                    return true;
                    }();
            }

            ~GLSLCompiler() {
                //glslang::FinalizeProcess();
            }

            void addIncludePath(const std::string& path) {
                m_includer.includePaths.push_back(path);
            }

            bool compileToSpirv(const std::string& shader_source,
                std::vector<uint32_t>& spirv,
                vk::ShaderStageFlagBits shader_stage,
                const std::string& entryPoint,
                const core::ShaderVariant& shader_variant,
                std::string& info_log)
            {
                spirv.clear();
                info_log.clear();

                CompileOptions options;

                options.shaderType = getShaderType(shader_stage);
                m_shader_stage = shader_stage;

                glslang::TShader shader(options.shaderType);
                glslang::TProgram program;

                const char* codePtr = shader_source.c_str();
                shader.setStrings(&codePtr, 1);
                shader.setEntryPoint(entryPoint.c_str());
                shader.setEnvInput(glslang::EShSourceGlsl, options.shaderType, glslang::EShClientVulkan, options.vulkanVersion);
                shader.setEnvClient(glslang::EShClientVulkan, getVulkanTargetEnv(options.vulkanVersion));
                shader.setEnvTarget(glslang::EShTargetSpv, getSpvTargetVersion(options.vulkanVersion));
                shader.setPreamble(shader_variant.getPreamble().c_str());
                shader.addProcesses(shader_variant.getProcesses());

                if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault, m_includer)) {
                    info_log = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
                    return false;
                }

                program.addShader(&shader);

                if (!program.link(EShMsgDefault)) {
                    info_log += program.getInfoLog();
                    return false;
                }

                glslang::SpvOptions spv_options;
                spv_options.generateDebugInfo = options.debugInfo;
                spv_options.disableOptimizer = !options.optimize;
                spv_options.optimizeSize = options.optimize;

                if (auto intermediate = program.getIntermediate(options.shaderType)) {
                    glslang::GlslangToSpv(*intermediate, spirv, &spv_options);
                    return true;
                }

                return false;
            }

            /*
            bool compile(const core::ShaderSource& shader_source,
                std::vector<uint32_t>& spirv,
                vk::ShaderStageFlagBits shader_stage = vk::ShaderStageFlagBits::eVertex,
                const std::string& entryPoint = "main")
            {
                spirv.clear();
                m_error_message.clear();

                CompileOptions options;

                options.shaderType = getShaderType(shader_stage);
                m_shader_stage = shader_stage;

                glslang::TShader shader(options.shaderType);
                glslang::TProgram program;

                const char* codePtr = shader_source.getSource().c_str();
                shader.setStrings(&codePtr, 1);
                shader.setEntryPoint(entryPoint.c_str());
                shader.setEnvInput(glslang::EShSourceGlsl, options.shaderType, glslang::EShClientVulkan, options.vulkanVersion);
                shader.setEnvClient(glslang::EShClientVulkan, getVulkanTargetEnv(options.vulkanVersion));
                shader.setEnvTarget(glslang::EShTargetSpv, getSpvTargetVersion(options.vulkanVersion));
                if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault, m_includer)) {
                    m_error_message = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
                    return false;
                }

                program.addShader(&shader);

                if (!program.link(EShMsgDefault)) {
                    m_error_message = program.getInfoLog();
                    return false;
                }

                glslang::SpvOptions spvOptions;
                spvOptions.generateDebugInfo = options.debugInfo;
                spvOptions.disableOptimizer = !options.optimize;
                spvOptions.optimizeSize = options.optimize;

                if (auto intermediate = program.getIntermediate(options.shaderType)) {
                    glslang::GlslangToSpv(*intermediate, spirv, &spvOptions);
                    return true;
                }

                m_error_message = "[GLSLCompiler] ERROR: Failed to generate intermediate code";
                return false;
            }
            */

            const SimpleIncluder& getIncluder() const { return m_includer; }

        private:
            EShLanguage getShaderType(const std::string& file_name) {

                std::string file_type;

                size_t pos = file_name.find_last_of('.');

                if (pos != std::string::npos && pos != 0) {
                    file_type = file_name.substr(pos + 1);
                }
                else {
                    throw std::runtime_error("[ShaderCompile] ERROR: Wrong shader file name!");
                }

                if (file_type == "vert") {
                    //m_shader_stage = VK_SHADER_STAGE_VERTEX_BIT;
                    return EShLangVertex;
                }
                else if (file_type == "frag") {
                    //m_shader_stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                    return EShLangFragment;
                }
                else if (file_type == "comp") {
                    //m_shader_stage = VK_SHADER_STAGE_COMPUTE_BIT;
                    return EShLangCompute;
                }
                else if (file_type == "geom") {
                    //m_shader_stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                    return EShLangGeometry;
                }
                else if (file_type == "mesh") {
                    //m_shader_stage = VK_SHADER_STAGE_MESH_BIT_EXT;
                    return EShLangMesh;
                }
                else if (file_type == "rahit") {
                    //m_shader_stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                    return EShLangAnyHit;
                }
                else if (file_type == "rcall") {
                    //m_shader_stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR;
                    return EShLangCallable;
                }
                else if (file_type == "rchit") {
                    //m_shader_stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                    return EShLangClosestHit;
                }
                else if (file_type == "rgen") {
                    //m_shader_stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                    return EShLangRayGen;
                }
                else if (file_type == "rint") {
                    //m_shader_stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                    return EShLangIntersect;
                }
                else if (file_type == "rmiss") {
                    //m_shader_stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                    return EShLangMiss;
                }
                else if (file_type == "task") {
                    //m_shader_stage = VK_SHADER_STAGE_TASK_BIT_EXT;
                    return EShLangTask;
                }
                else if (file_type == "tesc") {
                    //m_shader_stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                    return EShLangTessControl;
                }
                else if (file_type == "tese") {
                    //m_shader_stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                    return EShLangTessEvaluation;
                }
                else {
                    throw std::runtime_error("[ShaderCompile] ERROR: Wrong shader file type name!");
                }
            }

            EShLanguage getShaderType(VkShaderStageFlagBits stage) {
                switch (stage) {
                case VK_SHADER_STAGE_VERTEX_BIT: return EShLangVertex;
                case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return EShLangTessControl;
                case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return EShLangTessEvaluation;
                case VK_SHADER_STAGE_GEOMETRY_BIT: return EShLangGeometry;
                case VK_SHADER_STAGE_FRAGMENT_BIT: return EShLangFragment;
                case VK_SHADER_STAGE_COMPUTE_BIT: return EShLangCompute;
                case VK_SHADER_STAGE_RAYGEN_BIT_KHR: return EShLangRayGen;
                case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: return EShLangAnyHit;
                case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return EShLangClosestHit;
                case VK_SHADER_STAGE_MISS_BIT_KHR: return EShLangMiss;
                case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: return EShLangIntersect;
                case VK_SHADER_STAGE_CALLABLE_BIT_KHR: return EShLangCallable;
                case VK_SHADER_STAGE_MESH_BIT_EXT: return EShLangMesh;
                case VK_SHADER_STAGE_TASK_BIT_EXT: return EShLangTask;
                default: return EShLangVertex;
                }
            }

            EShLanguage getShaderType(vk::ShaderStageFlagBits stage) {
                switch (stage) {
                case vk::ShaderStageFlagBits::eVertex: return EShLangVertex;
                case vk::ShaderStageFlagBits::eTessellationControl: return EShLangTessControl;
                case vk::ShaderStageFlagBits::eTessellationEvaluation: return EShLangTessEvaluation;
                case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
                case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
                case vk::ShaderStageFlagBits::eCompute: return EShLangCompute;
                case vk::ShaderStageFlagBits::eRaygenKHR: return EShLangRayGen;
                case vk::ShaderStageFlagBits::eAnyHitKHR: return EShLangAnyHit;
                case vk::ShaderStageFlagBits::eClosestHitKHR: return EShLangClosestHit;
                case vk::ShaderStageFlagBits::eMissKHR: return EShLangMiss;
                case vk::ShaderStageFlagBits::eIntersectionKHR: return EShLangIntersect;
                case vk::ShaderStageFlagBits::eCallableKHR: return EShLangCallable;
                case vk::ShaderStageFlagBits::eMeshEXT: return EShLangMesh;
                case vk::ShaderStageFlagBits::eTaskEXT: return EShLangTask;
                default: return EShLangVertex;
                }
            }

            static glslang::EShTargetClientVersion getVulkanTargetEnv(int version) {
                switch (version) {
                case 460: return glslang::EShTargetVulkan_1_2;
                case 450: return glslang::EShTargetVulkan_1_1;
                case 440: return glslang::EShTargetVulkan_1_1;
                default:  return glslang::EShTargetVulkan_1_0;
                }
            }

            static glslang::EShTargetLanguageVersion getSpvTargetVersion(int version) {
                switch (version) {
                case 460: return glslang::EShTargetSpv_1_5;
                case 450: return glslang::EShTargetSpv_1_3;
                case 440: return glslang::EShTargetSpv_1_0;
                default:  return glslang::EShTargetSpv_1_0;
                }
            }

            SimpleIncluder m_includer;
            vk::ShaderStageFlagBits m_shader_stage;
        };

        class GLSLPrecompiler {
        public:
            explicit GLSLPrecompiler(std::filesystem::path base_dir = std::filesystem::current_path())
                : m_base_dir(std::move(base_dir)) {
            }

            std::string preCompile(const std::string& source) {
                std::set<std::filesystem::path> included_files;
                return processShaderSource(m_base_dir, source, included_files);
            }

        private:
            std::string processShaderSource(const std::filesystem::path& current_dir,
                const std::string& source,
                std::set<std::filesystem::path>& included_files)
            {
                std::ostringstream output;
                std::istringstream stream(source);
                std::string line;
                size_t line_num = 0;

                while (std::getline(stream, line)) {
                    ++line_num;

                    if (line.find("#include") == 0) {
                        handleInclude(current_dir, line, included_files, output);
                    }
                    else {
                        output << line << "\n";
                    }
                }

                return output.str();
            }

            void handleInclude(const std::filesystem::path& current_dir,
                const std::string& line,
                std::set<std::filesystem::path>& included_files,
                std::ostringstream& output)
            {
                size_t start_quote = line.find('"', 8);
                size_t end_quote = line.find('"', start_quote + 1);

                if (start_quote == std::string::npos || end_quote == std::string::npos) {
                    throw std::runtime_error("[GLSLCompiler] ERROR: Malformed #include directive: " + line);
                }

                std::filesystem::path include_rel(line.substr(start_quote + 1, end_quote - start_quote - 1));
                std::filesystem::path include_abs = std::filesystem::absolute(current_dir.parent_path() / include_rel);

                if (!std::filesystem::exists(include_abs)) {
                    throw std::runtime_error("[GLSLCompiler] ERROR: Shader include not found: " + include_abs.string());
                }

                if (included_files.count(include_abs)) {
                    return;
                }
                included_files.insert(include_abs);

                std::string included_source = readFile(include_abs);
                output << "// BEGIN INCLUDE: " << include_abs.filename() << "\n"
                    << processShaderSource(include_abs, included_source, included_files)
                    << "// END INCLUDE: " << include_abs.filename() << "\n";
            }

            static std::string readFile(const std::filesystem::path& path) {
                std::ifstream file(path);
                if (!file.is_open()) {
                    throw std::runtime_error("[GLSLCompiler] ERROR: Failed to open shader file: " + path.string());
                }
                return std::string((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
            }

            std::filesystem::path m_base_dir;
        };

    }
}