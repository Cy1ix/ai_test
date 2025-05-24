/* Copyright (c) 2018-2023, Arm Limited and Contributors
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

#include <optional>
#include "common/common.h"
#include "core/instance.h"

struct GLFWwindow;

namespace frame {
    namespace core {
        class Instance;
    }
    namespace platform {
        class Platform;
        
        class Window {
        public:
            enum class Backend {
                GLFW,
                WIN
            };
            
            struct Extent {
                uint32_t width;
                uint32_t height;
            };
            
            struct OptionalExtent {
                std::optional<uint32_t> width;
                std::optional<uint32_t> height;
            };
            
            enum class Mode {
                Fullscreen,
                FullscreenBorderless,
                Windowed,
                Default = Windowed
            };
            
            enum class Vsync {
                Off,
                On,
                Default = On
            };
            
            struct OptionalProperties {
                std::optional<std::string> title;
                std::optional<Mode> mode;
                std::optional<bool> resizable;
                std::optional<Vsync> vsync;
                OptionalExtent extent;
            };
            
            struct Properties {
                std::string title = "";
                Mode mode = Mode::Default;
                bool resizable = true;
                Vsync vsync = Vsync::Default;
                Extent extent = { 1280, 720 };
                Backend backend = Backend::GLFW;
            };

        public:
            
            static std::unique_ptr<Window> Create(
                Platform* platform,
                const Properties& properties = Properties{});
            
            virtual ~Window();

            vk::SurfaceKHR createSurface(core::Instance& instance);
            
            bool shouldClose() const;
            
            void processEvents();
            
            void close();
            
            float getDpiFactor() const;
            
            float getContentScaleFactor() const;
            
            Extent resize(const Extent& new_extent);
            
            bool getDisplayPresentInfo(VkDisplayPresentInfoKHR* info,
                uint32_t src_width, uint32_t src_height) const;
            
            std::vector<const char*> getRequiredSurfaceExtensions() const;
            
            const Extent& getExtent() const { return m_properties.extent; }
            
            Mode getWindowMode() const { return m_properties.mode; }
            
            const Properties& getProperties() const { return m_properties; }
            
            Platform* getPlatform() const { return m_platform; }
            
            GLFWwindow* getGLFWHandle() const { return m_glfw_handle; }

        private:
            
            Window(Platform* platform, const Properties& properties);

            bool initGLFW();

        private:
            Platform* m_platform;
            Properties m_properties;
            GLFWwindow* m_glfw_handle{ nullptr };
        };
    }
}
