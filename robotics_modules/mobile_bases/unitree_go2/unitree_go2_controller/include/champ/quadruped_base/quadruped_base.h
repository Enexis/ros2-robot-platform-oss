#ifndef QUADRUPPED_BASE_H
#define QUADRUPPED_BASE_H
#include <champ/geometry/geometry.h>
#include <champ/quadruped_base/quadruped_leg.h>
#include <champ/quadruped_base/quadruped_components.h>

namespace champ
{
    class QuadrupedBase
    {   
        champ::Velocities speed_;
        
        int getKneeDirection(char direction)
        {
            switch (direction) 
            {
                case '>':
                    return -1;
                case '<':
                    return 1;
                default:
                    return -1;
            }
        }    
        public:
            QuadrupedBase()
            {
                unsigned int total_legs = 0;
                legs[total_legs++] = &lf;
                legs[total_legs++] = &rf;
                legs[total_legs++] = &lh;
                legs[total_legs++] = &rh;
                setGaitConfig(gait_config);
            }
            
            QuadrupedBase(champ::GaitConfig &gait_conf)     
            {
                unsigned int total_legs = 0;
                legs[total_legs++] = &lf;
                legs[total_legs++] = &rf;
                legs[total_legs++] = &lh;
                legs[total_legs++] = &rh;
                setGaitConfig(gait_conf);
            }        
            
            void getJointPositions(float *joint_positions)
            {
                unsigned int total_joints = 0;
                for(unsigned int i = 0; i < 4; i++)
                {
                    joint_positions[total_joints++] = legs[i]->hip.theta();
                    joint_positions[total_joints++] = legs[i]->thigh.theta();
                    joint_positions[total_joints++] = legs[i]->calf.theta();
                }
            }
            
            void getFootPositions(geometry::Transformation *foot_positions)
            {
                for(unsigned int i = 0; i < 4; i++)
                {
                    foot_positions[i] = legs[i]->foot_from_base();
                }
            }
            
            void updateJointPositions(float joints[12])
            {
                for(unsigned int i = 0; i < 4; i++)
                {
                    int index = i * 3;
                    legs[i]->hip.theta(joints[index]);
                    legs[i]->thigh.theta(joints[index + 1]);
                    legs[i]->calf.theta(joints[index + 2]);
                }
            }
            
            void setGaitConfig(champ::GaitConfig &gait_conf)
            {
                gait_config = gait_conf;
                for(unsigned int i=0; i < 4; i++)
                {
                    int dir;
                    legs[i]->id(i);
                    if(i < 2)
                    {
                        dir = getKneeDirection(gait_config.knee_orientation[0]);
                    }
                    else
                    {
                        dir = getKneeDirection(gait_config.knee_orientation[1]);
                    }
                    legs[i]->is_pantograph(gait_config.pantograph_leg);
                    legs[i]->knee_direction(dir);
                    legs[i]->setGaitConfig(&gait_config);
                }
            }
            
            champ::QuadrupedLeg *legs[4];
            champ::QuadrupedLeg lf;
            champ::QuadrupedLeg rf;
            champ::QuadrupedLeg lh;
            champ::QuadrupedLeg rh;
            champ::GaitConfig gait_config;
    };
}
#endif
