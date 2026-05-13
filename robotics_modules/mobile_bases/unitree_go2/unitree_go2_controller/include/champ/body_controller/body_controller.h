#ifndef BODY_CONTROLLER_H
#define BODY_CONTROLLER_H

#include <champ/geometry/geometry.h>
#include <champ/quadruped_base/quadruped_base.h>
#include <champ/quadruped_base/quadruped_leg.h>
#include <champ/kinematics/kinematics.h>

namespace champ
{
    class BodyController
    {
        QuadrupedBase *base_;

        public:
            BodyController(QuadrupedBase &quadruped_base):
                base_(&quadruped_base)
            {
            }

            void poseCommand(geometry::Transformation (&foot_positions)[4], 
                             const champ::Pose &req_pose)
            {
                for(int i = 0; i < 4; i++)
                {
                    poseCommand(foot_positions[i], *base_->legs[i], req_pose);
                }
            }
            
            static void poseCommand(geometry::Transformation &foot_position, 
                                    champ::QuadrupedLeg &leg, 
                                    const champ::Pose &req_pose)
            {
                float req_translation_x = -req_pose.position.x;

                float req_translation_y = -req_pose.position.y;

                float req_translation_z = -(leg.zero_stance().Z() + req_pose.position.z);
                float max_translation_z = -leg.zero_stance().Z() * 0.65;

                //there shouldn't be any negative translation when
                //the legs are already fully stretched
                if(req_translation_z < 0.0)
                {
                    req_translation_z = 0.0;
                }
                else if(req_translation_z > max_translation_z)
                {
                    req_translation_z = max_translation_z;
                }

                //create a new foot position from position of legs when stretched out
                foot_position = leg.zero_stance();

                //move the foot position to desired body position of the robot
                foot_position.Translate(req_translation_x, req_translation_y, req_translation_z);

                //rotate the leg opposite the required orientation of the body
                foot_position.RotateZ(-req_pose.orientation.yaw);
                foot_position.RotateY(-req_pose.orientation.pitch);
                foot_position.RotateX(-req_pose.orientation.roll);
            }
    };
}

#endif
