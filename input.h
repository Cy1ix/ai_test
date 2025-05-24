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

#pragma once

#include <cstddef>
#include <cstdint>

namespace frame {
	namespace platform {

		class Platform;

		enum class EventSource {
			Keyboard,
			Mouse,
			Touchscreen,
			Controller,
			ControllerConnection
		};

		class InputEvent {
		public:
			InputEvent(EventSource source);

			EventSource getSource() const;

		private:
			EventSource m_source;
		};

		enum class KeyCode {
			Unknown,
			Space,
			Apostrophe, /* ' */
			Comma,      /* , */
			Minus,      /* - */
			Period,     /* . */
			Slash,      /* / */
			_0,
			_1,
			_2,
			_3,
			_4,
			_5,
			_6,
			_7,
			_8,
			_9,
			Semicolon, /* ; */
			Equal,     /* = */
			A,
			B,
			C,
			D,
			E,
			F,
			G,
			H,
			I,
			J,
			K,
			L,
			M,
			N,
			O,
			P,
			Q,
			R,
			S,
			T,
			U,
			V,
			W,
			X,
			Y,
			Z,
			LeftBracket,  /* [ */
			Backslash,    /* \ */
			RightBracket, /* ] */
			GraveAccent,  /* ` */
			Escape,
			Enter,
			Tab,
			Backspace,
			Insert,
			DelKey,
			Right,
			Left,
			Down,
			Up,
			PageUp,
			PageDown,
			Home,
			End,
			Back,
			CapsLock,
			ScrollLock,
			NumLock,
			PrintScreen,
			Pause,
			F1,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			KP_0,
			KP_1,
			KP_2,
			KP_3,
			KP_4,
			KP_5,
			KP_6,
			KP_7,
			KP_8,
			KP_9,
			KP_Decimal,
			KP_Divide,
			KP_Multiply,
			KP_Subtract,
			KP_Add,
			KP_Enter,
			KP_Equal,
			LeftShift,
			LeftControl,
			LeftAlt,
			RightShift,
			RightControl,
			RightAlt
		};

		enum class KeyAction {
			Down,
			Up,
			Repeat,
			Unknown
		};

		class KeyInputEvent : public InputEvent {
		public:
			KeyInputEvent(KeyCode code, KeyAction action);

			KeyCode getCode() const;

			KeyAction getAction() const;

		private:
			KeyCode m_code;
			KeyAction m_action;
		};

		enum class MouseButton {
			Left,
			Right,
			Middle,
			Back,
			Forward,
			Unknown
		};

		enum class MouseAction {
			Down,
			Up,
			Move,
			Unknown
		};

		class MouseButtonInputEvent : public InputEvent {
		public:
			MouseButtonInputEvent(MouseButton button, MouseAction action, float pos_x, float pos_y);

			MouseButton getButton() const;

			MouseAction getAction() const;

			float getPosX() const;

			float getPosY() const;

		private:
			MouseButton m_button;
			MouseAction m_action;
			float m_pos_x;
			float m_pos_y;
		};

		enum class TouchAction {
			Down,
			Up,
			Move,
			Cancel,
			PointerDown,
			PointerUp,
			Unknown
		};

		class TouchInputEvent : public InputEvent {
		public:
			TouchInputEvent(int32_t pointer_id, size_t touch_points, TouchAction action, float pos_x, float pos_y);

			TouchAction getAction() const;

			int32_t getPointerId() const;

			size_t getTouchPoints() const;

			float getPosX() const;

			float getPosY() const;

		private:
			TouchAction m_action;
			int32_t m_pointer_id;
			size_t m_touch_points;
			float m_pos_x;
			float m_pos_y;
		};

		enum class ControllerButton {
			BUTTON_A,           // Xbox:A | Switch:B | PS:Cross
			BUTTON_B,           // Xbox:B | Switch:A | PS:Circle
			BUTTON_X,           // Xbox:X | Switch:Y | PS:Square
			BUTTON_Y,           // Xbox:Y | Switch:X | PS:Triangle

			BUMPER_LEFT,           // Xbox:LB | Switch:L | PS:L1
			BUMPER_RIGHT,          // Xbox:RB | Switch:R | PS:R1

			BUTTON_START,           // Xbox:START
			BUTTON_SELECT,          // Xbox:SELECT
			BUTTON_HOME,            // Xbox:Guide | PS:PS | Switch:Home

			STICK_LEFT_CLICK,
			STICK_RIGHT_CLICK,

			DPAD_UP,
			DPAD_DOWN,
			DPAD_LEFT,
			DPAD_RIGHT,

			BUTTON_TOUCHPAD,        // PS | Steam Deck
			BUTTON_MICROPHONE,      // PS | Xbox

			MOTION_ACCELERATION,
			MOTION_GYROSCOPE,

			EXTRA_1,
			EXTRA_2,
			EXTRA_3,
			EXTRA_4,

			Unknown
		};

		enum class ControllerAxis {
			AXIS_LEFT_X,
			AXIS_LEFT_Y,
			AXIS_RIGHT_X,
			AXIS_RIGHT_Y,
			TRIGGER_LEFT,	// Xbox:LT | Switch:ZL | PS:L2
			TRIGGER_RIGHT,	// Xbox:RT | Switch:ZR | PS:R2
			Unknown
		};

		enum class ControllerAction {
			Down,
			Up,
			Repeat,
			Unknown
		};

		class ControllerButtonEvent : public InputEvent {
		public:
			ControllerButtonEvent(uint32_t controller_id, ControllerButton button, ControllerAction action);

			uint32_t getControllerId() const;

			ControllerButton getButton() const;

			ControllerAction getAction() const;

		private:
			uint32_t m_controller_id;
			ControllerButton m_button;
			ControllerAction m_action;
		};

		class ControllerAxisEvent : public InputEvent {
		public:
			ControllerAxisEvent(uint32_t controller_id, ControllerAxis axis, float value);

			uint32_t getControllerId() const;

			ControllerAxis getAxis() const;

			float getValue() const;

		private:
			uint32_t m_controller_id;
			ControllerAxis m_axis;
			float m_value;
		};

		enum class ControllerBatteryStatus {
			Unknown,
			NotPresent,
			Discharging,
			Charging,
			Full
		};

		enum class ControllerBatteryLevel {
			Unknown,
			Empty,
			Low,
			Medium,
			High,
			Full
		};

		class ControllerBatteryEvent : public InputEvent {
		public:
			ControllerBatteryEvent(uint32_t controller_id, ControllerBatteryStatus status, ControllerBatteryLevel level, float percentage);

			uint32_t getControllerId() const;

			ControllerBatteryStatus getStatus() const;

			ControllerBatteryLevel getLevel() const;

			float getPercentage() const;

		private:
			uint32_t m_controller_id;
			ControllerBatteryStatus m_status;
			ControllerBatteryLevel m_level;
			float m_percentage;
		};

		class ControllerMotionEvent : public InputEvent {
		public:
			ControllerMotionEvent(uint32_t controller_id,
				float accel_x, float accel_y, float accel_z,
				float gyro_x, float gyro_y, float gyro_z);

			uint32_t getControllerId() const;

			float getAccelX() const;
			float getAccelY() const;
			float getAccelZ() const;

			float getGyroX() const;
			float getGyroY() const;
			float getGyroZ() const;

		private:
			uint32_t m_controller_id;
			float m_accel_x;
			float m_accel_y;
			float m_accel_z;
			float m_gyro_x;
			float m_gyro_y;
			float m_gyro_z;
		};

		enum class ControllerConnectionState {
			Connected,
			Disconnected
		};

		class ControllerConnectionEvent : public InputEvent {
		public:
			ControllerConnectionEvent(uint32_t controller_id, ControllerConnectionState state);

			uint32_t getControllerId() const;

			ControllerConnectionState getState() const;

		private:
			uint32_t m_controller_id;
			ControllerConnectionState m_state;
		};

		class ControllerFeedback {
		public:
			ControllerFeedback(uint32_t controller_id);

			bool setVibration(float left_motor, float right_motor, uint32_t duration_ms = 0);

			bool setTriggerFeedback(bool is_left_trigger, float resistance, float intensity);

			void stopVibration();

			uint32_t getControllerId() const;

		private:
			uint32_t m_controller_id;
		};
	}
}
