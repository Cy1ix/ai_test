/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2020-2024, Andrew Cox, Huawei Technologies Research & Development (UK) Limited
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

#include "scene/components/camera/free_camera.h"

#include "global_common.h"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include "scene/components/camera/perspective_camera.h"
#include "scene/components/transform.h"
#include "scene/node.h"

namespace frame {
	namespace scene {
		class Camera;
		const float FreeCamera::TOUCH_DOWN_MOVE_FORWARD_WAIT_TIME = 2.0f;

		const float FreeCamera::ROTATION_MOVE_WEIGHT = 0.1f;

		const float FreeCamera::KEY_ROTATION_MOVE_WEIGHT = 0.5f;

		const float FreeCamera::TRANSLATION_MOVE_WEIGHT = 3.0f;

		const float FreeCamera::TRANSLATION_MOVE_STEP = 50.0f;

		const uint32_t FreeCamera::TRANSLATION_MOVE_SPEED = 4;

		FreeCamera::FreeCamera(Node& node) :
			NodeScript{ node, "FreeCamera" }
		{}

		void FreeCamera::update(float delta_time) {

			glm::vec3 delta_translation(0.0f, 0.0f, 0.0f);
			glm::vec3 delta_rotation(0.0f, 0.0f, 0.0f);

			float mul_translation = m_speed_multiplier;

			if (m_key_pressed[platform::KeyCode::W]) {
				delta_translation.z -= TRANSLATION_MOVE_STEP;
			}
			if (m_key_pressed[platform::KeyCode::S]) {
				delta_translation.z += TRANSLATION_MOVE_STEP;
			}
			if (m_key_pressed[platform::KeyCode::A]) {
				delta_translation.x -= TRANSLATION_MOVE_STEP;
			}
			if (m_key_pressed[platform::KeyCode::D]) {
				delta_translation.x += TRANSLATION_MOVE_STEP;
			}
			if (m_key_pressed[platform::KeyCode::Q]) {
				delta_translation.y -= TRANSLATION_MOVE_STEP;
			}
			if (m_key_pressed[platform::KeyCode::E]) {
				delta_translation.y += TRANSLATION_MOVE_STEP;
			}
			if (m_key_pressed[platform::KeyCode::LeftControl]) {
				mul_translation *= (1.0f * TRANSLATION_MOVE_SPEED);
			}
			if (m_key_pressed[platform::KeyCode::LeftShift]) {
				mul_translation *= (1.0f / TRANSLATION_MOVE_SPEED);
			}

			if (m_key_pressed[platform::KeyCode::I]) {
				delta_rotation.x += KEY_ROTATION_MOVE_WEIGHT;
			}
			if (m_key_pressed[platform::KeyCode::K]) {
				delta_rotation.x -= KEY_ROTATION_MOVE_WEIGHT;
			}
			if (m_key_pressed[platform::KeyCode::J]) {
				delta_rotation.y += KEY_ROTATION_MOVE_WEIGHT;
			}
			if (m_key_pressed[platform::KeyCode::L]) {
				delta_rotation.y -= KEY_ROTATION_MOVE_WEIGHT;
			}

			if (m_mouse_button_pressed[platform::MouseButton::Left] && m_mouse_button_pressed[platform::MouseButton::Right]) {
				delta_rotation.z += TRANSLATION_MOVE_WEIGHT * m_mouse_move_delta.x;
			}
			else if (m_mouse_button_pressed[platform::MouseButton::Right]) {
				delta_rotation.x -= ROTATION_MOVE_WEIGHT * m_mouse_move_delta.y;
				delta_rotation.y -= ROTATION_MOVE_WEIGHT * m_mouse_move_delta.x;
			}
			else if (m_mouse_button_pressed[platform::MouseButton::Left]) {
				delta_translation.x += TRANSLATION_MOVE_WEIGHT * m_mouse_move_delta.x;
				delta_translation.y += TRANSLATION_MOVE_WEIGHT * -m_mouse_move_delta.y;
			}

			if (m_touch_pointer_pressed[0]) {

				delta_rotation.x -= ROTATION_MOVE_WEIGHT * m_touch_move_delta.y;
				delta_rotation.y -= ROTATION_MOVE_WEIGHT * m_touch_move_delta.x;

				if (m_touch_pointer_time > TOUCH_DOWN_MOVE_FORWARD_WAIT_TIME) {
					delta_translation.z -= TRANSLATION_MOVE_STEP;
				}
				else {
					m_touch_pointer_time += delta_time;
				}
			}

			delta_translation *= mul_translation * delta_time;
			delta_rotation *= delta_time;
			
			if (delta_rotation != glm::vec3(0.0f, 0.0f, 0.0f) || delta_translation != glm::vec3(0.0f, 0.0f, 0.0f)) {
				auto& transform = getNode().getComponent<Transform>();

				glm::quat qx = glm::angleAxis(delta_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
				glm::quat qy = glm::angleAxis(delta_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));

				glm::quat orientation = glm::normalize(qy * transform.getRotation() * qx);

				transform.setTranslation(transform.getTranslation() + delta_translation * glm::conjugate(orientation));
				transform.setRotation(orientation);
			}

			m_mouse_move_delta = {};
			m_touch_move_delta = {};
		}

		void FreeCamera::inputEvent(const platform::InputEvent& input_event) {

			if (input_event.getSource() == platform::EventSource::Keyboard) {
				const auto& key_event = static_cast<const platform::KeyInputEvent&>(input_event);

				if (key_event.getAction() == platform::KeyAction::Down ||
					key_event.getAction() == platform::KeyAction::Repeat)
				{
					m_key_pressed[key_event.getCode()] = true;
				}
				else {
					m_key_pressed[key_event.getCode()] = false;
				}
			}
			else if (input_event.getSource() == platform::EventSource::Mouse) {

				const auto& mouse_button = static_cast<const platform::MouseButtonInputEvent&>(input_event);

				glm::vec2 mouse_pos{ std::floor(mouse_button.getPosX()), std::floor(mouse_button.getPosY()) };

				if (mouse_button.getAction() == platform::MouseAction::Down) {
					m_mouse_button_pressed[mouse_button.getButton()] = true;
				}

				if (mouse_button.getAction() == platform::MouseAction::Up) {
					m_mouse_button_pressed[mouse_button.getButton()] = false;
				}

				if (mouse_button.getAction() == platform::MouseAction::Move) {
					m_mouse_move_delta = mouse_pos - m_mouse_last_pos;
					m_mouse_last_pos = mouse_pos;
				}
			}
			else if (input_event.getSource() == platform::EventSource::Touchscreen) {

				const auto& touch_event = static_cast<const platform::TouchInputEvent&>(input_event);

				glm::vec2 touch_pos{ std::floor(touch_event.getPosX()), std::floor(touch_event.getPosY()) };

				if (touch_event.getAction() == platform::TouchAction::Down) {
					m_touch_pointer_pressed[touch_event.getPointerId()] = true;
					m_touch_last_pos = touch_pos;
				}

				if (touch_event.getAction() == platform::TouchAction::Up) {
					m_touch_pointer_pressed[touch_event.getPointerId()] = false;
					m_touch_pointer_time = 0.0f;
				}

				if (touch_event.getAction() == platform::TouchAction::Move && touch_event.getPointerId() == 0) {
					m_touch_move_delta = touch_pos - m_touch_last_pos;
					m_touch_last_pos = touch_pos;
				}
			}
		}

		void FreeCamera::resize(uint32_t width, uint32_t height) {
			auto& camera_node = getNode();

			if (camera_node.hasComponent<Camera>()) {
				if (auto camera = dynamic_cast<PerspectiveCamera*>(&camera_node.getComponent<Camera>())) {
					camera->setAspectRatio(static_cast<float>(width) / height);
				}
			}
		}
	}
}
