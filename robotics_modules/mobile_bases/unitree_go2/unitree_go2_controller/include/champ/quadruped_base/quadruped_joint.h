#ifndef QUADRUPED_JOINT_H
#define QUADRUPED_JOINT_H

#include <champ/quadruped_base/quadruped_components.h>

namespace champ
{
    class Joint
    {
        // float translation_.x; 
        // float translation_.y; 
        // float translation_.z; 

        // float rotation_.roll; 
        // float rotation_.pitch; 
        // float rotation_.yaw;

        float theta_;

        champ::Point translation_;
        champ::Euler rotation_;

        public:
            Joint():
                theta_(0.0)
            {
            }
            
            Joint(float pos_x, float pos_y, float pos_z, 
                  float or_r,  float or_p,  float or_y):
                theta_(0.0)
            { 
                translation_.x = pos_x;
                translation_.y = pos_y;
                translation_.z = pos_z;
                rotation_.roll = or_r;
                rotation_.pitch = or_p;
                rotation_.yaw = or_y;
            } 

            float theta()
            { 
                return theta_; 
            }

            void theta(float angle)
            { 
                theta_ = angle; 
            }

            void setTranslation(float x, float y, float z)
            {
                RCLCPP_INFO(rclcpp::get_logger("Joint"), "setTranslation called: x=%.4f, y=%.4f, z=%.4f", x, y, z);
                translation_.x = x;
                translation_.y = y;
                translation_.z = z;
                RCLCPP_INFO(rclcpp::get_logger("Joint"), "After assignment: translation_.x=%.4f, translation_.y=%.4f, translation_.z=%.4f", 
                            translation_.x, translation_.y, translation_.z);
            }

            void setRotation(float roll, float pitch, float yaw)
            {
                rotation_.roll = roll;
                rotation_.pitch = pitch;
                rotation_.yaw = yaw;
            }

            void setOrigin(float x, float y, float z,
                    float roll, float pitch, float yaw)
            {
                translation_.x = x;
                translation_.y = y;
                translation_.z = z;
                rotation_.roll = roll;
                rotation_.pitch = pitch;
                rotation_.yaw = yaw;
            }

            float x() const
            {
                return translation_.x;
            }

            float y() const
            {
                return translation_.y;
            }

            float z() const
            {
                return translation_.z;
            }

            float roll() const
            {
                return rotation_.roll;
            }

            float pitch() const
            {
                return rotation_.pitch;
            }

            float yaw() const
            {
                return rotation_.yaw;
            }

    };
}
#endif
