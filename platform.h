/* Copyright (c) 2019-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "platform/platform_context.h"
#include "common/common.h"
#include "platform/application.h"
#include "platform/window.h"
#include "rendering/render_context.h"

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#	undef Success
#endif

namespace frame {
    namespace platform {

        class PlatformContext;
        class InputEvent;
        class Application;
        class Window;

        enum class ExitCode {
            Success = 0,
            Close,
            NoApplication,
            FatalError
        };
        
        class Platform {
        public:
            Platform(const PlatformContext& context);

            virtual ~Platform() = default;
            
            ExitCode initialize();
            
            ExitCode mainLoop();
            
            ExitCode mainLoopFrame();
            
            void update();
            
            void terminate(ExitCode code);
            
            void close();
            
            std::string& getLastError();
            
            void resize(uint32_t width, uint32_t height);
            
            void inputEvent(const InputEvent& input_event);
            
            Window& getWindow();
            
            Application& getApp();
            
            Application& getApp() const;
            
            void setLastError(const std::string& error);
            
            void setFocus(bool focused);
            
            bool startApplication(std::unique_ptr<Application> app);
            
            void forceSimulationFps(float fps);
            
            void forceRender(bool should_always_render);
            
            void disableInputProcessing();
            
            void setWindowProperties(const Window::OptionalProperties& properties);
            
            static const uint32_t MIN_WINDOW_WIDTH;
            static const uint32_t MIN_WINDOW_HEIGHT;

        private:
            std::unique_ptr<Window> m_window{ nullptr };
            std::unique_ptr<Application> m_active_app{ nullptr };

            Window::Properties m_window_properties;
            bool m_fixed_simulation_fps{ false };
            bool m_always_render{ false };
            float m_simulation_frame_time{ 0.016f };
            bool m_process_input_events{ true };
            bool m_focused{ true };
            bool m_close_requested{ false };

            Timer m_timer;
            std::vector<std::string> m_arguments;
            std::string m_lastError;
        };
    }
}
