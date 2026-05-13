/*
Adapted from CHAMP for Unitree Go2 (3-joint legs)
Corrected transformation order to match CHAMP's approach
*/

#ifndef QUADRUPED_LEG_H
#define QUADRUPED_LEG_H

#include <champ/geometry/geometry.h>
#include <champ/quadruped_base/quadruped_joint.h>

namespace champ
{
    class QuadrupedLeg
    {
        unsigned int no_of_links_;
        
        geometry::Transformation zero_stance_;
        float center_to_nominal_;

        unsigned int id_;

        unsigned long last_touchdown_;

        bool in_contact_;

        int knee_direction_;
        bool is_pantograph_;

        bool gait_phase_;
        
        public:
            QuadrupedLeg():
                no_of_links_(0),
                center_to_nominal_(0),
                id_(0),
                last_touchdown_(0),
                in_contact_(1),
                knee_direction_(0),
                is_pantograph_(false),
                gait_phase_(1)
            {
                // Go2 has 3 joints per leg (no separate foot joint)
                int no_of_joints = 0;
                joint_chain[no_of_joints++] = &hip;
                joint_chain[no_of_joints++] = &thigh;
                joint_chain[no_of_joints++] = &calf;
            }

            geometry::Transformation foot_from_hip()
            {
                // Forward kinematics - build transformation from foot back to hip
                // This matches CHAMP's approach for consistent FK/IK
                geometry::Transformation foot_position;
                foot_position = Identity<4,4>();

                // Build chain backwards: foot -> calf -> thigh
                // Start with foot offset
                foot_position.Translate(foot_offset_x, foot_offset_y, foot_offset_z);
                
                // Rotate by calf angle, then translate by calf offset
                foot_position.RotateY(joint_chain[2]->theta());  // calf rotation
                foot_position.Translate(joint_chain[2]->x(), joint_chain[2]->y(), joint_chain[2]->z());
                
                // Rotate by thigh angle, then translate by thigh offset
                foot_position.RotateY(joint_chain[1]->theta());  // thigh rotation
                foot_position.Translate(joint_chain[1]->x(), joint_chain[1]->y(), joint_chain[1]->z());
                
                // Don't rotate by hip here - that's handled in foot_from_base()

                return foot_position;
            }

            geometry::Transformation foot_from_base()
            {
                // Full forward kinematics from base frame
                geometry::Transformation foot_position;

                // Get position from hip frame
                foot_position.p = foot_from_hip().p;
                
                // Apply hip rotation
                foot_position.RotateX(hip.theta());

                // Translate to base frame
                foot_position.Translate(hip.x(), hip.y(), hip.z());

                return foot_position;
            }

            void joints(float hip_joint, float thigh_joint, float calf_joint)
            { 
                hip.theta(hip_joint);
                thigh.theta(thigh_joint);
                calf.theta(calf_joint);
            }

            void joints(float *joints)
            {
                for(unsigned int i = 0; i < 3; i++)
                {
                    joint_chain[i]->theta(joints[i]);
                }
            }

            geometry::Transformation zero_stance()
            {
                // Calculate nominal standing position
                zero_stance_.X() = hip.x() + thigh.x() + gait_config->com_x_translation;
                zero_stance_.Y() = hip.y() + thigh.y();
                zero_stance_.Z() = hip.z() + thigh.z() + calf.z() + foot_offset_z;
                
                return zero_stance_;
            }

            float center_to_nominal()
            {
                float x = hip.x() + thigh.x();
                float y = hip.y() + thigh.y();
                
                return sqrtf(pow(x,2) + pow(y,2));
            }

            unsigned int id()
            {
                return id_;
            }

            unsigned long int last_touchdown()
            {
                return last_touchdown_;
            }

            void last_touchdown(unsigned long int current_time)
            {
                last_touchdown_ = current_time;
            }

            void id(unsigned int id)
            {
                id_ = id;
            }

            void in_contact(bool in_contact)
            {
                in_contact_ = in_contact;
            }

            bool in_contact()
            {
                return in_contact_;
            }

            void gait_phase(bool phase)
            {
                gait_phase_ = phase;
            }

            bool gait_phase()
            {
                return gait_phase_;
            }

            int knee_direction()
            {
                return knee_direction_;
            }

            void knee_direction(int direction)
            {
                knee_direction_ = direction;
            }

            void is_pantograph(bool config)
            {
                is_pantograph_ = config;
            }

            bool is_pantograph()
            {
                return is_pantograph_;
            }

            void setGaitConfig(champ::GaitConfig *gait_conf)
            {
                gait_config = gait_conf;
            }

            // Go2 joints (3 per leg)
            champ::Joint hip;
            champ::Joint thigh;
            champ::Joint calf;

            // Foot offset (fixed, not a joint)
            float foot_offset_x = 0.0f;
            float foot_offset_y = 0.0f;
            float foot_offset_z = 0.0f;

            champ::GaitConfig *gait_config;

            Joint *joint_chain[3];
    };
}
#endif
