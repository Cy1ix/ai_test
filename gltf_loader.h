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

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tinygltf/tiny_gltf.h>

#include <vulkan/vulkan.hpp>

#include "glm/glm.hpp"

#define KHR_LIGHTS_PUNCTUAL_EXTENSION "KHR_lights_punctual"
#define KHR_MATERIALS_UNLIT_EXTENSION          "KHR_materials_unlit"
#define KHR_TEXTURE_TRANSFORM_EXTENSION        "KHR_texture_transform"
#define KHR_MATERIALS_PBR_SPECULAR_GLOSSINESS  "KHR_materials_pbrSpecularGlossiness"
#define EXT_TEXTURE_WEBP_EXTENSION             "EXT_texture_webp"
#define KHR_DRACO_MESH_COMPRESSION_EXTENSION   "KHR_draco_mesh_compression"
#define KHR_TECHNIQUES_WEBGL_EXTENSION         "KHR_techniques_webgl"
#define KHR_MESH_QUANTIZATION_EXTENSION        "KHR_mesh_quantization"
#define KHR_TEXTURE_BASISU_EXTENSION           "KHR_texture_basisu"

namespace frame {
    namespace core {
	    class Device;
    }

    namespace scene {
        class Camera;
        class Image;
        class Light;
        class Mesh;
        class Node;
        class PBRMaterial;
        class Sampler;
        class Scene;
        class SubMesh;
        class Texture;
    }

    namespace common {
        struct Vertex {
            glm::vec4 pos;
            glm::vec3 normal;
            glm::vec2 uv;
            glm::vec4 color;
            glm::vec4 joint0;
            glm::vec4 weight0;
        };

        struct AlignedVertex
        {
            glm::vec4 pos;
            glm::vec4 normal;
        };

        struct Meshlet
        {
            uint32_t vertices[64];
            uint32_t indices[126];
            uint32_t vertex_count;
            uint32_t index_count;
        };

        template <class T, class Y>
        struct TypeCast {
            Y operator()(T value) const noexcept {
                return static_cast<Y>(value);
            }
        };

        class GltfLoader {
        public:
            GltfLoader(core::Device& device);
            virtual ~GltfLoader() = default;

            std::unique_ptr<scene::Scene> readSceneFromFile(
                const std::string& file_name,
                int scene_index = -1,
                vk::BufferUsageFlags additional_buffer_usage_flags = {});

            std::unique_ptr<scene::SubMesh> readModelFromFile(
                const std::string& file_name,
                uint32_t index,
                bool storage_buffer = false,
                vk::BufferUsageFlags additional_buffer_usage_flags = {});

        protected:
            virtual std::unique_ptr<scene::Node> parseNode(const tinygltf::Node& gltf_node, size_t index) const;
            virtual std::unique_ptr<scene::Camera> parseCamera(const tinygltf::Camera& gltf_camera) const;
            virtual std::unique_ptr<scene::Mesh> parseMesh(const tinygltf::Mesh& gltf_mesh) const;
            virtual std::unique_ptr<scene::PBRMaterial> parseMaterial(const tinygltf::Material& gltf_material) const;
            virtual std::unique_ptr<scene::Image> parseImage(tinygltf::Image& gltf_image) const;
            virtual std::unique_ptr<scene::Sampler> parseSampler(const tinygltf::Sampler& gltf_sampler) const;
            virtual std::unique_ptr<scene::Texture> parseTexture(const tinygltf::Texture& gltf_texture) const;

            virtual std::unique_ptr<scene::PBRMaterial> createDefaultMaterial();
            virtual std::unique_ptr<scene::Sampler> createDefaultSampler(int filter);
            virtual std::unique_ptr<scene::Camera> createDefaultCamera();

            std::vector<std::unique_ptr<scene::Light>> parseKhrLightsPunctual();
            bool isExtensionEnabled(const std::string& requested_extension);
            tinygltf::Value* getExtension(tinygltf::ExtensionMap& tinygltf_extensions, const std::string& extension);

            core::Device& m_device;
            tinygltf::Model m_model;
            std::string m_model_path;

            static std::unordered_map<std::string, bool> m_supported_extensions;

        private:
            scene::Scene loadScene(int scene_index = -1, vk::BufferUsageFlags additional_buffer_usage_flags = {});
            std::unique_ptr<scene::SubMesh> loadModel(uint32_t index, bool storage_buffer = false, vk::BufferUsageFlags additional_buffer_usage_flags = {});
        };
    }
}
