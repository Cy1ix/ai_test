/* Copyright (c) 2024, Mobica Limited
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

#include <array>
#include <stdint.h>
#include <string>
#include <vector>
#include "imgui/imgui.h"

namespace frame {
    namespace platform {
        class ImguiDrawer {
        public:
            enum class ColorOp {
                Edit,
                Pick
            };

            ImguiDrawer() = default;

            void clear();

            bool isDirty();

            void setDirty(bool dirty);

            bool header(const char* caption);

            bool checkbox(const char* caption, bool* value);

            bool checkbox(const char* caption, int32_t* value);

            bool radioButton(const char* caption, int32_t* selected_option, const int32_t element_option);

            bool inputFloat(const char* caption, float* value, float step, const char* precision);

            bool sliderFloat(const char* caption, float* value, float min, float max);

            bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);

            bool comboBox(const char* caption, int32_t* item_index, std::vector<std::string> items);

            bool button(const char* caption);

            void text(const char* format_str, ...);

            bool colorPicker(const char* caption, std::array<float, 3>& color, float width = 0.0f, ImGuiColorEditFlags flags = 0) {
                return colorOp<ColorOp::Pick>(caption, color, width, flags);
            }

            bool colorPicker(const char* caption, std::array<float, 4>& color, float width = 0.0f, ImGuiColorEditFlags flags = 0) {
                return colorOp<ColorOp::Pick>(caption, color, width, flags);
            }

            bool colorEdit(const char* caption, std::array<float, 3>& color, float width = 0.0f, ImGuiColorEditFlags flags = 0) {
                return colorOp<ColorOp::Edit>(caption, color, width, flags);
            }

            bool colorEdit(const char* caption, std::array<float, 4>& color, float width = 0.0f, ImGuiColorEditFlags flags = 0) {
                return colorOp<ColorOp::Edit>(caption, color, width, flags);
            }

            template <ColorOp OP, size_t N>
            bool colorOp(const std::string& caption, std::array<float, N>& color, float width = 0.0f, ImGuiColorEditFlags flags = 0) {

                static_assert((N == 3) || (N == 4), "[ImguiDrawer] ASSERT: The channel count must be 3 or 4.");

                ImGui::PushItemWidth(width);
                bool res = colorOpImpl<OP, N>(caption.c_str(), color.data(), flags);
                ImGui::PopItemWidth();
                if (res)
                    m_dirty = true;
                return res;
            }

        private:
            template <ColorOp OP, size_t N>
            inline bool colorOpImpl(const char* caption, float* colors, ImGuiColorEditFlags flags) {
                assert(false);
                return false;
            }

            bool m_dirty{ false };
        };

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Edit, 3>(const char* caption, float* colors, ImGuiColorEditFlags flags);

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Edit, 4>(const char* caption, float* colors, ImGuiColorEditFlags flags);

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Pick, 3>(const char* caption, float* colors, ImGuiColorEditFlags flags);

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Pick, 4>(const char* caption, float* colors, ImGuiColorEditFlags flags);
    }
}
