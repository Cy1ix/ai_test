/* Copyright (c) 2023, Thomas Atkinson
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
#include <string>
#include <vector>
#include <Windows.h>

namespace frame {
    namespace platform {
        class PlatformContext {
        public:
            virtual ~PlatformContext() = default;

            virtual const std::vector<std::string>& getArguments() const {
                return m_arguments;
            }

            virtual const std::string& getExternalStorageDirectory() const {
                return m_external_storage_directory;
            }

            virtual const std::string& getTempDirectory() const {
                return m_temp_directory;
            }

        protected:
            std::vector<std::string> m_arguments;
            std::string m_external_storage_directory;
            std::string m_temp_directory;

            PlatformContext() = default;
        };

        class WindowsPlatformContext final : public PlatformContext {
        public:
            WindowsPlatformContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow);
            ~WindowsPlatformContext() override {
                FreeConsole();
            }
        };
    }
}
