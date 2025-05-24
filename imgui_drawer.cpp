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

#include "platform/imgui_drawer.h"

namespace frame {
    namespace platform {
        void ImguiDrawer::clear() {
            m_dirty = false;
        }

        bool ImguiDrawer::isDirty() {
            return m_dirty;
        }

        void ImguiDrawer::setDirty(bool dirty) {
            m_dirty = dirty;
        }

        bool ImguiDrawer::header(const char* caption) {
            return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
        }

        bool ImguiDrawer::checkbox(const char* caption, bool* value) {
            bool res = ImGui::Checkbox(caption, value);
            if (res)
            {
                m_dirty = true;
            }
            return res;
        }

        bool ImguiDrawer::checkbox(const char* caption, int32_t* value) {
            bool val = (*value == 1);
            bool res = ImGui::Checkbox(caption, &val);
            *value = val;
            if (res) {
                m_dirty = true;
            }
            return res;
        }

        bool ImguiDrawer::radioButton(const char* caption, int32_t* selected_option, const int32_t element_option) {
            bool res = ImGui::RadioButton(caption, selected_option, element_option);
            if (res) {
                m_dirty = true;
            }
            return res;
        }

        bool ImguiDrawer::inputFloat(const char* caption, float* value, float step, const char* precision) {
            bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
            if (res) {
                m_dirty = true;
            }
            return res;
        }

        bool ImguiDrawer::sliderFloat(const char* caption, float* value, float min, float max) {
            bool res = ImGui::SliderFloat(caption, value, min, max);
            if (res) {
                m_dirty = true;
            }
            return res;
        }

        bool ImguiDrawer::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max) {
            bool res = ImGui::SliderInt(caption, value, min, max);
            if (res) {
                m_dirty = true;
            }
            return res;
        }

        bool ImguiDrawer::comboBox(const char* caption, int32_t* item_index, std::vector<std::string> items) {
            if (items.empty()) {
                return false;
            }

            std::vector<const char*> char_items;
            char_items.reserve(items.size());

            for (const auto& item : items) {
                char_items.push_back(item.c_str());
            }

            uint32_t item_count = static_cast<uint32_t>(char_items.size());
            bool res = ImGui::Combo(caption, item_index, &char_items[0], item_count, item_count);

            if (res) {
                m_dirty = true;
            }

            return res;
        }

        bool ImguiDrawer::button(const char* caption) {
            bool res = ImGui::Button(caption);
            if (res) {
                m_dirty = true;
            }
            return res;
        }

        void ImguiDrawer::text(const char* format_str, ...) {
            va_list args;
            va_start(args, format_str);
            ImGui::TextV(format_str, args);
            va_end(args);
        }

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Edit, 3>(const char* caption, float* colors, ImGuiColorEditFlags flags) {
            return ImGui::ColorEdit3(caption, colors, flags);
        }

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Edit, 4>(const char* caption, float* colors, ImGuiColorEditFlags flags) {
            return ImGui::ColorEdit4(caption, colors, flags);
        }

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Pick, 3>(const char* caption, float* colors, ImGuiColorEditFlags flags) {
            return ImGui::ColorPicker3(caption, colors, flags);
        }

        template <>
        bool ImguiDrawer::colorOpImpl<ImguiDrawer::ColorOp::Pick, 4>(const char* caption, float* colors, ImGuiColorEditFlags flags) {
            return ImGui::ColorPicker4(caption, colors, flags);
        }
    }
}
