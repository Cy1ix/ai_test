/* Copyright (c) 2018-2024, Arm Limited and Contributors
 * Copyright (c) 2019-2024, Sascha Willems
 * Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#define TINYGLTF_IMPLEMENTATION
#include "common/gltf_loader.h"

#include <limits>
#include <queue>
#include <set>

#include "global_common.h"
#include <glm/gtc/type_ptr.hpp>

#include "common/profiling.h"
#include "utils/logger.h"

#include "core/physical_device.h"
#include "core/device.h"
#include "core/image.h"
#include "core/queue.h"
#include "core/fence_pool.h"
#include "common/buffer.h"
#include "filesystem/filesystem.h"

#include "scene/components/camera/camera.h"
#include "scene/components/image/image.h"
#include "scene/components/image/astc.h"
#include "scene/components/light.h"
#include "scene/components/mesh/mesh.h"
#include "scene/components/material/pbr_material.h"
#include "scene/components/camera/perspective_camera.h"
#include "scene/components/sampler.h"
#include "scene/components/mesh/sub_mesh.h"
#include "scene/components/texture.h"
#include "scene/components/transform.h"
#include "scene/node.h"
#include "scene/scene.h"
#include "scene/scripts/animation.h"

#include <BS_thread_pool.hpp>

#include "utils.h"

namespace frame {
    namespace common {
        namespace {
            vk::Filter findMinFilter(int min_filter) {
                switch(min_filter) {
                case TINYGLTF_TEXTURE_FILTER_NEAREST:
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
                    return vk::Filter::eNearest;
                case TINYGLTF_TEXTURE_FILTER_LINEAR:
                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
                    return vk::Filter::eLinear;
                default:
                    return vk::Filter::eLinear;
                }
            }
    
            vk::SamplerMipmapMode findMipmapMode(int min_filter) {
                switch(min_filter) {
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
                    return vk::SamplerMipmapMode::eNearest;
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
                    return vk::SamplerMipmapMode::eLinear;
                default:
                    return vk::SamplerMipmapMode::eLinear;
                }
            }
    
            vk::Filter findMagFilter(int mag_filter) {
                switch(mag_filter) {
                case TINYGLTF_TEXTURE_FILTER_NEAREST:
                    return vk::Filter::eNearest;
                case TINYGLTF_TEXTURE_FILTER_LINEAR:
                    return vk::Filter::eLinear;
                default:
                    return vk::Filter::eLinear;
                }
            }
    
            vk::SamplerAddressMode findWrapMode(int wrap) {
                switch(wrap) {
                case TINYGLTF_TEXTURE_WRAP_REPEAT:
                    return vk::SamplerAddressMode::eRepeat;
                case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
                    return vk::SamplerAddressMode::eClampToEdge;
                case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
                    return vk::SamplerAddressMode::eMirroredRepeat;
                default:
                    return vk::SamplerAddressMode::eRepeat;
                }
            }
    
            std::vector<uint8_t> getAttributeData(const tinygltf::Model* model, uint32_t accessor_id) {
                const auto& accessor = model->accessors[accessor_id];
                const auto& buffer_view = model->bufferViews[accessor.bufferView];
                const auto& buffer = model->buffers[buffer_view.buffer];
    
                size_t stride = accessor.ByteStride(buffer_view);
                size_t start_byte = accessor.byteOffset + buffer_view.byteOffset;
                size_t end_byte = start_byte + accessor.count * stride;
    
                return { buffer.data.begin() + start_byte, buffer.data.begin() + end_byte };
            }
    
            size_t getAttributeSize(const tinygltf::Model* model, uint32_t accessor_id) {
                return model->accessors[accessor_id].count;
            }
    
            size_t getAttributeStride(const tinygltf::Model* model, uint32_t accessor_id) {
                const auto& accessor = model->accessors[accessor_id];
                const auto& buffer_view = model->bufferViews[accessor.bufferView];
                return accessor.ByteStride(buffer_view);
            }
    
            vk::Format getAttributeFormat(const tinygltf::Model* model, uint32_t accessor_id) {
                const auto& accessor = model->accessors[accessor_id];
    
                switch(accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_BYTE: {
                    static const std::map<int, vk::Format> mapped_format = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR8Sint},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR8G8Sint},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR8G8B8Sint},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR8G8B8A8Sint}
                    };
                    return mapped_format.at(accessor.type);
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                    static const std::map<int, vk::Format> mapped_format = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR8Uint},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR8G8Uint},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR8G8B8Uint},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR8G8B8A8Uint}
                    };
                    static const std::map<int, vk::Format> mapped_format_normalize = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR8Unorm},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR8G8Unorm},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR8G8B8Unorm},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR8G8B8A8Unorm}
                    };
                    return accessor.normalized ? mapped_format_normalize.at(accessor.type) : mapped_format.at(accessor.type);
                }
                case TINYGLTF_COMPONENT_TYPE_SHORT: {
                    static const std::map<int, vk::Format> mapped_format = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR16Sint},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR16G16Sint},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR16G16B16Sint},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR16G16B16A16Sint}
                    };
                    return mapped_format.at(accessor.type);
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                    static const std::map<int, vk::Format> mapped_format = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR16Uint},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR16G16Uint},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR16G16B16Uint},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR16G16B16A16Uint}
                    };
                    static const std::map<int, vk::Format> mapped_format_normalize = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR16Unorm},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR16G16Unorm},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR16G16B16Unorm},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR16G16B16A16Unorm}
                    };
                    return accessor.normalized ? mapped_format_normalize.at(accessor.type) : mapped_format.at(accessor.type);
                }
                case TINYGLTF_COMPONENT_TYPE_INT: {
                    static const std::map<int, vk::Format> mapped_format = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR32Sint},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR32G32Sint},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR32G32B32Sint},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR32G32B32A32Sint}
                    };
                    return mapped_format.at(accessor.type);
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                    static const std::map<int, vk::Format> mapped_format = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR32Uint},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR32G32Uint},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR32G32B32Uint},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR32G32B32A32Uint}
                    };
                    return mapped_format.at(accessor.type);
                }
                case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                    static const std::map<int, vk::Format> mapped_format = {
                        {TINYGLTF_TYPE_SCALAR, vk::Format::eR32Sfloat},
                        {TINYGLTF_TYPE_VEC2, vk::Format::eR32G32Sfloat},
                        {TINYGLTF_TYPE_VEC3, vk::Format::eR32G32B32Sfloat},
                        {TINYGLTF_TYPE_VEC4, vk::Format::eR32G32B32A32Sfloat}
                    };
                    return mapped_format.at(accessor.type);
                }
                default:
                    return vk::Format::eUndefined;
                }
            }
    
            std::vector<uint8_t> convertUnderlyingDataStride(const std::vector<uint8_t>& src_data, uint32_t src_stride, uint32_t dst_stride) {
                auto elem_count = static_cast<uint32_t>(src_data.size()) / src_stride;
                std::vector<uint8_t> result(elem_count * dst_stride);
    
                for (uint32_t idx_src = 0, idx_dst = 0;
                    idx_src < src_data.size() && idx_dst < result.size();
                    idx_src += src_stride, idx_dst += dst_stride)
                {
                    std::copy(src_data.begin() + idx_src, src_data.begin() + idx_src + src_stride, result.begin() + idx_dst);
                }
    
                return result;
            }
    
            void uploadImageToGpu(core::CommandBuffer& command_buffer, Buffer& staging_buffer, scene::Image& image) {
    
                image.clearData();
    
                {
                    ImageMemoryBarrier memory_barrier{};
                    memory_barrier.m_old_layout = vk::ImageLayout::eUndefined;
                    memory_barrier.m_new_layout = vk::ImageLayout::eTransferDstOptimal;
                    memory_barrier.m_src_access_mask = {};
                    memory_barrier.m_dst_access_mask = vk::AccessFlagBits::eTransferWrite;
                    memory_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eHost;
                    memory_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
    
                    command_buffer.imageMemoryBarrier(image.getImageView(), memory_barrier);
                }
                
                auto& mipmaps = image.getMipmaps();
                std::vector<vk::BufferImageCopy> buffer_copy_regions(mipmaps.size());
    
                for (size_t i = 0; i < mipmaps.size(); ++i) {
                    auto& mipmap = mipmaps[i];
                    auto& copy_region = buffer_copy_regions[i];
    
                    copy_region.bufferOffset = mipmap.offset;
                    copy_region.imageSubresource = image.getImageView().getSubresourceLayers();
                    copy_region.imageSubresource.mipLevel = mipmap.level;
                    copy_region.imageExtent = mipmap.extent;
                }
    
                command_buffer.copyBufferToImage(staging_buffer, image.getImage(), buffer_copy_regions);
    
                {
                    ImageMemoryBarrier memory_barrier{};
                    memory_barrier.m_old_layout = vk::ImageLayout::eTransferDstOptimal;
                    memory_barrier.m_new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    memory_barrier.m_src_access_mask = vk::AccessFlagBits::eTransferWrite;
                    memory_barrier.m_dst_access_mask = vk::AccessFlagBits::eShaderRead;
                    memory_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eTransfer;
                    memory_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eFragmentShader;
    
                    command_buffer.imageMemoryBarrier(image.getImageView(), memory_barrier);
                }
            }
    
            void prepareMeshlets(std::vector<Meshlet>& meshlets, std::unique_ptr<scene::SubMesh>& submesh, std::vector<unsigned char>& index_data) {
    
                Meshlet meshlet;
                meshlet.vertex_count = 0;
                meshlet.index_count = 0;
    
                std::set<uint32_t> vertices;
                uint32_t triangle_check = 0;
    
                for (uint32_t i = 0; i < submesh->m_vertex_indices; i++) {
    
                    meshlet.indices[meshlet.index_count] = *(reinterpret_cast<uint32_t*>(index_data.data()) + i);
    
                    if(vertices.insert(meshlet.indices[meshlet.index_count]).second) {
                        ++meshlet.vertex_count;
                    }
    
                    meshlet.index_count++;
                    triangle_check = triangle_check < 3 ? ++triangle_check : 1;
                    
                    if(meshlet.vertex_count == 64 || meshlet.index_count == 96 || i == submesh->m_vertex_indices - 1) {
    
                        if(i == submesh->m_vertex_indices - 1) {
                            assert(triangle_check == 3);
                        }
    
                        uint32_t counter = 0;
    
                        for (auto v : vertices) {
                            meshlet.vertices[counter++] = v;
                        }
    
                        if(triangle_check != 3) {
                            meshlet.index_count -= triangle_check;
                            i -= triangle_check;
                            triangle_check = 0;
                        }
    
                        meshlets.push_back(meshlet);
                        meshlet.vertex_count = 0;
                        meshlet.index_count = 0;
                        vertices.clear();
                    }
                }
            }
    
            bool textureNeedsSrgbColorspace(const std::string& name) {
    
                if(name == "baseColorTexture" || name == "emissiveTexture") {
                    return true;
                }
                return false;
            }
            
            std::string toSnakeCase(const std::string& str) {
    
                std::string result;
                result.reserve(str.size() + 8);
    
                for (size_t i = 0; i < str.size(); ++i) {
                    char c = str[i];
                    if(i > 0 && std::isupper(c) && !std::isupper(str[i - 1])) {
                        result += '_';
                    }
                    result += std::tolower(c);
                }
                return result;
            }
        }
        
        std::unordered_map<std::string, bool> GltfLoader::m_supported_extensions = {
            {KHR_LIGHTS_PUNCTUAL_EXTENSION, false},
        	{KHR_MATERIALS_UNLIT_EXTENSION, false},
        	{KHR_TEXTURE_TRANSFORM_EXTENSION, false},

        	{EXT_TEXTURE_WEBP_EXTENSION, false},
        	{KHR_TEXTURE_BASISU_EXTENSION, false},


        	{KHR_DRACO_MESH_COMPRESSION_EXTENSION, false},
        	{KHR_MESH_QUANTIZATION_EXTENSION, false},

        	{KHR_MATERIALS_PBR_SPECULAR_GLOSSINESS, false},
        	{KHR_TECHNIQUES_WEBGL_EXTENSION, false},
        };
    
        GltfLoader::GltfLoader(core::Device& device) : m_device(device) {}
    
        std::unique_ptr<scene::Scene> GltfLoader::readSceneFromFile(
            const std::string& file_name,
            int scene_index,
            vk::BufferUsageFlags additional_buffer_usage_flags)
        {
            PROFILE_SCOPE("Load GLTF Scene");
    
            std::string err;
            std::string warn;
    
            tinygltf::TinyGLTF gltf_loader;
            std::string gltf_file = filesystem::path::get(filesystem::path::Type::Assets) + file_name;
    
            bool import_result = gltf_loader.LoadASCIIFromFile(&m_model, &err, &warn, gltf_file.c_str());
    
            if(!import_result) {
                LOGE("Failed to load gltf file {}.", gltf_file.c_str());
                return nullptr;
            }
    
            if(!err.empty()) {
                LOGE("Error loading gltf model: {}.", err.c_str());
                return nullptr;
            }
    
            if(!warn.empty()) {
                LOGI("{}", warn.c_str());
            }
    
            size_t pos = file_name.find_last_of('/');
            m_model_path = file_name.substr(0, pos);
    
            if(pos == std::string::npos) {
                m_model_path.clear();
            }
    
            return std::make_unique<scene::Scene>(loadScene(scene_index, additional_buffer_usage_flags));
        }
    
        std::unique_ptr<scene::SubMesh> GltfLoader::readModelFromFile(
            const std::string& file_name,
            uint32_t index,
            bool storage_buffer,
            vk::BufferUsageFlags additional_buffer_usage_flags)
        {
            PROFILE_SCOPE("Load GLTF Model");
    
            std::string err;
            std::string warn;
    
            tinygltf::TinyGLTF gltf_loader;
            std::string gltf_file = filesystem::path::get(filesystem::path::Type::Assets) + file_name;
    
            bool import_result = gltf_loader.LoadASCIIFromFile(&m_model, &err, &warn, gltf_file.c_str());
    
            if(!import_result) {
                LOGE("Failed to load gltf file {}.", gltf_file.c_str());
                return nullptr;
            }
    
            if(!err.empty()) {
                LOGE("Error loading gltf model: {}.", err.c_str());
                return nullptr;
            }
    
            if(!warn.empty()) {
                LOGI("{}", warn.c_str());
            }
    
            size_t pos = file_name.find_last_of('/');
            m_model_path = file_name.substr(0, pos);
    
            if(pos == std::string::npos) {
                m_model_path.clear();
            }
    
            return loadModel(index, storage_buffer, additional_buffer_usage_flags);
        }
    
        scene::Scene GltfLoader::loadScene(int scene_index, vk::BufferUsageFlags additional_buffer_usage_flags) {
    
            PROFILE_SCOPE("Process Scene");
    
            auto scene = scene::Scene();
            scene.setName("gltf_scene");
            
            for (auto& used_extension : m_model.extensionsUsed) {
    
                auto it = m_supported_extensions.find(used_extension);
    
                if(it == m_supported_extensions.end()) {
                    if(std::find(m_model.extensionsRequired.begin(), m_model.extensionsRequired.end(), used_extension) != m_model.extensionsRequired.end()) {
                        throw std::runtime_error("Cannot load glTF file. Contains a required unsupported extension: " + used_extension);
                    }
                    else {
                        LOGW("gltf file contains an unsupported extension, unexpected results may occur: {}", used_extension);
                    }
                }
                else {
                    LOGI("gltf file contains extension: {}", used_extension);
                    it->second = true;
                }
            }
            
            std::vector<std::unique_ptr<scene::Light>> light_components = parseKhrLightsPunctual();
            scene.setComponents(std::move(light_components));
            
            std::vector<std::unique_ptr<scene::Sampler>> sampler_components(m_model.samplers.size());
            for (size_t sampler_index = 0; sampler_index < m_model.samplers.size(); sampler_index++) {
                sampler_components[sampler_index] = parseSampler(m_model.samplers[sampler_index]);
            }
            scene.setComponents(std::move(sampler_components));
            
            auto thread_count = std::thread::hardware_concurrency();
            thread_count = thread_count == 0 ? 1 : thread_count;
            
            BS::thread_pool thread_pool(thread_count);
    
            auto image_count = static_cast<uint32_t>(m_model.images.size());
            std::vector<std::future<std::unique_ptr<scene::Image>>> image_component_futures;
    
            for (size_t image_index = 0; image_index < image_count; image_index++) {
    
                auto fut = thread_pool.submit_task(
                    [this, image_index]() {
                        auto image = parseImage(m_model.images[image_index]);
                        LOGI("Loaded gltf image #{} ({})", image_index, m_model.images[image_index].uri.c_str());
                        return image;
                    });
    
                image_component_futures.push_back(std::move(fut));
            }
    
            std::vector<std::unique_ptr<scene::Image>> image_components;
            
            size_t image_index = 0;
            while (image_index < image_count) {
    
                std::vector<Buffer> transient_buffers;
                auto& command_buffer = m_device.getCommandPool().requestCommandBuffer();
                command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    
                size_t batch_size = 0;
                const size_t max_batch_size = 64 * 1024 * 1024; // 64MB
    
                while (image_index < image_count && batch_size < max_batch_size) {
                    image_components.push_back(image_component_futures[image_index].get());
                    auto& image = image_components[image_index];
    
                    Buffer stage_buffer = Buffer::createStagingBuffer(m_device, image->getData());
                    batch_size += image->getData().size();
    
                    uploadImageToGpu(command_buffer, stage_buffer, *image);
                    transient_buffers.push_back(std::move(stage_buffer));
                    image_index++;
                }
    
                command_buffer.end();
    
                auto& queue = m_device.getQueueByFlags(vk::QueueFlagBits::eGraphics, 0);
                queue.submit(command_buffer, m_device.getFencePool().requestFence());
    
                m_device.getFencePool().wait();
                m_device.getFencePool().reset();
                m_device.getCommandPool().resetPool();
                m_device.getHandle().waitIdle();
    
                transient_buffers.clear();
            }
    
            scene.setComponents(std::move(image_components));
            
            auto images = scene.getComponents<scene::Image>();
            auto samplers = scene.getComponents<scene::Sampler>();
            auto default_sampler_linear = createDefaultSampler(TINYGLTF_TEXTURE_FILTER_LINEAR);
            auto default_sampler_nearest = createDefaultSampler(TINYGLTF_TEXTURE_FILTER_NEAREST);
            bool used_nearest_sampler = false;
    
            for (auto& gltf_texture : m_model.textures) {
                auto texture = parseTexture(gltf_texture);
    
                assert(gltf_texture.source < images.size());
                texture->setImage(*images[gltf_texture.source]);
    
                if(gltf_texture.sampler >= 0 && gltf_texture.sampler < static_cast<int>(samplers.size())) {
                    texture->setSampler(*samplers[gltf_texture.sampler]);
                }
                else {
                    if(gltf_texture.name.empty()) {
                        gltf_texture.name = images[gltf_texture.source]->getName();
                    }
    
                    const vk::FormatProperties fmt_props = m_device.getPhysicalDevice().getFormatProperties(images[gltf_texture.source]->getFormat());
    
                    if(fmt_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear) {
                        texture->setSampler(*default_sampler_linear);
                    }
                    else {
                        texture->setSampler(*default_sampler_nearest);
                        used_nearest_sampler = true;
                    }
                }
    
                scene.addComponent(std::move(texture));
            }
    
            scene.addComponent(std::move(default_sampler_linear));
    
            if(used_nearest_sampler) {
                scene.addComponent(std::move(default_sampler_nearest));
            }
            
            bool has_textures = scene.hasComponent<scene::Texture>();
            std::vector<scene::Texture*> textures;
    
            if(has_textures) {
                textures = scene.getComponents<scene::Texture>();
            }
    
            for (auto& gltf_material : m_model.materials) {
    
                auto material = parseMaterial(gltf_material);
    
                for (auto& gltf_value : gltf_material.values) {
                    if(gltf_value.first.find("Texture") != std::string::npos) {
                        std::string tex_name = toSnakeCase(gltf_value.first);
    
                        assert(gltf_value.second.TextureIndex() < textures.size());
                        scene::Texture* tex = textures[gltf_value.second.TextureIndex()];
    
                        if(textureNeedsSrgbColorspace(gltf_value.first)) {
                            tex->getImage()->coerceFormatToSrgb();
                        }
    
                        material->m_textures[tex_name] = tex;
                    }
                }
    
                for (auto& gltf_value : gltf_material.additionalValues) {
                    if(gltf_value.first.find("Texture") != std::string::npos) {
                        std::string tex_name = toSnakeCase(gltf_value.first);
    
                        assert(gltf_value.second.TextureIndex() < textures.size());
                        scene::Texture* tex = textures[gltf_value.second.TextureIndex()];
    
                        if(textureNeedsSrgbColorspace(gltf_value.first)) {
                            tex->getImage()->coerceFormatToSrgb();
                        }
    
                        material->m_textures[tex_name] = tex;
                    }
                }
    
                scene.addComponent(std::move(material));
            }
    
            auto default_material = createDefaultMaterial();
            
            auto materials = scene.getComponents<scene::PBRMaterial>();
    
            for (auto& gltf_mesh : m_model.meshes) {
                PROFILE_SCOPE("Processing Mesh");
    
                auto mesh = parseMesh(gltf_mesh);
    
                for (size_t i_primitive = 0; i_primitive < gltf_mesh.primitives.size(); i_primitive++) {
                    const auto& gltf_primitive = gltf_mesh.primitives[i_primitive];
    
                    auto submesh_name = fmt::format("'{}' mesh, primitive #{}", gltf_mesh.name, i_primitive);
                    auto submesh = std::make_unique<scene::SubMesh>(std::move(submesh_name));
    
                    for (auto& attribute : gltf_primitive.attributes) {
                        std::string attrib_name = attribute.first;
                        std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::tolower);
    
                        auto vertex_data = getAttributeData(&m_model, attribute.second);
    
                        if(attrib_name == "position") {
                            assert(attribute.second < m_model.accessors.size());
                            submesh->m_vertices_count = static_cast<uint32_t>(m_model.accessors[attribute.second].count);
                        }
    
                        Buffer buffer{ m_device,
                        	vertex_data.size(),
                        	vk::BufferUsageFlagBits::eVertexBuffer | additional_buffer_usage_flags,
                        	VMA_MEMORY_USAGE_CPU_TO_GPU };
    
                        buffer.update(vertex_data);
                        buffer.setDebugName(fmt::format("'{}' mesh, primitive #{}: '{}' vertex buffer",
                            gltf_mesh.name, i_primitive, attrib_name));
    
                        submesh->m_vertex_buffers.insert(std::make_pair(attrib_name, std::move(buffer)));
    
                        scene::VertexAttribute attrib;
                        attrib.format = getAttributeFormat(&m_model, attribute.second);
                        attrib.stride = static_cast<uint32_t>(getAttributeStride(&m_model, attribute.second));
    
                        submesh->setAttribute(attrib_name, attrib);
                    }
    
                    if(gltf_primitive.indices >= 0) {
                        submesh->m_vertex_indices = static_cast<uint32_t>(getAttributeSize(&m_model, gltf_primitive.indices));
    
                        auto format = getAttributeFormat(&m_model, gltf_primitive.indices);
                        auto index_data = getAttributeData(&m_model, gltf_primitive.indices);
    
                        switch(format) {
                        case vk::Format::eR8Uint:
                            index_data = convertUnderlyingDataStride(index_data, 1, 2);
                            submesh->m_index_type = vk::IndexType::eUint16;
                            break;
                        case vk::Format::eR16Uint:
                            submesh->m_index_type = vk::IndexType::eUint16;
                            break;
                        case vk::Format::eR32Uint:
                            submesh->m_index_type = vk::IndexType::eUint32;
                            break;
                        default:
                            LOGE("gltf primitive has invalid format type");
                            break;
                        }
    
                        submesh->m_index_buffer = std::make_unique<Buffer>(m_device,
                            index_data.size(),
                            vk::BufferUsageFlagBits::eIndexBuffer | additional_buffer_usage_flags,
                            VMA_MEMORY_USAGE_GPU_TO_CPU);
                        submesh->m_index_buffer->setDebugName(fmt::format("'{}' mesh, primitive #{}: index buffer",
                            gltf_mesh.name, i_primitive));
    
                        submesh->m_index_buffer->update(index_data);
                    }
                    else {
                        submesh->m_vertices_count = static_cast<uint32_t>(getAttributeSize(&m_model, gltf_primitive.attributes.at("POSITION")));
                    }
    
                    if(gltf_primitive.material < 0) {
                        submesh->setMaterial(*default_material);
                    }
                    else {
                        assert(gltf_primitive.material < materials.size());
                        submesh->setMaterial(*materials[gltf_primitive.material]);
                    }
    
                    mesh->addSubmesh(*submesh);
                    scene.addComponent(std::move(submesh));
                }
    
                scene.addComponent(std::move(mesh));
            }
    
            m_device.getFencePool().wait();
            m_device.getFencePool().reset();
            m_device.getCommandPool().resetPool();
    
            scene.addComponent(std::move(default_material));
            
            for (auto& gltf_camera : m_model.cameras) {
                auto camera = parseCamera(gltf_camera);
                scene.addComponent(std::move(camera));
            }
            
            auto meshes = scene.getComponents<scene::Mesh>();
            std::vector<std::unique_ptr<scene::Node>> nodes;
    
            for (size_t node_index = 0; node_index < m_model.nodes.size(); ++node_index) {
                auto gltf_node = m_model.nodes[node_index];
                auto node = parseNode(gltf_node, node_index);
    
                if(gltf_node.mesh >= 0) {
                    assert(gltf_node.mesh < meshes.size());
                    auto mesh = meshes[gltf_node.mesh];
    
                    node->setComponent(*mesh);
                    mesh->addNode(*node);
                }
    
                if(gltf_node.camera >= 0) {
                    auto cameras = scene.getComponents<scene::Camera>();
                    assert(gltf_node.camera < cameras.size());
                    auto camera = cameras[gltf_node.camera];
    
                    node->setComponent(*camera);
                    camera->setNode(*node);
                }
    
                if(auto extension = getExtension(gltf_node.extensions, KHR_LIGHTS_PUNCTUAL_EXTENSION)) {
                    auto lights = scene.getComponents<scene::Light>();
                    int light_index = extension->Get("light").Get<int>();
                    assert(light_index < lights.size());
                    auto light = lights[light_index];
    
                    node->setComponent(*light);
                    light->setNode(*node);
                }
    
                nodes.push_back(std::move(node));
            }
    
            std::vector<std::unique_ptr<scene::Animation>> animations;
            
            for (size_t animation_index = 0; animation_index < m_model.animations.size(); ++animation_index) {
    
                auto& gltf_animation = m_model.animations[animation_index];
                std::vector<scene::AnimationSampler> samplers;
    
                for (size_t sampler_index = 0; sampler_index < gltf_animation.samplers.size(); ++sampler_index) {
    
                    auto gltf_sampler = gltf_animation.samplers[sampler_index];
                    scene::AnimationSampler sampler;
    
                    if(gltf_sampler.interpolation == "LINEAR") {
                        sampler.type = scene::AnimationType::Linear;
                    }
                    else if(gltf_sampler.interpolation == "STEP") {
                        sampler.type = scene::AnimationType::Step;
                    }
                    else if(gltf_sampler.interpolation == "CUBICSPLINE") {
                        sampler.type = scene::AnimationType::CubicSpline;
                    }
                    else {
                        LOGW("gltf animation sampler #{} has unknown interpolation value", sampler_index);
                    }
    
                    auto input_accessor = m_model.accessors[gltf_sampler.input];
                    auto input_accessor_data = getAttributeData(&m_model, gltf_sampler.input);
                    const float* data = reinterpret_cast<const float*>(input_accessor_data.data());
    
                    for (size_t i = 0; i < input_accessor.count; ++i) {
                        sampler.inputs.push_back(data[i]);
                    }
    
                    auto output_accessor = m_model.accessors[gltf_sampler.output];
                    auto output_accessor_data = getAttributeData(&m_model, gltf_sampler.output);
    
                    switch(output_accessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                        const glm::vec3* data = reinterpret_cast<const glm::vec3*>(output_accessor_data.data());
                        for (size_t i = 0; i < output_accessor.count; ++i) {
                            sampler.outputs.push_back(glm::vec4(data[i], 0.0f));
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4: {
                        const glm::vec4* data = reinterpret_cast<const glm::vec4*>(output_accessor_data.data());
                        for (size_t i = 0; i < output_accessor.count; ++i) {
                            sampler.outputs.push_back(data[i]);
                        }
                        break;
                    }
                    default: {
                        LOGW("gltf animation sampler #{} has unknown output data type", sampler_index);
                        continue;
                    }
                    }
    
                    samplers.push_back(sampler);
                }
    
                auto animation = std::make_unique<scene::Animation>(gltf_animation.name);
    
                for (size_t channel_index = 0; channel_index < gltf_animation.channels.size(); ++channel_index) {
    
                    auto& gltf_channel = gltf_animation.channels[channel_index];
                    scene::AnimationTarget target;
    
                    if(gltf_channel.target_path == "translation") {
                        target = scene::AnimationTarget::Translation;
                    }
                    else if(gltf_channel.target_path == "rotation") {
                        target = scene::AnimationTarget::Rotation;
                    }
                    else if(gltf_channel.target_path == "scale") {
                        target = scene::AnimationTarget::Scale;
                    }
                    else if(gltf_channel.target_path == "weights") {
                        LOGW("gltf animation channel #{} has unsupported target path: {}", channel_index, gltf_channel.target_path);
                        continue;
                    }
                    else {
                        LOGW("gltf animation channel #{} has unknown target path", channel_index);
                        continue;
                    }
    
                    float start_time = std::numeric_limits<float>::max();
                    float end_time = std::numeric_limits<float>::min();
    
                    for (auto input : samplers[gltf_channel.sampler].inputs) {
                        if(input < start_time) start_time = input;
                        if(input > end_time) end_time = input;
                    }
    
                    animation->updateTimes(start_time, end_time);
                    animation->addChannel(*nodes[gltf_channel.target_node], target, samplers[gltf_channel.sampler]);
                }
    
                animations.push_back(std::move(animation));
            }
    
            scene.setComponents(std::move(animations));
            
            std::queue<std::pair<scene::Node&, int>> traverse_nodes;
            tinygltf::Scene* gltf_scene = nullptr;
    
            if(scene_index >= 0 && scene_index < static_cast<int>(m_model.scenes.size())) {
                gltf_scene = &m_model.scenes[scene_index];
            }
            else if(m_model.defaultScene >= 0 && m_model.defaultScene < static_cast<int>(m_model.scenes.size())) {
                gltf_scene = &m_model.scenes[m_model.defaultScene];
            }
            else if(!m_model.scenes.empty()) {
                gltf_scene = &m_model.scenes[0];
            }
    
            if(!gltf_scene) {
                throw std::runtime_error("Couldn't determine which scene to load!");
            }
    
            auto root_node = std::make_unique<scene::Node>(0, gltf_scene->name);
    
            for (auto node_index : gltf_scene->nodes) {
                traverse_nodes.push(std::make_pair(std::ref(*root_node), node_index));
            }
    
            while (!traverse_nodes.empty()) {
                auto node_it = traverse_nodes.front();
                traverse_nodes.pop();
    
                if(node_it.second >= nodes.size()) {
                    continue;
                }
    
                auto& current_node = *nodes[node_it.second];
                auto& traverse_root_node = node_it.first;
    
                current_node.setParent(traverse_root_node);
                traverse_root_node.addChild(current_node);
    
                for (auto child_node_index : m_model.nodes[node_it.second].children) {
                    traverse_nodes.push(std::make_pair(std::ref(current_node), child_node_index));
                }
            }
    
            scene.setRootNode(*root_node);
            nodes.push_back(std::move(root_node));
            scene.setNodes(std::move(nodes));
            
            auto camera_node = std::make_unique<scene::Node>(-1, "default_camera");
            auto default_camera = createDefaultCamera();
            default_camera->setNode(*camera_node);
            camera_node->setComponent(*default_camera);
            scene.addComponent(std::move(default_camera));
            scene.getRootNode().addChild(*camera_node);
            scene.addNode(std::move(camera_node));
            
            if(!scene.hasComponent<scene::Light>()) {
                addDirectionalLight(scene, glm::quat({ glm::radians(-90.0f), 0.0f, glm::radians(30.0f) }));
            }
    
            return scene;
        }
    
        std::unique_ptr<scene::SubMesh> GltfLoader::loadModel(uint32_t index, bool storage_buffer, vk::BufferUsageFlags additional_buffer_usage_flags) {
    
        	PROFILE_SCOPE("Process Model");
            auto submesh = std::make_unique<scene::SubMesh>();
            std::vector<Buffer> transient_buffers;
    
            auto& queue = m_device.getQueueByFlags(vk::QueueFlagBits::eGraphics, 0);
            auto& command_buffer = m_device.getCommandPool().requestCommandBuffer();
            command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    
            assert(index < m_model.meshes.size());
            auto& gltf_mesh = m_model.meshes[index];
    
            assert(!gltf_mesh.primitives.empty());
            auto& gltf_primitive = gltf_mesh.primitives[0];
    
            std::vector<Vertex> vertex_data;
            std::vector<AlignedVertex> aligned_vertex_data;
            
            const float* pos = nullptr;
            const float* normals = nullptr;
            const float* uvs = nullptr;
            const uint16_t* joints = nullptr;
            const float* weights = nullptr;
            const float* colors = nullptr;
            uint32_t color_component_count = 4;
            
            auto& accessor = m_model.accessors[gltf_primitive.attributes.find("POSITION")->second];
            size_t vertex_count = accessor.count;
            auto& buffer_view = m_model.bufferViews[accessor.bufferView];
            pos = reinterpret_cast<const float*>(&(m_model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
    
            submesh->m_vertices_count = static_cast<uint32_t>(vertex_count);
            
            if(gltf_primitive.attributes.find("NORMAL") != gltf_primitive.attributes.end()) {
                accessor = m_model.accessors[gltf_primitive.attributes.find("NORMAL")->second];
                buffer_view = m_model.bufferViews[accessor.bufferView];
                normals = reinterpret_cast<const float*>(&(m_model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
            }
    
            if(gltf_primitive.attributes.find("TEXCOORD_0") != gltf_primitive.attributes.end()) {
                accessor = m_model.accessors[gltf_primitive.attributes.find("TEXCOORD_0")->second];
                buffer_view = m_model.bufferViews[accessor.bufferView];
                uvs = reinterpret_cast<const float*>(&(m_model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
            }
    
            if(gltf_primitive.attributes.find("COLOR_0") != gltf_primitive.attributes.end()) {
                accessor = m_model.accessors[gltf_primitive.attributes.find("COLOR_0")->second];
                buffer_view = m_model.bufferViews[accessor.bufferView];
                colors = reinterpret_cast<const float*>(&(m_model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
                color_component_count = accessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
            }
            
            if(gltf_primitive.attributes.find("JOINTS_0") != gltf_primitive.attributes.end()) {
                accessor = m_model.accessors[gltf_primitive.attributes.find("JOINTS_0")->second];
                buffer_view = m_model.bufferViews[accessor.bufferView];
                joints = reinterpret_cast<const uint16_t*>(&(m_model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
            }
    
            if(gltf_primitive.attributes.find("WEIGHTS_0") != gltf_primitive.attributes.end()) {
                accessor = m_model.accessors[gltf_primitive.attributes.find("WEIGHTS_0")->second];
                buffer_view = m_model.bufferViews[accessor.bufferView];
                weights = reinterpret_cast<const float*>(&(m_model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
            }
    
            bool has_skin = (joints && weights);
    
            if(storage_buffer) {
                for (size_t v = 0; v < vertex_count; v++) {
                    AlignedVertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&pos[v * 3]), 1.0f);
                    vert.normal = normals ?
                        glm::vec4(glm::normalize(glm::make_vec3(&normals[v * 3])), 0.0f) :
                        glm::vec4(0.0f);
                    aligned_vertex_data.push_back(vert);
                }
    
                Buffer stage_buffer = Buffer::createStagingBuffer(m_device, aligned_vertex_data);
    
                Buffer buffer{ m_device,
                	aligned_vertex_data.size() * sizeof(AlignedVertex),
                	vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
                	VMA_MEMORY_USAGE_GPU_ONLY };
    
                command_buffer.copyBuffer(stage_buffer, buffer, aligned_vertex_data.size() * sizeof(AlignedVertex));
                submesh->m_vertex_buffers.insert(std::make_pair("vertex_buffer", std::move(buffer)));
                transient_buffers.push_back(std::move(stage_buffer));
            }
            else {
                for (size_t v = 0; v < vertex_count; v++) {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&pos[v * 3]), 1.0f);
                    vert.normal = glm::normalize(normals ?
                        glm::make_vec3(&normals[v * 3]) :
                        glm::vec3(0.0f));
                    vert.uv = uvs ? glm::make_vec2(&uvs[v * 2]) : glm::vec2(0.0f);
    
                    if(colors) {
                        switch(color_component_count) {
                        case 3:
                            vert.color = glm::vec4(glm::make_vec3(&colors[v * 3]), 1.0f);
                            break;
                        case 4:
                            vert.color = glm::make_vec4(&colors[v * 4]);
                            break;
                        }
                    }
                    else {
                        vert.color = glm::vec4(1.0f);
                    }
    
                    vert.joint0 = has_skin ? glm::vec4(glm::make_vec4(&joints[v * 4])) : glm::vec4(0.0f);
                    vert.weight0 = has_skin ? glm::make_vec4(&weights[v * 4]) : glm::vec4(0.0f);
                    vertex_data.push_back(vert);
                }
    
                Buffer stage_buffer = Buffer::createStagingBuffer(m_device, vertex_data);
    
                Buffer buffer{ m_device,
                	vertex_data.size() * sizeof(Vertex),
                	vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                	VMA_MEMORY_USAGE_GPU_ONLY };
    
                command_buffer.copyBuffer(stage_buffer, buffer, vertex_data.size() * sizeof(Vertex));
                submesh->m_vertex_buffers.insert(std::make_pair("vertex_buffer", std::move(buffer)));
                transient_buffers.push_back(std::move(stage_buffer));
            }
            
            if(gltf_primitive.indices >= 0) {
                submesh->m_vertex_indices = static_cast<uint32_t>(getAttributeSize(&m_model, gltf_primitive.indices));
                auto format = getAttributeFormat(&m_model, gltf_primitive.indices);
                auto index_data = getAttributeData(&m_model, gltf_primitive.indices);
                
                switch(format) {
                case vk::Format::eR16Uint:
                    index_data = convertUnderlyingDataStride(index_data, 2, 4);
                    break;
                case vk::Format::eR8Uint:
                    index_data = convertUnderlyingDataStride(index_data, 1, 4);
                    break;
                default:
                    break;
                }
    
                submesh->m_index_type = vk::IndexType::eUint32;
    
                if(storage_buffer) {
                    std::vector<Meshlet> meshlets;
                    prepareMeshlets(meshlets, submesh, index_data);
                    
                    submesh->m_vertex_indices = static_cast<uint32_t>(meshlets.size());
    
                    Buffer stage_buffer = Buffer::createStagingBuffer(m_device, meshlets);
    
                    submesh->m_index_buffer = std::make_unique<Buffer>(m_device,
                        meshlets.size() * sizeof(Meshlet),
                        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
                        VMA_MEMORY_USAGE_GPU_ONLY);
    
                    command_buffer.copyBuffer(stage_buffer, *submesh->m_index_buffer, meshlets.size() * sizeof(Meshlet));
                    transient_buffers.push_back(std::move(stage_buffer));
                }
                else {
                    Buffer stage_buffer = Buffer::createStagingBuffer(m_device, index_data);
    
                    submesh->m_index_buffer = std::make_unique<Buffer>(m_device,
                        index_data.size(),
                        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                        VMA_MEMORY_USAGE_GPU_ONLY);
    
                    command_buffer.copyBuffer(stage_buffer, *submesh->m_index_buffer, index_data.size());
                    transient_buffers.push_back(std::move(stage_buffer));
                }
            }
    
            command_buffer.end();
            queue.submit(command_buffer, m_device.getFencePool().requestFence());
    
            m_device.getFencePool().wait();
            m_device.getFencePool().reset();
            m_device.getCommandPool().resetPool();
    
            return submesh;
        }
    
        std::unique_ptr<scene::Node> GltfLoader::parseNode(const tinygltf::Node& gltf_node, size_t index) const {
    
            auto node = std::make_unique<scene::Node>(index, gltf_node.name);
            auto& transform = node->getComponent<scene::Transform>();
    
            if(!gltf_node.translation.empty()) {
                glm::vec3 translation;
                std::transform(gltf_node.translation.begin(), gltf_node.translation.end(),
                    glm::value_ptr(translation), TypeCast<double, float>{});
                transform.setTranslation(translation);
            }
    
            if(!gltf_node.rotation.empty()) {
                glm::quat rotation;
                std::transform(gltf_node.rotation.begin(), gltf_node.rotation.end(),
                    glm::value_ptr(rotation), TypeCast<double, float>{});
                transform.setRotation(rotation);
            }
    
            if(!gltf_node.scale.empty()) {
                glm::vec3 scale;
                std::transform(gltf_node.scale.begin(), gltf_node.scale.end(),
                    glm::value_ptr(scale), TypeCast<double, float>{});
                transform.setScale(scale);
            }
    
            if(!gltf_node.matrix.empty()) {
                glm::mat4 matrix;
                std::transform(gltf_node.matrix.begin(), gltf_node.matrix.end(),
                    glm::value_ptr(matrix), TypeCast<double, float>{});
                transform.setMatrix(matrix);
            }
    
            return node;
        }
    
        std::unique_ptr<scene::Camera> GltfLoader::parseCamera(const tinygltf::Camera& gltf_camera) const {
            std::unique_ptr<scene::Camera> camera;
    
            if(gltf_camera.type == "perspective") {
                auto perspective_camera = std::make_unique<scene::PerspectiveCamera>(gltf_camera.name);
                perspective_camera->setAspectRatio(static_cast<float>(gltf_camera.perspective.aspectRatio));
                perspective_camera->setFieldOfView(static_cast<float>(gltf_camera.perspective.yfov));
                perspective_camera->setNearPlane(static_cast<float>(gltf_camera.perspective.znear));
                perspective_camera->setFarPlane(static_cast<float>(gltf_camera.perspective.zfar));
                camera = std::move(perspective_camera);
            }
            else {
                LOGW("Camera type not supported");
            }
    
            return camera;
        }
    
        std::unique_ptr<scene::Mesh> GltfLoader::parseMesh(const tinygltf::Mesh& gltf_mesh) const {
            return std::make_unique<scene::Mesh>(gltf_mesh.name);
        }
    
        std::unique_ptr<scene::PBRMaterial> GltfLoader::parseMaterial(const tinygltf::Material& gltf_material) const {
    
            auto material = std::make_unique<scene::PBRMaterial>(gltf_material.name);
            material->m_color = glm::vec4(1.0f);
    
            for (auto& gltf_value : gltf_material.values) {
                if(gltf_value.first == "baseColorFactor") {
                    const auto& color_factor = gltf_value.second.ColorFactor();
                    material->m_color = glm::vec4(color_factor[0], color_factor[1], color_factor[2], color_factor[3]);
                }
                else if(gltf_value.first == "metallicFactor") {
                    material->m_metallic = static_cast<float>(gltf_value.second.Factor());
                }
                else if(gltf_value.first == "roughnessFactor") {
                    material->m_roughness = static_cast<float>(gltf_value.second.Factor());
                }
            }
    
            for (auto& gltf_value : gltf_material.additionalValues) {
                if(gltf_value.first == "emissiveFactor") {
                    const auto& emissive_factor = gltf_value.second.number_array;
                    material->m_emissive = glm::vec3(emissive_factor[0], emissive_factor[1], emissive_factor[2]);
                }
                else if(gltf_value.first == "alphaMode") {
                    if(gltf_value.second.string_value == "BLEND") {
                        material->m_alpha_mode = scene::AlphaMode::Blend;
                    }
                    else if(gltf_value.second.string_value == "OPAQUE") {
                        material->m_alpha_mode = scene::AlphaMode::Opaque;
                    }
                    else if(gltf_value.second.string_value == "MASK") {
                        material->m_alpha_mode = scene::AlphaMode::Mask;
                    }
                }
                else if(gltf_value.first == "alphaCutoff") {
                    material->m_alpha_cutoff = static_cast<float>(gltf_value.second.number_value);
                }
                else if(gltf_value.first == "doubleSided") {
                    material->m_double_sided = gltf_value.second.bool_value;
                }
            }
    
            return material;
        }
    
        std::unique_ptr<scene::Image> GltfLoader::parseImage(tinygltf::Image& gltf_image) const {
    
            std::unique_ptr<scene::Image> image{ nullptr };
    
            if(!gltf_image.image.empty()) {
                auto mipmap = scene::Mipmap{
                    0,
                    0,
                    vk::Extent3D{ static_cast<uint32_t>(gltf_image.width), static_cast<uint32_t>(gltf_image.height), 1u }
                };
    
                std::vector<scene::Mipmap> mipmaps{ mipmap };
                image = std::make_unique<scene::Image>(gltf_image.name, std::move(gltf_image.image), std::move(mipmaps));
            }
            else {
                auto image_uri = m_model_path + "/" + gltf_image.uri;
                image = scene::Image::load(gltf_image.name, image_uri, scene::Image::Unknown);
            }
            
            if(scene::isAstc(image->getFormat())) {
                if(!m_device.isImageFormatSupported(image->getFormat())) {
                    LOGW("ASTC not supported: decoding {}", image->getName());
                    image = std::make_unique<scene::Astc>(*image);
                    image->generateMipmaps();
                }
            }
    
            image->createVkImage(m_device);
            return image;
        }
    
        std::unique_ptr<scene::Sampler> GltfLoader::parseSampler(const tinygltf::Sampler& gltf_sampler) const {
    
            vk::Filter min_filter = findMinFilter(gltf_sampler.minFilter);
            vk::Filter mag_filter = findMagFilter(gltf_sampler.magFilter);
            vk::SamplerMipmapMode mipmap_mode = findMipmapMode(gltf_sampler.minFilter);
            vk::SamplerAddressMode address_mode_u = findWrapMode(gltf_sampler.wrapS);
            vk::SamplerAddressMode address_mode_v = findWrapMode(gltf_sampler.wrapT);
            vk::SamplerAddressMode address_mode_w = findWrapMode(gltf_sampler.wrapR);
    
            vk::SamplerCreateInfo sampler_info;
            sampler_info.magFilter = mag_filter;
            sampler_info.minFilter = min_filter;
            sampler_info.mipmapMode = mipmap_mode;
            sampler_info.addressModeU = address_mode_u;
            sampler_info.addressModeV = address_mode_v;
            sampler_info.addressModeW = address_mode_w;
            sampler_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;
            sampler_info.maxLod = std::numeric_limits<float>::max();
    
            core::Sampler vk_sampler{ m_device, sampler_info };
            vk_sampler.setDebugName(gltf_sampler.name);
    
            return std::make_unique<scene::Sampler>(gltf_sampler.name, std::move(vk_sampler));
        }
    
        std::unique_ptr<scene::Texture> GltfLoader::parseTexture(const tinygltf::Texture& gltf_texture) const {
            return std::make_unique<scene::Texture>(gltf_texture.name);
        }
    
        std::unique_ptr<scene::PBRMaterial> GltfLoader::createDefaultMaterial() {
            tinygltf::Material gltf_material;
            return parseMaterial(gltf_material);
        }
    
        std::unique_ptr<scene::Sampler> GltfLoader::createDefaultSampler(int filter) {
            tinygltf::Sampler gltf_sampler;
            gltf_sampler.minFilter = filter;
            gltf_sampler.magFilter = filter;
            gltf_sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
            gltf_sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
            gltf_sampler.wrapR = TINYGLTF_TEXTURE_WRAP_REPEAT;
            return parseSampler(gltf_sampler);
        }
    
        std::unique_ptr<scene::Camera> GltfLoader::createDefaultCamera() {
            tinygltf::Camera gltf_camera;
            gltf_camera.name = "default_camera";
            gltf_camera.type = "perspective";
            gltf_camera.perspective.aspectRatio = 1.77f;
            gltf_camera.perspective.yfov = 1.0f;
            gltf_camera.perspective.znear = 0.1f;
            gltf_camera.perspective.zfar = 1000.0f;
            return parseCamera(gltf_camera);
        }
    
        std::vector<std::unique_ptr<scene::Light>> GltfLoader::parseKhrLightsPunctual() {
    
            if(isExtensionEnabled(KHR_LIGHTS_PUNCTUAL_EXTENSION)) {
    
                if(m_model.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION) == m_model.extensions.end() ||
                    !m_model.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Has("lights"))
                {
                    return {};
                }
    
                auto& khr_lights = m_model.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Get("lights");
                std::vector<std::unique_ptr<scene::Light>> light_components(khr_lights.ArrayLen());
    
                for (size_t light_index = 0; light_index < khr_lights.ArrayLen(); ++light_index) {
                    auto& khr_light = khr_lights.Get(static_cast<int>(light_index));
    
                    if(!khr_light.Has("type")) {
                        LOGE("KHR_lights_punctual extension: light {} doesn't have a type!", light_index);
                        throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
                    }
    
                    auto light = std::make_unique<scene::Light>(khr_light.Get("name").Get<std::string>());
                    scene::LightType type;
                    scene::LightProperties properties;
                    
                    auto& gltf_light_type = khr_light.Get("type").Get<std::string>();
                    if(gltf_light_type == "point") {
                        type = scene::LightType::Point;
                    }
                    else if(gltf_light_type == "spot") {
                        type = scene::LightType::Spot;
                    }
                    else if(gltf_light_type == "directional") {
                        type = scene::LightType::Directional;
                    }
                    else {
                        LOGE("KHR_lights_punctual extension: light type '{}' is invalid", gltf_light_type);
                        throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
                    }
                    
                    if(khr_light.Has("color")) {
                        properties.color = glm::vec3(
                            static_cast<float>(khr_light.Get("color").Get(0).Get<double>()),
                            static_cast<float>(khr_light.Get("color").Get(1).Get<double>()),
                            static_cast<float>(khr_light.Get("color").Get(2).Get<double>()));
                    }
    
                    if(khr_light.Has("intensity")) {
                        properties.intensity = static_cast<float>(khr_light.Get("intensity").Get<double>());
                    }
    
                    if(type != scene::LightType::Directional) {
    
                        properties.range = static_cast<float>(khr_light.Get("range").Get<double>());
    
                        if(type == scene::LightType::Spot) {
                            if(!khr_light.Has("spot")) {
                                LOGE("KHR_lights_punctual extension: spot light doesn't have a 'spot' property");
                                throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
                            }
    
                            properties.inner_cone_angle = static_cast<float>(khr_light.Get("spot").Get("innerConeAngle").Get<double>());
    
                            if(khr_light.Get("spot").Has("outerConeAngle")) {
                                properties.outer_cone_angle = static_cast<float>(khr_light.Get("spot").Get("outerConeAngle").Get<double>());
                            }
                            else {
                                properties.outer_cone_angle = glm::pi<float>() / 4.0f;
                            }
                        }
                    }
                    else if(type == scene::LightType::Directional || type == scene::LightType::Spot) {
                        properties.direction = glm::vec3(0.0f, 0.0f, -1.0f);
                    }
    
                    light->setLightType(type);
                    light->setProperties(properties);
                    light_components[light_index] = std::move(light);
                }
    
                return light_components;
            }
    
            return {};
        }
    
        bool GltfLoader::isExtensionEnabled(const std::string& requested_extension) {
            auto it = m_supported_extensions.find(requested_extension);
            return (it != m_supported_extensions.end()) ? it->second : false;
        }
    
        tinygltf::Value* GltfLoader::getExtension(tinygltf::ExtensionMap& tinygltf_extensions, const std::string& extension) {
            auto it = tinygltf_extensions.find(extension);
            return (it != tinygltf_extensions.end()) ? &it->second : nullptr;
        }
    }
}
