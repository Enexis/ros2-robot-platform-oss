#ifndef GO2_QUADRUPED_DESCRIPTION_H
#define GO2_QUADRUPED_DESCRIPTION_H

#include <champ/quadruped_base/quadruped_base.h>

namespace champ
{
    namespace URDF
    {
        inline void loadFromHeader(champ::QuadrupedBase &base)
        {
            // Dimensions from unitree_go2_description/xacro/constants.xacro
            
            // CRITICAL: Each offset represents the distance from one joint to the next
            // - hip: position of hip joint relative to trunk center
            // - thigh: offset from hip joint to thigh joint
            // - calf: offset from thigh joint to calf joint  (0.213m down)
            // - foot_offset: offset from calf joint to foot contact point (0.213m down)
            //
            // Total leg length from thigh joint to ground = 0.213 + 0.213 = 0.426m
            
            // FL - Front Left Leg
            base.lf.hip.setOrigin(0.1934, 0.0465, 0.0, 0, 0, 0);
            base.lf.thigh.setOrigin(0.0, 0.0955, 0.0, 0, 0, 0);
            base.lf.calf.setOrigin(0.0, 0.0, -0.213, 0, 0, 0);
            base.lf.foot_offset_x = 0.0;
            base.lf.foot_offset_y = 0.0;
            base.lf.foot_offset_z = -0.213;  // MUST be -0.213, not 0!

            // FR - Front Right Leg
            base.rf.hip.setOrigin(0.1934, -0.0465, 0.0, 0, 0, 0);
            base.rf.thigh.setOrigin(0.0, -0.0955, 0.0, 0, 0, 0);
            base.rf.calf.setOrigin(0.0, 0.0, -0.213, 0, 0, 0);
            base.rf.foot_offset_x = 0.0;
            base.rf.foot_offset_y = 0.0;
            base.rf.foot_offset_z = -0.213;

            // RL - Rear Left Leg
            base.lh.hip.setOrigin(-0.1934, 0.0465, 0.0, 0, 0, 0);
            base.lh.thigh.setOrigin(0.0, 0.0955, 0.0, 0, 0, 0);
            base.lh.calf.setOrigin(0.0, 0.0, -0.213, 0, 0, 0);
            base.lh.foot_offset_x = 0.0;
            base.lh.foot_offset_y = 0.0;
            base.lh.foot_offset_z = -0.213;

            // RR - Rear Right Leg
            base.rh.hip.setOrigin(-0.1934, -0.0465, 0.0, 0, 0, 0);
            base.rh.thigh.setOrigin(0.0, -0.0955, 0.0, 0, 0, 0);
            base.rh.calf.setOrigin(0.0, 0.0, -0.213, 0, 0, 0);
            base.rh.foot_offset_x = 0.0;
            base.rh.foot_offset_y = 0.0;
            base.rh.foot_offset_z = -0.213;
        }
    }
}
#endif
