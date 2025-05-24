/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "core/spirv_reflection.h"
#include "utils/logger.h"

namespace frame {
    namespace core {
        namespace {
            template <ShaderResourceType T>
            inline void readShaderResource(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                LOGE("Not implemented! Read shader resources of type.");
            }

            template <spv::Decoration T>
            inline void readResourceDecoration(const spirv_cross::Compiler& /*compiler*/,
                const spirv_cross::Resource& /*resource*/,
                ShaderResource& /*shader_resource*/,
                const ShaderVariant& /* variant */)
            {
                LOGE("Not implemented! Read resources decoration of type.");
            }

            template <>
            inline void readResourceDecoration<spv::DecorationLocation>(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                shader_resource.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            }

            template <>
            inline void readResourceDecoration<spv::DecorationDescriptorSet>(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                shader_resource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            }

            template <>
            inline void readResourceDecoration<spv::DecorationBinding>(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                shader_resource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            }

            template <>
            inline void readResourceDecoration<spv::DecorationInputAttachmentIndex>(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                shader_resource.input_attachment_index = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            }

            template <>
            inline void readResourceDecoration<spv::DecorationNonWritable>(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                shader_resource.qualifiers |= ShaderResourceQualifiers::NonWritable;
            }

            template <>
            inline void readResourceDecoration<spv::DecorationNonReadable>(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                shader_resource.qualifiers |= ShaderResourceQualifiers::NonReadble;
            }

            inline void readResourceVecSize(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                const auto& spirv_type = compiler.get_type_from_variable(resource.id);
                shader_resource.vec_size = spirv_type.vecsize;
                shader_resource.columns = spirv_type.columns;
            }

            inline void readResourceArraySize(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                const auto& spirv_type = compiler.get_type_from_variable(resource.id);
                shader_resource.array_size = spirv_type.array.size() ? spirv_type.array[0] : 1;
            }

            inline void readResourceSize(const spirv_cross::Compiler& compiler,
                const spirv_cross::Resource& resource,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                const auto& spirv_type = compiler.get_type_from_variable(resource.id);

                size_t array_size = 0;
                if (variant.getRuntimeArraySizes().count(resource.name) != 0) {
                    array_size = variant.getRuntimeArraySizes().at(resource.name);
                }

                shader_resource.size = common::toU32(compiler.get_declared_struct_size_runtime_array(spirv_type, array_size));
            }

            inline void readResourceSize(const spirv_cross::Compiler& compiler,
                const spirv_cross::SPIRConstant& constant,
                ShaderResource& shader_resource,
                const ShaderVariant& variant)
            {
                auto spirv_type = compiler.get_type(constant.constant_type);

                switch (spirv_type.basetype) {
                case spirv_cross::SPIRType::BaseType::Boolean:
                case spirv_cross::SPIRType::BaseType::Char:
                case spirv_cross::SPIRType::BaseType::Int:
                case spirv_cross::SPIRType::BaseType::UInt:
                case spirv_cross::SPIRType::BaseType::Float:
                    shader_resource.size = 4;
                    break;
                case spirv_cross::SPIRType::BaseType::Int64:
                case spirv_cross::SPIRType::BaseType::UInt64:
                case spirv_cross::SPIRType::BaseType::Double:
                    shader_resource.size = 8;
                    break;
                default:
                    shader_resource.size = 0;
                    break;
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::Input>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto input_resources = compiler.get_shader_resources().stage_inputs;

                for (auto& resource : input_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::Input;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceVecSize(compiler, resource, shader_resource, variant);
                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationLocation>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::InputAttachment>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits /*stage*/,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto subpass_resources = compiler.get_shader_resources().subpass_inputs;

                for (auto& resource : subpass_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::InputAttachment;
                    shader_resource.stages = vk::ShaderStageFlagBits::eFragment;
                    shader_resource.name = resource.name;

                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationInputAttachmentIndex>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::Output>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto output_resources = compiler.get_shader_resources().stage_outputs;

                for (auto& resource : output_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::Output;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceVecSize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationLocation>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::ImageCPP>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto image_resources = compiler.get_shader_resources().separate_images;

                for (auto& resource : image_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::ImageCPP;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::ImageCPPSampler>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto image_resources = compiler.get_shader_resources().sampled_images;

                for (auto& resource : image_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::ImageCPPSampler;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::ImageCPPStorage>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto storage_resources = compiler.get_shader_resources().storage_images;

                for (auto& resource : storage_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::ImageCPPStorage;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationNonReadable>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationNonWritable>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::Sampler>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto sampler_resources = compiler.get_shader_resources().separate_samplers;

                for (auto& resource : sampler_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::Sampler;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::BufferUniform>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto uniform_resources = compiler.get_shader_resources().uniform_buffers;

                for (auto& resource : uniform_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::BufferUniform;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceSize(compiler, resource, shader_resource, variant);
                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }

            template <>
            inline void readShaderResource<ShaderResourceType::BufferStorage>(const spirv_cross::Compiler& compiler,
                vk::ShaderStageFlagBits stage,
                std::vector<ShaderResource>& resources,
                const ShaderVariant& variant)
            {
                auto storage_resources = compiler.get_shader_resources().storage_buffers;

                for (auto& resource : storage_resources) {
                    ShaderResource shader_resource{};
                    shader_resource.type = ShaderResourceType::BufferStorage;
                    shader_resource.stages = stage;
                    shader_resource.name = resource.name;

                    readResourceSize(compiler, resource, shader_resource, variant);
                    readResourceArraySize(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationNonReadable>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationNonWritable>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationDescriptorSet>(compiler, resource, shader_resource, variant);
                    readResourceDecoration<spv::DecorationBinding>(compiler, resource, shader_resource, variant);

                    resources.push_back(shader_resource);
                }
            }
        }

        bool SPIRVReflection::reflectShaderResources(vk::ShaderStageFlagBits stage,
            const std::vector<uint32_t>& spirv,
            std::vector<ShaderResource>& resources,
            const ShaderVariant& variant)
        {
            spirv_cross::CompilerGLSL compiler{ spirv };

            auto opts = compiler.get_common_options();
            opts.enable_420pack_extension = true;

            compiler.set_common_options(opts);

            parseShaderResources(compiler, stage, resources, variant);
            parsePushConstants(compiler, stage, resources, variant);
            parseSpecializationConstants(compiler, stage, resources, variant);

            return true;
        }

        void SPIRVReflection::parseShaderResources(const spirv_cross::Compiler& compiler,
            vk::ShaderStageFlagBits stage,
            std::vector<ShaderResource>& resources,
            const ShaderVariant& variant)
        {
            readShaderResource<ShaderResourceType::Input>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::InputAttachment>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::Output>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::ImageCPP>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::ImageCPPSampler>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::ImageCPPStorage>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::Sampler>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::BufferUniform>(compiler, stage, resources, variant);
            readShaderResource<ShaderResourceType::BufferStorage>(compiler, stage, resources, variant);
        }

        void SPIRVReflection::parsePushConstants(const spirv_cross::Compiler& compiler,
            vk::ShaderStageFlagBits stage,
            std::vector<ShaderResource>& resources,
            const ShaderVariant& variant)
        {
            auto shader_resources = compiler.get_shader_resources();

            for (auto& resource : shader_resources.push_constant_buffers) {
                const auto& spirv_type = compiler.get_type_from_variable(resource.id);

                std::uint32_t offset = std::numeric_limits<std::uint32_t>::max();

                for (auto i = 0U; i < spirv_type.member_types.size(); ++i) {
                    auto mem_offset = compiler.get_member_decoration(spirv_type.self, i, spv::DecorationOffset);
                    offset = std::min(offset, mem_offset);
                }

                ShaderResource shader_resource{};
                shader_resource.type = ShaderResourceType::PushConstant;
                shader_resource.stages = stage;
                shader_resource.name = resource.name;
                shader_resource.offset = offset;

                readResourceSize(compiler, resource, shader_resource, variant);

                shader_resource.size -= shader_resource.offset;

                resources.push_back(shader_resource);
            }
        }

        void SPIRVReflection::parseSpecializationConstants(const spirv_cross::Compiler& compiler,
            vk::ShaderStageFlagBits stage,
            std::vector<ShaderResource>& resources,
            const ShaderVariant& variant)
        {
            auto specialization_constants = compiler.get_specialization_constants();

            for (auto& resource : specialization_constants) {
                auto& spirv_value = compiler.get_constant(resource.id);

                ShaderResource shader_resource{};
                shader_resource.type = ShaderResourceType::SpecializationConstant;
                shader_resource.stages = stage;
                shader_resource.name = compiler.get_name(resource.id);
                shader_resource.offset = 0;
                shader_resource.constant_id = resource.constant_id;

                readResourceSize(compiler, spirv_value, shader_resource, variant);

                resources.push_back(shader_resource);
            }
        }
    }
}