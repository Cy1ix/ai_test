/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2024, Thomas Atkinson
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

#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace frame {
	namespace platform {
        class PlatformContext;
	}
    namespace filesystem {

        namespace path {
            enum Type {
                Assets,
                Textures,
                Shaders,
                Storage,
                Screenshots,
                Logs,
                TotalRelativePathTypes,

                ExternalStorage,
                Temp
            };

            static const std::unordered_map<Type, std::string> relative_paths = {
                {Type::Assets, ASSETS_DIR},
                {Type::Textures, TEXTURE_DIR},
                {Type::Shaders, GLSL_SHADER_DIR},
                {Type::Storage, OUTPUT_DIR},
                {Type::Screenshots, OUTPUT_DIR + std::string("images/")},
                {Type::Logs, OUTPUT_DIR + std::string("logs/")},
            };

            const std::string get(const Type type, const std::string& file = "");
        }

        struct FileStat {
            bool   is_file;
            bool   is_directory;
            size_t size;
        };

        using Path = std::filesystem::path;

        bool isDirectory(const std::string& path);
        bool isFile(const std::string& filename);
        void createDirectory(const std::string& path);
        void createPath(const std::string& root, const std::string& path);
        std::vector<uint8_t> readAsset(const std::string& filename);
        std::vector<uint8_t> readTexture(const std::string& filename);
        std::string readShader(const std::string& filename);
        std::vector<uint8_t> readShaderBinary(const std::string& filename);
        std::vector<uint8_t> readTemp(const std::string& filename);
        void writeTemp(const std::vector<uint8_t>& data, const std::string& filename);
        void writeImage(const uint8_t* data, const std::string& filename, const uint32_t width,
            const uint32_t height, const uint32_t components, const uint32_t row_stride);

        class FileSystem {
        public:
            FileSystem() = default;
            virtual ~FileSystem() = default;

            virtual FileStat statFile(const Path& path) = 0;
            virtual bool isFile(const Path& path) = 0;
            virtual bool isDirectory(const Path& path) = 0;
            virtual bool exists(const Path& path) = 0;
            virtual bool createDirectory(const Path& path) = 0;
            virtual std::vector<uint8_t> readChunk(const Path& path, size_t offset, size_t count) = 0;
            virtual void writeFile(const Path& path, const std::vector<uint8_t>& data) = 0;
            virtual void remove(const Path& path) = 0;

            virtual void setExternalStorageDirectory(const std::string& dir) = 0;
            virtual const Path& externalStorageDirectory() const = 0;
            virtual const Path& tempDirectory() const = 0;

            void writeFile(const Path& path, const std::string& data);
            std::string readFileString(const Path& path);
            std::vector<uint8_t> readFileBinary(const Path& path);

        };
        
        class StdFileSystem final : public FileSystem {
        public:
            StdFileSystem(Path external_storage_directory = std::filesystem::current_path().string(),
                Path temp_directory = std::filesystem::temp_directory_path().string());

            ~StdFileSystem() override = default;

            FileStat statFile(const Path& path) override;
            bool isFile(const Path& path) override;
            bool isDirectory(const Path& path) override;
            bool exists(const Path& path) override;
            bool createDirectory(const Path& path) override;
            std::vector<uint8_t> readChunk(const Path& path, size_t offset, size_t count) override;
            void writeFile(const Path& path, const std::vector<uint8_t>& data) override;
            void remove(const Path& path) override;
            void setExternalStorageDirectory(const std::string& dir) override;
            const Path& externalStorageDirectory() const override;
            const Path& tempDirectory() const override;

            std::vector<std::string> listFilesRecursive(const std::string& directory);
            std::vector<std::string> listFilesPath(const std::string& directory);
        private:
            Path m_external_storage_directory;
            Path m_temp_directory;
        };

        using StdFileSystemPtr = std::shared_ptr<StdFileSystem>;

        void init();

        void initWithContext(const platform::PlatformContext& context);

        StdFileSystemPtr get();

    	std::string filename(const std::string& path);
    }
}