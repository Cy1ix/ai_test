#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>
#include <functional>

#include "global_common.h"
#include "scene/components/camera/camera.h"

namespace frame {
    namespace scene {
        
        class QuaternionCamera : public Camera {
        public:
            enum class ProjectionType {
                Perspective,
                Orthographic
            };

            QuaternionCamera(const std::string& name);
            virtual ~QuaternionCamera() = default;
            
            void setProjectionType(ProjectionType type);
            ProjectionType getProjectionType() const;
            
            void setAspectRatio(float aspect_ratio) override;
            void setFieldOfView(float fov);
            float getAspectRatio() const;
            float getFieldOfView() const;
            
            void setOrthoBounds(float left, float right, float bottom, float top);
            float getOrthoLeft() const;
            float getOrthoRight() const;
            float getOrthoBottom() const;
            float getOrthoTop() const;
            
            float getFarPlane() const;
            void setFarPlane(float zfar);
            float getNearPlane() const;
            void setNearPlane(float znear);
            
            void setOrientation(const glm::quat& orientation);
            glm::quat getOrientation() const;
            void rotate(const glm::quat& rotation);
            void rotateEuler(float pitch, float yaw, float roll);
            
            void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
            void orbit(const glm::vec3& target, float delta_yaw, float delta_pitch, float distance = 0.0f);
            
            void setFrustumOffset(const glm::vec2& offset);
            glm::vec2 getFrustumOffset() const;
            
            void enableCameraShake(bool enable);
            void setCameraShakeParameters(float magnitude, float frequency);
            void updateCameraShake(float delta_time);
            
            virtual glm::mat4 getProjection() override;
            glm::mat4 getQuaternionView();
            
            using CameraUpdateCallback = std::function<void(QuaternionCamera&, float)>;
            void setUpdateCallback(CameraUpdateCallback callback);
            void update(float delta_time);

        private:
            ProjectionType m_projection_type{ ProjectionType::Perspective };
            
            float m_aspect_ratio{ 1.0f };
            float m_fov{ glm::radians(60.0f) };
            
            float m_left{ -1.0f };
            float m_right{ 1.0f };
            float m_bottom{ -1.0f };
            float m_top{ 1.0f };
            
            float m_far_plane{ 100.0f };
            float m_near_plane{ 0.1f };
            
            glm::quat m_orientation{ 1.0f, 0.0f, 0.0f, 0.0f };
            
            glm::vec2 m_frustum_offset{ 0.0f };
            
            bool m_shake_enabled{ false };
            float m_shake_magnitude{ 0.1f };
            float m_shake_frequency{ 5.0f };
            float m_shake_time{ 0.0f };
            glm::vec3 m_shake_offset{ 0.0f };
            
            CameraUpdateCallback m_update_callback{ nullptr };
            
            glm::mat4 calculatePerspectiveProjection() const;
            glm::mat4 calculateOrthographicProjection() const;
        };
    }
}