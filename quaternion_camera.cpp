#include "scene/components/camera/quaternion_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/random.hpp>

#include "scene/components/transform.h"
#include "scene/node.h"

namespace frame {
    namespace scene {
        QuaternionCamera::QuaternionCamera(const std::string& name) :
            Camera{ name }
        {
        }

        void QuaternionCamera::setProjectionType(ProjectionType type) {
            m_projection_type = type;
        }

        QuaternionCamera::ProjectionType QuaternionCamera::getProjectionType() const {
            return m_projection_type;
        }

        void QuaternionCamera::setAspectRatio(float aspect_ratio) {
            m_aspect_ratio = aspect_ratio;
        }

        void QuaternionCamera::setFieldOfView(float fov) {
            m_fov = fov;
        }

        float QuaternionCamera::getAspectRatio() const {
            return m_aspect_ratio;
        }

        float QuaternionCamera::getFieldOfView() const {
            return m_fov;
        }

        void QuaternionCamera::setOrthoBounds(float left, float right, float bottom, float top) {
            m_left = left;
            m_right = right;
            m_bottom = bottom;
            m_top = top;
        }

        float QuaternionCamera::getOrthoLeft() const {
            return m_left;
        }

        float QuaternionCamera::getOrthoRight() const {
            return m_right;
        }

        float QuaternionCamera::getOrthoBottom() const {
            return m_bottom;
        }

        float QuaternionCamera::getOrthoTop() const {
            return m_top;
        }

        float QuaternionCamera::getFarPlane() const {
            return m_far_plane;
        }

        void QuaternionCamera::setFarPlane(float zfar) {
            m_far_plane = zfar;
        }

        float QuaternionCamera::getNearPlane() const {
            return m_near_plane;
        }

        void QuaternionCamera::setNearPlane(float znear) {
            m_near_plane = znear;
        }

        void QuaternionCamera::setOrientation(const glm::quat& orientation) {
            m_orientation = glm::normalize(orientation);
        }

        glm::quat QuaternionCamera::getOrientation() const {
            return m_orientation;
        }

        void QuaternionCamera::rotate(const glm::quat& rotation) {
            m_orientation = glm::normalize(rotation * m_orientation);
        }

        void QuaternionCamera::rotateEuler(float pitch, float yaw, float roll) {
            glm::quat qx = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::quat qy = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat qz = glm::angleAxis(roll, glm::vec3(0.0f, 0.0f, 1.0f));

            glm::quat rotation = qz * qy * qx;
            rotate(rotation);
        }

        void QuaternionCamera::lookAt(const glm::vec3& target, const glm::vec3& up) {
            if (!getNode()) {
                throw std::runtime_error{ "Camera component is not attached to a node" };
            }

            auto& transform = getNode()->getComponent<Transform>();
            glm::vec3 position = transform.getTranslation();
            
            glm::mat4 view_matrix = glm::lookAt(position, target, up);
            
            glm::mat3 rotation_matrix(view_matrix);
            m_orientation = glm::normalize(glm::quat_cast(rotation_matrix));
            
            transform.setRotation(m_orientation);
        }

        void QuaternionCamera::orbit(const glm::vec3& target, float delta_yaw, float delta_pitch, float distance) {
            if (!getNode()) {
                throw std::runtime_error{ "Camera component is not attached to a node" };
            }

            auto& transform = getNode()->getComponent<Transform>();
            
            if (distance <= 0.0f) {
                distance = glm::length(transform.getTranslation() - target);
            }
            
            rotateEuler(delta_pitch, delta_yaw, 0.0f);
            
            glm::vec3 forward = glm::rotate(glm::conjugate(m_orientation), glm::vec3(0.0f, 0.0f, -1.0f));
            glm::vec3 new_position = target - forward * distance;
            
            transform.setTranslation(new_position);
        }

        void QuaternionCamera::setFrustumOffset(const glm::vec2& offset) {
            m_frustum_offset = offset;
        }

        glm::vec2 QuaternionCamera::getFrustumOffset() const {
            return m_frustum_offset;
        }

        void QuaternionCamera::enableCameraShake(bool enable) {
            m_shake_enabled = enable;
            if (!enable) {
                m_shake_offset = glm::vec3(0.0f);
            }
        }

        void QuaternionCamera::setCameraShakeParameters(float magnitude, float frequency) {
            m_shake_magnitude = magnitude;
            m_shake_frequency = frequency;
        }

        void QuaternionCamera::updateCameraShake(float delta_time) {
            if (!m_shake_enabled) {
                return;
            }

            m_shake_time += delta_time * m_shake_frequency;
            
            float x = glm::linearRand(-1.0f, 1.0f) * m_shake_magnitude;
            float y = glm::linearRand(-1.0f, 1.0f) * m_shake_magnitude;
            float z = glm::linearRand(-1.0f, 1.0f) * m_shake_magnitude * 0.5f;
            
            m_shake_offset = glm::mix(m_shake_offset, glm::vec3(x, y, z), delta_time * 10.0f);
        }

        glm::mat4 QuaternionCamera::calculatePerspectiveProjection() const {
            glm::mat4 proj = glm::perspective(m_fov, m_aspect_ratio, m_near_plane, m_far_plane);
            
            if (m_frustum_offset != glm::vec2(0.0f)) {
                proj[2][0] += m_frustum_offset.x;
                proj[2][1] += m_frustum_offset.y;
            }

            return proj;
        }

        glm::mat4 QuaternionCamera::calculateOrthographicProjection() const {
            return glm::ortho(m_left, m_right, m_bottom, m_top, m_near_plane, m_far_plane);
        }

        glm::mat4 QuaternionCamera::getProjection() {
            if (m_projection_type == ProjectionType::Perspective) {
                return calculatePerspectiveProjection();
            }
            else {
                return calculateOrthographicProjection();
            }
        }

        glm::mat4 QuaternionCamera::getQuaternionView() {
            if (!getNode()) {
                throw std::runtime_error{ "Camera component is not attached to a node" };
            }

            auto& transform = getNode()->getComponent<Transform>();

            glm::vec3 position = transform.getTranslation();
            if (m_shake_enabled) {
                position += glm::rotate(m_orientation, m_shake_offset);
            }
            
            glm::quat orientation = m_orientation * glm::quat(getPreRotation());
            
            glm::mat4 rotation_matrix = glm::toMat4(glm::conjugate(orientation));
            glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -position);

            return rotation_matrix * translation_matrix;
        }

        void QuaternionCamera::setUpdateCallback(CameraUpdateCallback callback) {
            m_update_callback = callback;
        }

        void QuaternionCamera::update(float delta_time) {
            updateCameraShake(delta_time);
            
            if (m_update_callback) {
                m_update_callback(*this, delta_time);
            }
        }
    }
}