/*
Go2 Kinematics - Final corrected version for 3-joint legs
Based on CHAMP kinematics, adapted for Unitree Go2
*/

#ifndef KINEMATICS_H
#define KINEMATICS_H

#ifdef __unix__
    #include <cmath>
    using namespace std;
#endif

#include <champ/macros/macros.h>
#include <champ/geometry/geometry.h>
#include <champ/quadruped_base/quadruped_leg.h>
#include <champ/quadruped_base/quadruped_base.h>

namespace champ
{
    class Kinematics
    {
        champ::QuadrupedBase *base_;
        
        public:
            Kinematics(champ::QuadrupedBase &quadruped_base):
                base_(&quadruped_base)
            {
            }

            void inverse(float (&joint_positions)[12], geometry::Transformation (&foot_positions)[4])
            {
                float calculated_joints[12];
                geometry::Transformation hip_frame_foot_pos;

                for(unsigned int i = 0; i < 4; i++)
                {
                    // Transform from base frame to hip frame - IK expects hip frame!
                    hip_frame_foot_pos = foot_positions[i];
                    transformToHip(hip_frame_foot_pos, *base_->legs[i]);
                    
                    inverse(calculated_joints[(i*3)], calculated_joints[(i*3) + 1], calculated_joints[(i*3) + 2], 
                           *base_->legs[i], hip_frame_foot_pos);
                    
                    if(isnan(calculated_joints[(i*3)]) || isnan(calculated_joints[(i*3) + 1]) || isnan(calculated_joints[(i*3) + 2]))
                    {
                        return;
                    }
                }
                
                for(unsigned int i = 0; i < 12; i++)
                {
                    joint_positions[i] = calculated_joints[i];
                }
            }

            static void inverse(float &hip_joint, float &thigh_joint, float &calf_joint, 
                                champ::QuadrupedLeg &leg, geometry::Transformation &foot_position)
            {
                geometry::Transformation temp_foot_pos = foot_position;

                // l0 is the total lateral offset from hip rotation axis
                // Sum all Y offsets in the kinematic chain (matching CHAMP's approach)
                // For Go2: joint_chain[1] (thigh) + joint_chain[2] (calf)
                float l0 = 0.0f;
                for(unsigned int i = 1; i < 3; i++)
                {
                    l0 += leg.joint_chain[i]->y();
                }

                // Link lengths using CHAMP's sqrt pattern for consistency
                // l1 is the effective length of the thigh link
                float l1 = -sqrtf(pow(leg.calf.x(), 2) + pow(leg.calf.z(), 2));
                
                // l2 is the effective length of the calf link (to foot contact point)
                float l2 = -sqrtf(pow(leg.foot_offset_x, 2) + pow(leg.foot_offset_z, 2));

                float x = temp_foot_pos.X();
                float y = temp_foot_pos.Y();
                float z = temp_foot_pos.Z();

                // Hip joint calculation (abduction/adduction around X-axis)
                float y2_z2 = pow(y, 2) + pow(z, 2);
                float sqrt_y2_z2 = sqrtf(y2_z2);
                
                // Reachability check for hip joint
                if(sqrt_y2_z2 < fabs(l0))
                {
                    hip_joint = NAN;
                    thigh_joint = NAN;
                    calf_joint = NAN;
                    return;
                }
                
                hip_joint = -(atanf(y / z) - ((M_PI/2) - acosf(-l0 / sqrt_y2_z2)));

                // Transform foot position to thigh frame
                // After hip rotation, lateral offset is absorbed, so Y = 0
                // This matches CHAMP's approach: Translate(-upper_leg.x(), 0.0f, -upper_leg.z())
                temp_foot_pos.RotateX(-hip_joint);
                temp_foot_pos.Translate(-leg.thigh.x(), 0.0f, -leg.thigh.z());

                x = temp_foot_pos.X();
                y = temp_foot_pos.Y();
                z = temp_foot_pos.Z();

                // Reachability check - target must be within reach of two-link chain
                float target_distance = sqrtf(pow(x, 2) + pow(z, 2));
                if(target_distance > (fabs(l1) + fabs(l2)) || 
                   target_distance < fabs(fabs(l1) - fabs(l2)))
                {
                    hip_joint = NAN;
                    thigh_joint = NAN;
                    calf_joint = NAN;
                    return;
                }

                // Two-link IK using law of cosines
                // Calf joint angle (knee)
                float cos_calf = (pow(target_distance, 2) - pow(l1, 2) - pow(l2, 2)) / (2 * l1 * l2);
                
                // Clamp to valid range to handle floating point errors
                cos_calf = fmaxf(-1.0f, fminf(1.0f, cos_calf));
                
                calf_joint = leg.knee_direction() * acosf(cos_calf);
                
                // Thigh joint angle
                thigh_joint = atanf(x / z) - atanf((l2 * sinf(calf_joint)) / (l1 + (l2 * cosf(calf_joint))));

                // Adjust thigh angle based on knee direction
                // This ensures the leg configuration is physically valid
                if(leg.knee_direction() < 0)
                {
                    if(thigh_joint < 0)
                        thigh_joint = thigh_joint + M_PI;
                }
                else 
                {
                    if(thigh_joint > 0)
                        thigh_joint = thigh_joint + M_PI;
                }
            }

            static void forward(geometry::Transformation &foot_position, const champ::QuadrupedLeg &leg, 
                                const float thigh_theta, 
                                const float calf_theta)
            {
                // Forward kinematics from thigh frame to foot
                // Build transformation in reverse order (foot back to thigh)
                foot_position = Identity<4,4>();

                // Start with foot offset
                foot_position.Translate(leg.foot_offset_x, leg.foot_offset_y, leg.foot_offset_z);
                
                // Apply calf rotation and translation
                foot_position.RotateY(calf_theta);
                foot_position.Translate(leg.calf.x(), leg.calf.y(), leg.calf.z());
                
                // Apply thigh rotation and translation
                foot_position.RotateY(thigh_theta);
                foot_position.Translate(leg.thigh.x(), leg.thigh.y(), leg.thigh.z());
            }

            static void forward(geometry::Transformation &foot_position, const champ::QuadrupedLeg &leg, 
                                const float hip_theta, 
                                const float thigh_theta, 
                                const float calf_theta)
            {
                // Full forward kinematics from base to foot
                // Build transformation in reverse order (foot back to base)
                foot_position = Identity<4,4>();

                // Start with foot offset
                foot_position.Translate(leg.foot_offset_x, leg.foot_offset_y, leg.foot_offset_z);

                // Apply calf rotation and translation
                foot_position.RotateY(calf_theta);
                foot_position.Translate(leg.calf.x(), leg.calf.y(), leg.calf.z());

                // Apply thigh rotation and translation
                foot_position.RotateY(thigh_theta);   
                foot_position.Translate(leg.thigh.x(), leg.thigh.y(), leg.thigh.z());

                // Apply hip rotation and translation
                foot_position.RotateX(hip_theta);   
                foot_position.Translate(leg.hip.x(), leg.hip.y(), leg.hip.z());
            }

            static void transformToHip(geometry::Transformation &foot_position, const champ::QuadrupedLeg &leg)
            {
                foot_position.Translate(-leg.hip.x(), -leg.hip.y(), -leg.hip.z());
            }

            static void transformToBase(geometry::Transformation &foot_position, const champ::QuadrupedLeg &leg)
            {
                foot_position.Translate(leg.hip.x(), leg.hip.y(), leg.hip.z());
            }
    };
}

#endif
