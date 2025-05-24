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

#include "platform/application.h"

#include "glslang/Public/ShaderLang.h"
#include "utils/logger.h"

namespace frame {
    namespace platform {
        Application::Application() :
            m_name{ "Sample Name" }
        {
        }

        bool Application::prepare(const ApplicationOptions& options) {
            assert(options.window != nullptr && "[Application] ASSERT: Window must be valid");

            m_lock_simulation_speed = options.benchmark_enabled;
            m_window = options.window;

            return true;
        }

        void Application::finish() {
            glslang::FinalizeProcess();
        }

        bool Application::resize(const uint32_t /*width*/, const uint32_t /*height*/) {
            return true;
        }

        void Application::inputEvent(const InputEvent&/*input_event*/) {}

        ImguiDrawer* Application::getDrawer() {
            return nullptr;
        }

        void Application::update(float delta_time) {
            m_fps = 1.0f / delta_time;
            m_frame_time = delta_time * 1000.0f;
        }

        void Application::updateOverlay(float /*delta_time*/, const std::function<void()>&/*additional_ui*/) {}

        const std::string& Application::getName() const {
            return m_name;
        }

        void Application::setName(const std::string& name) {
            m_name = name;
        }

        common::DebugInfo& Application::getDebugInfo() {
            return m_debug_info;
        }

        void Application::changeShader(const common::ShaderSourceLanguage&/*shader_language*/) {
            LOGE("Not implemented by sample");
        }

        const std::map<common::ShaderSourceLanguage, std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>>& Application::getAvailableShaders() const {
            return m_available_shaders;
        }

        void Application::storeShaders(const common::ShaderSourceLanguage& shader_language, const std::vector<std::pair<vk::ShaderStageFlagBits, std::string>>& list_of_shaders) {
            m_available_shaders.insert({ shader_language, list_of_shaders });
        }
    }
}
