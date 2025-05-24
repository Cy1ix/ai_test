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

#include "platform/platform.h"
#include "platform/window.h"
#include "utils/logger.h"
#include "common/glsl_compiler.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <mutex>
#include <vector>
#include <Windows.h>
#include <shellapi.h>
#include <stdexcept>

namespace frame {
    namespace platform {

        const uint32_t Platform::MIN_WINDOW_WIDTH = 420;
        const uint32_t Platform::MIN_WINDOW_HEIGHT = 320;

        Platform::Platform(const PlatformContext& context) :
            m_arguments(context.getArguments())
        {
        }

        ExitCode Platform::initialize() {
            auto sinks = std::vector<spdlog::sink_ptr>{
                std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
            };

            auto logger = logger::Logger::getInstance().init("VK_LOGGER", "logs/app.log");

#ifdef VK_DEBUG
            logger::Logger::getInstance().setLevel(logger::LogLevel::DEBUG);
#else
            logger::Logger::getInstance().setLevel(logger::LogLevel::INFO);
#endif

            LOGI("Logger initialized");
            /*
            if (m_arguments.empty()) {
                return ExitCode::NoApplication;
            }
            */
            if (m_close_requested) {
                return ExitCode::Close;
            }
            
            if (!m_window) {
                m_window = Window::Create(this, m_window_properties);

                if (!m_window) {
                    LOGE("Window creation failed!");
                    return ExitCode::FatalError;
                }
            }

            return ExitCode::Success;
        }

        ExitCode Platform::mainLoopFrame() {
            try {
                if (!m_active_app) {
                    return ExitCode::NoApplication;
                }

                update();

                if (m_active_app->shouldClose()) {
                    m_active_app->finish();
                }

                m_window->processEvents();

                if (m_window->shouldClose() || m_close_requested) {
                    return ExitCode::Close;
                }
            }
            catch (std::exception& e) {
                LOGE("[Platform] ERROR: Error Message: {}", e.what());
                LOGE("[Platform] ERROR: Failed when running application {}", m_active_app->getName());

                setLastError(e.what());
                return ExitCode::FatalError;
            }

            return ExitCode::Success;
        }

        ExitCode Platform::mainLoop() {
            ExitCode exit_code = ExitCode::Success;
            while (exit_code == ExitCode::Success) {
                exit_code = mainLoopFrame();
            }

            return exit_code;
        }

        void Platform::update() {
            auto delta_time = static_cast<float>(m_timer.tick<Timer::Seconds>());

            if (m_focused || m_always_render) {
                if (m_fixed_simulation_fps) {
                    delta_time = m_simulation_frame_time;
                }

                m_active_app->updateOverlay(delta_time, [=]() {});
                m_active_app->update(delta_time);
            }
        }

        void Platform::terminate(ExitCode code) {
            if (m_active_app) {
                m_active_app->finish();
            }

            m_active_app.reset();
            m_window.reset();

            spdlog::drop_all();

            if (code != ExitCode::Success) {
                std::cout << "Press return to continue";
                std::cin.get();
            }
        }

        void Platform::close() {
            if (m_window) {
                m_window->close();
            }

            m_close_requested = true;
        }

        void Platform::forceSimulationFps(float fps) {
            m_fixed_simulation_fps = true;
            m_simulation_frame_time = 1 / fps;
        }

        void Platform::forceRender(bool should_always_render) {
            m_always_render = should_always_render;
        }

        void Platform::disableInputProcessing() {
            m_process_input_events = false;
        }

        void Platform::setFocus(bool focused) {
            m_focused = focused;
        }

        void Platform::setWindowProperties(const Window::OptionalProperties& properties) {
            if (properties.title) {
                m_window_properties.title = properties.title.value();
            }
            if (properties.mode) {
                m_window_properties.mode = properties.mode.value();
            }
            if (properties.resizable) {
                m_window_properties.resizable = properties.resizable.value();
            }
            if (properties.vsync) {
                m_window_properties.vsync = properties.vsync.value();
            }
            if (properties.extent.width) {
                m_window_properties.extent.width = properties.extent.width.value();
            }
            if (properties.extent.height) {
                m_window_properties.extent.height = properties.extent.height.value();
            }
        }

        std::string& Platform::getLastError() {
            return m_lastError;
        }

        Application& Platform::getApp() {
            assert(m_active_app && "Application is not valid");
            return *m_active_app;
        }

        Application& Platform::getApp() const {
            assert(m_active_app && "Application is not valid");
            return *m_active_app;
        }

        Window& Platform::getWindow() {
            assert(m_window && "Window is not valid");
            return *m_window;
        }

        void Platform::setLastError(const std::string& error) {
            m_lastError = error;
        }

        bool Platform::startApplication(std::unique_ptr<Application> app) {
            if (m_active_app) {
                auto execution_time = m_timer.stop();
                LOGI("Closing App (Runtime: {:.1f})", execution_time);
                m_active_app->finish();
            }

            m_active_app = std::move(app);

            if (!m_active_app) {
                LOGE("Failed to create a valid vulkan app.");
                return false;
            }

            ApplicationOptions app_options;
            app_options.benchmark_enabled = false;
            app_options.window = m_window.get();

            if (!m_active_app->prepare(app_options)) {
                LOGE("Failed to prepare vulkan app.");
                return false;
            }

            return true;
        }

        void Platform::inputEvent(const InputEvent& input_event) {
            if (m_process_input_events && m_active_app) {
                m_active_app->inputEvent(input_event);
            }

            if (input_event.getSource() == EventSource::Keyboard) {
                const auto& key_event = static_cast<const KeyInputEvent&>(input_event);

                if (key_event.getCode() == KeyCode::Back ||
                    key_event.getCode() == KeyCode::Escape)
                {
                    close();
                }
            }
        }

        void Platform::resize(uint32_t width, uint32_t height) {
            auto extent = Window::Extent{
                std::max<uint32_t>(width, MIN_WINDOW_WIDTH),
                std::max<uint32_t>(height, MIN_WINDOW_HEIGHT)
            };

            if ((m_window) && (width > 0) && (height > 0)) {
                auto actual_extent = m_window->resize(extent);

                if (m_active_app) {
                    m_active_app->resize(actual_extent.width, actual_extent.height);
                }
            }
        }
    }
}