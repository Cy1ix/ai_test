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

#include "filesystem/filesystem.h"

#include <cassert>
#include <fstream>
#include <stdexcept>

#include "platform/platform_context.h"

#ifdef ENABLE_IMAGE_WRITE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif

namespace frame {
    namespace filesystem {

        static StdFileSystemPtr fs = nullptr;

        void FileSystem::writeFile(const Path& path, const std::string& data) {
            writeFile(path, std::vector<uint8_t>(data.begin(), data.end()));
        }

        std::string FileSystem::readFileString(const Path& path) {
            auto bin = readFileBinary(path);
            return { bin.begin(), bin.end() };
        }

        std::vector<uint8_t> FileSystem::readFileBinary(const Path& path) {
            auto stat = statFile(path);
            return readChunk(path, 0, stat.size);
        }

        StdFileSystem::StdFileSystem(Path external_storage_directory, Path temp_directory) :
            m_external_storage_directory(std::move(external_storage_directory)),
            m_temp_directory(std::move(temp_directory)) {
        }

        FileStat StdFileSystem::statFile(const Path& path) {

            std::error_code ec;

            auto fs_stat = std::filesystem::status(path, ec);
            if (ec) {
                return FileStat{
                    false,
                    false,
                    0,
                };
            }

            auto size = std::filesystem::file_size(path, ec);
            if (ec) {
                size = 0;
            }

            return FileStat{
                fs_stat.type() == std::filesystem::file_type::regular,
                fs_stat.type() == std::filesystem::file_type::directory,
                size,
            };
        }

        bool StdFileSystem::isFile(const Path& path) {
            auto stat = statFile(path);
            return stat.is_file;
        }

        bool StdFileSystem::isDirectory(const Path& path) {
            auto stat = statFile(path);
            return stat.is_directory;
        }

        bool StdFileSystem::exists(const Path& path) {
            auto stat = statFile(path);
            return stat.is_file || stat.is_directory;
        }

        bool StdFileSystem::createDirectory(const Path& path) {
            std::error_code ec;

            std::filesystem::create_directories(path, ec);

            if (ec) {
                throw std::runtime_error("[Filesystem] ERROR: Failed to create directory at path: " + path.string());
            }

            return !ec;
        }

        std::vector<uint8_t> StdFileSystem::readChunk(const Path& path, size_t offset, size_t count) {
            std::ifstream file{ path, std::ios::binary | std::ios::ate };

            if (!file.is_open()) {
                throw std::runtime_error("[Filesystem] ERROR: Failed to open file for reading at path: " + path.string());
            }

            auto size = statFile(path).size;

            if (offset + count > size) {
                return {};
            }

            file.seekg(offset, std::ios::beg);
            std::vector<uint8_t> data(count);
            file.read(reinterpret_cast<char*>(data.data()), count);

            return data;
        }

        void StdFileSystem::writeFile(const Path& path, const std::vector<uint8_t>& data) {
            auto parent = path.parent_path();
            if (!std::filesystem::exists(parent)) {
                createDirectory(parent);
            }

            std::ofstream file{ path, std::ios::binary | std::ios::trunc };

            if (!file.is_open()) {
                throw std::runtime_error("[Filesystem] ERROR: Failed to open file for writing at path: " + path.string());
            }

            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        void StdFileSystem::remove(const Path& path) {
            std::error_code ec;

            std::filesystem::remove_all(path, ec);

            if (ec) {
                throw std::runtime_error("[Filesystem] ERROR: Failed to remove file at path: " + path.string());
            }
        }

        void StdFileSystem::setExternalStorageDirectory(const std::string& dir) {
            m_external_storage_directory = dir;
        }

        const Path& StdFileSystem::externalStorageDirectory() const {
            return m_external_storage_directory;
        }

        const Path& StdFileSystem::tempDirectory() const {
            return m_temp_directory;
        }

        std::vector<std::string> StdFileSystem::listFilesRecursive(const std::string& directory) {
            std::vector<std::string> files;

            std::string path = GLSL_SHADER_DIR + directory;

            if (!isDirectory(path)) {
                throw std::runtime_error("[Filesystem] ERROR: Directory does not exist at path: " + directory);
            }

            try {
                std::filesystem::path base_path(directory);
                for (const auto& entry : std::filesystem::recursive_directory_iterator(base_path)) {
                    if (entry.is_regular_file()) {
                        std::filesystem::path relative_path = std::filesystem::relative(entry.path(), base_path);
                        files.push_back(relative_path.string());
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                throw std::runtime_error("[Filesystem] ERROR: Failed to list files in directory: " + directory +
                    " - " + e.what());
            }

            return files;
        }

        std::vector<std::string> StdFileSystem::listFilesPath(const std::string& directory) {

            std::vector<std::string> files;
            try {
                std::filesystem::path base_path(directory);
                for (const auto& entry : std::filesystem::recursive_directory_iterator(base_path)) {
                    if (entry.is_regular_file()) {
                        std::filesystem::path relative_path = std::filesystem::relative(entry.path(), base_path);
                        files.push_back(relative_path.generic_string());
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                throw std::runtime_error("[Filesystem] ERROR: Failed to list files in directory: " + directory +
                    " - " + e.what());
            }
            return files;
        }

        void init() {
            if(!fs) {
	            fs = std::make_shared<StdFileSystem>();
            }
        }

        void initWithContext(const platform::PlatformContext& context) {
            fs = std::make_shared<StdFileSystem>(context.getExternalStorageDirectory(), context.getTempDirectory());
        }

        StdFileSystemPtr get() {
            assert(fs && "[Filesystem] ASSERT: Filesystem not initialized");
            return fs;
        }

        std::string filename(const std::string& path) {
            return std::filesystem::path(path).filename().string();
        }

        namespace path {
            const std::string get(const Type type, const std::string& file) {
                assert(relative_paths.size() == Type::TotalRelativePathTypes &&
                    "[Filesystem] ASSERT: Not all paths are defined in filesystem, please check that each enum is specified");
                
                if (type == Type::Temp) {
                    return filesystem::get()->tempDirectory().string();
                }
                
                auto it = relative_paths.find(type);

                if (relative_paths.size() < Type::TotalRelativePathTypes) {
                    throw std::runtime_error("[Filesystem] ERROR: Platform hasn't initialized the paths correctly");
                }
                else if (it == relative_paths.end()) {
                    throw std::runtime_error("[Filesystem] ERROR: Path enum doesn't exist, or wasn't specified in the path map");
                }
                else if (it->second.empty()) {
                    throw std::runtime_error("[Filesystem] ERROR: Path was found, but it is empty");
                }

                auto fsys = filesystem::get();
                auto path = fsys->externalStorageDirectory() / it->second;

                if (!isDirectory(path.string())) {
                    createPath(fsys->externalStorageDirectory().string(), it->second);
                }

                if(file.empty()) {
                    return path.string();
                }
                else {
                    auto full_path = path / file;
                    return full_path.string();
                }
            }
        }

        bool isDirectory(const std::string& path) {
            return filesystem::get()->isDirectory(path);
        }

        bool isFile(const std::string& filename) {
            return filesystem::get()->isFile(filename);
        }

        void createDirectory(const std::string& path) {
            filesystem::get()->createDirectory(path);
        }

        void createPath(const std::string& root, const std::string& path) {
            std::filesystem::path full_path = std::filesystem::path(root) / path;

            if (!filesystem::get()->createDirectory(full_path))
            {
                throw std::runtime_error("[Filesystem] ERROR: Failed to create directory: " + full_path.string());
            }
        }

        std::vector<uint8_t> readAsset(const std::string& filename) {
            return filesystem::get()->readFileBinary(path::get(path::Type::Assets) + filename);
        }

        std::vector<uint8_t> readTexture(const std::string& filename) {
            return filesystem::get()->readFileBinary(path::get(path::Type::Textures) + filename);
        }

        std::string readShader(const std::string& filename) {
            return filesystem::get()->readFileString(path::get(path::Type::Shaders) + filename);
        }

        std::vector<uint8_t> readShaderBinary(const std::string& filename) {
            return filesystem::get()->readFileBinary(path::get(path::Type::Shaders) + filename);
        }

        std::vector<uint8_t> readTemp(const std::string& filename) {
            return filesystem::get()->readFileBinary(path::get(path::Type::Temp) + filename);
        }

        void writeTemp(const std::vector<uint8_t>& data, const std::string& filename) {
            filesystem::get()->writeFile(path::get(path::Type::Temp) + filename, data);
        }

        void writeImage(const uint8_t* data, const std::string& filename, const uint32_t width,
            const uint32_t height, const uint32_t components, const uint32_t row_stride)
        {
#ifdef ENABLE_IMAGE_WRITE
            stbi_write_png((path::get(Type::Screenshots) + filename + ".png").c_str(),
                width, height, components, data, row_stride);
#else
            throw std::runtime_error("[Filesystem] ERROR: ImageCPP writing support not enabled");
#endif
        }
    }
}
