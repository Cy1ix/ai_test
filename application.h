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

#include <functional>

#include "common/common.h"
#include "common/debug_info.h"
#include "platform/imgui_drawer.h"
#include "platform/input.h"
#include "platform/window.h"
#include "utils/timer.h"

namespace frame {
    namespace common {
        class DebugInfo;
    }
    namespace platform {

        class Window;
        class InputEvent;
        
        struct ApplicationOptions {
            bool benchmark_enabled{ false };
            Window* window{ nullptr };
        };

        class Application {
        public:
            Application();
            virtual ~Application() = default;
            
            virtual bool prepare(const ApplicationOptions& options);
            virtual void update(float delta_time);
            virtual void updateOverlay(float delta_time, const std::function<void()>& additional_ui = []() {});
            virtual void finish();
            virtual bool resize(const uint32_t width, const uint32_t height);
            virtual void inputEvent(const InputEvent& input_event);
            
            virtual ImguiDrawer* getDrawer();
            const std::string& getName() const;
            void setName(const std::string& name);
            common::DebugInfo& getDebugInfo();
            
            bool shouldClose() const {
                return m_requested_close;
            }
            
            void close() {
                m_requested_close = true;
            }
            
            virtual void changeShader(const common::ShaderSourceLanguage& shader_language);
            const std::map<common::ShaderSourceLanguage, std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>>& getAvailableShaders() const;
            static void setShadingLanguage(const common::ShadingLanguage language);
            static common::ShadingLanguage getShadingLanguage();

        protected:
            void storeShaders(const common::ShaderSourceLanguage& shader_language, const std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>& list_of_shaders);

            float m_fps{ 0.0f };
            float m_frame_time{ 0.0f };
            uint32_t m_frame_count{ 0 };
            uint32_t m_last_frame_count{ 0 };
            bool m_lock_simulation_speed{ false };
            Window* m_window{ nullptr };

        private:
            std::string m_name{};
            std::map<common::ShaderSourceLanguage, std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>> m_available_shaders;
            common::DebugInfo m_debug_info{};
            bool m_requested_close{ false };
            inline static common::ShadingLanguage m_shading_language{ common::ShadingLanguage::GLSL };
        };

        inline void Application::setShadingLanguage(const common::ShadingLanguage language) {
            m_shading_language = language;
        }

        inline common::ShadingLanguage Application::getShadingLanguage() {
            return m_shading_language;
        }
    }
}
