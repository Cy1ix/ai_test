/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "platform/input.h"

namespace frame {
    namespace platform {
        InputEvent::InputEvent(EventSource source) :
            m_source{ source }
        {}

        EventSource InputEvent::getSource() const {
            return m_source;
        }

        KeyInputEvent::KeyInputEvent(KeyCode code, KeyAction action) :
            InputEvent{ EventSource::Keyboard },
            m_code{ code },
            m_action{ action }
        {}

        KeyCode KeyInputEvent::getCode() const {
            return m_code;
        }

        KeyAction KeyInputEvent::getAction() const {
            return m_action;
        }

        MouseButtonInputEvent::MouseButtonInputEvent(MouseButton button, MouseAction action, float pos_x, float pos_y) :
            InputEvent{ EventSource::Mouse },
            m_button{ button },
            m_action{ action },
            m_pos_x{ pos_x },
            m_pos_y{ pos_y }
        {}

        MouseButton MouseButtonInputEvent::getButton() const {
            return m_button;
        }

        MouseAction MouseButtonInputEvent::getAction() const {
            return m_action;
        }

        float MouseButtonInputEvent::getPosX() const {
            return m_pos_x;
        }

        float MouseButtonInputEvent::getPosY() const {
            return m_pos_y;
        }

        TouchInputEvent::TouchInputEvent(int32_t pointer_id, std::size_t touch_points, TouchAction action, float pos_x, float pos_y) :
            InputEvent{ EventSource::Touchscreen },
            m_action{ action },
            m_pointer_id{ pointer_id },
            m_touch_points{ touch_points },
            m_pos_x{ pos_x },
            m_pos_y{ pos_y }
        {}

        TouchAction TouchInputEvent::getAction() const {
            return m_action;
        }

        int32_t TouchInputEvent::getPointerId() const {
            return m_pointer_id;
        }

        std::size_t TouchInputEvent::getTouchPoints() const {
            return m_touch_points;
        }

        float TouchInputEvent::getPosX() const {
            return m_pos_x;
        }

        float TouchInputEvent::getPosY() const {
            return m_pos_y;
        }

        ControllerButtonEvent::ControllerButtonEvent(uint32_t controller_id, ControllerButton button, ControllerAction action) :
            InputEvent{ EventSource::Controller },
            m_controller_id{ controller_id },
            m_button{ button },
            m_action{ action }
        {}

        uint32_t ControllerButtonEvent::getControllerId() const {
            return m_controller_id;
        }

        ControllerButton ControllerButtonEvent::getButton() const {
            return m_button;
        }

        ControllerAction ControllerButtonEvent::getAction() const {
            return m_action;
        }

        ControllerAxisEvent::ControllerAxisEvent(uint32_t controller_id, ControllerAxis axis, float value) :
            InputEvent{ EventSource::Controller },
            m_controller_id{ controller_id },
            m_axis{ axis },
            m_value{ value }
        {}

        uint32_t ControllerAxisEvent::getControllerId() const {
            return m_controller_id;
        }

        ControllerAxis ControllerAxisEvent::getAxis() const {
            return m_axis;
        }

        float ControllerAxisEvent::getValue() const {
            return m_value;
        }

        ControllerBatteryEvent::ControllerBatteryEvent(uint32_t controller_id, ControllerBatteryStatus status, ControllerBatteryLevel level, float percentage) :
            InputEvent{ EventSource::Controller },
            m_controller_id{ controller_id },
            m_status{ status },
            m_level{ level },
            m_percentage{ percentage }
        {}

        uint32_t ControllerBatteryEvent::getControllerId() const {
            return m_controller_id;
        }

        ControllerBatteryStatus ControllerBatteryEvent::getStatus() const {
            return m_status;
        }

        ControllerBatteryLevel ControllerBatteryEvent::getLevel() const {
            return m_level;
        }

        float ControllerBatteryEvent::getPercentage() const {
            return m_percentage;
        }

        ControllerMotionEvent::ControllerMotionEvent(uint32_t controller_id,
            float accel_x, float accel_y, float accel_z,
            float gyro_x, float gyro_y, float gyro_z) :
            InputEvent{ EventSource::Controller },
            m_controller_id{ controller_id },
            m_accel_x{ accel_x },
            m_accel_y{ accel_y },
            m_accel_z{ accel_z },
            m_gyro_x{ gyro_x },
            m_gyro_y{ gyro_y },
            m_gyro_z{ gyro_z }
        {}

        uint32_t ControllerMotionEvent::getControllerId() const {
            return m_controller_id;
        }

        float ControllerMotionEvent::getAccelX() const {
            return m_accel_x;
        }

        float ControllerMotionEvent::getAccelY() const {
            return m_accel_y;
        }

        float ControllerMotionEvent::getAccelZ() const {
            return m_accel_z;
        }

        float ControllerMotionEvent::getGyroX() const {
            return m_gyro_x;
        }

        float ControllerMotionEvent::getGyroY() const {
            return m_gyro_y;
        }

        float ControllerMotionEvent::getGyroZ() const {
            return m_gyro_z;
        }

        ControllerConnectionEvent::ControllerConnectionEvent(uint32_t controller_id, ControllerConnectionState state) :
            InputEvent{ EventSource::ControllerConnection },
            m_controller_id{ controller_id },
            m_state{ state }
        {}

        uint32_t ControllerConnectionEvent::getControllerId() const {
            return m_controller_id;
        }

        ControllerConnectionState ControllerConnectionEvent::getState() const {
            return m_state;
        }

        ControllerFeedback::ControllerFeedback(uint32_t controller_id) :
            m_controller_id{ controller_id }
        {}

        bool ControllerFeedback::setVibration(float left_motor, float right_motor, uint32_t duration_ms) {
            // TODO:Implement vibration
            return true;
        }

        bool ControllerFeedback::setTriggerFeedback(bool is_left_trigger, float resistance, float intensity) {
            // TODO:Implement trigger feedback, require hardware support
            return true;
        }

        void ControllerFeedback::stopVibration() {
            setVibration(0.0f, 0.0f, 0);
        }

        uint32_t ControllerFeedback::getControllerId() const {
            return m_controller_id;
        }
    }
}
