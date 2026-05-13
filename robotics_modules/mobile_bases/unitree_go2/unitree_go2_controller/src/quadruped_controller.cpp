#include "quadruped_controller.h"
#include <algorithm>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <champ/utils/urdf_loader.h>

static float applyDeadzone(float value, float deadzone)
{
    if (fabs(value) < deadzone)
    {
        return 0.0f;
    }
    return value;
}

QuadrupedController::QuadrupedController()
    : Node("quadruped_controller"),
      joint_states_received_(false),
      startup_transition_complete_(false),
      startup_blend_counter_(0),
      in_motion_(false)
{
    // Declare parameters
    this->declare_parameter("gait.knee_orientation", ">>");
    this->declare_parameter("gait.pantograph_leg", false);
    this->declare_parameter("gait.odom_scaler", 1.0);
    this->declare_parameter("gait.max_linear_velocity_x", 0.5);
    this->declare_parameter("gait.max_linear_velocity_y", 0.25);
    this->declare_parameter("gait.max_angular_velocity_z", 1.0);
    this->declare_parameter("gait.com_x_translation", 0.0);
    this->declare_parameter("gait.swing_height", 0.04);
    this->declare_parameter("gait.stance_depth", 0.00);
    this->declare_parameter("gait.stance_duration", 0.25);
    this->declare_parameter("gait.nominal_height", 0.30);
    this->declare_parameter("control_rate", 100.0);

    // Declare links_map parameters (required by CHAMP urdf_loader)
    this->declare_parameter("links_map.left_front", std::vector<std::string>());
    this->declare_parameter("links_map.right_front", std::vector<std::string>());
    this->declare_parameter("links_map.left_hind", std::vector<std::string>());
    this->declare_parameter("links_map.right_hind", std::vector<std::string>());
    
    // Declare joints_map parameters (required by CHAMP urdf_loader)
    this->declare_parameter("joints_map.left_front", std::vector<std::string>());
    this->declare_parameter("joints_map.right_front", std::vector<std::string>());
    this->declare_parameter("joints_map.left_hind", std::vector<std::string>());
    this->declare_parameter("joints_map.right_hind", std::vector<std::string>());

    // Load gait configuration
    std::string knee_orientation = this->get_parameter("gait.knee_orientation").as_string();
    gait_config_.knee_orientation = knee_orientation.c_str();
    gait_config_.pantograph_leg = this->get_parameter("gait.pantograph_leg").as_bool();
    gait_config_.odom_scaler = this->get_parameter("gait.odom_scaler").as_double();
    gait_config_.max_linear_velocity_x = this->get_parameter("gait.max_linear_velocity_x").as_double();
    gait_config_.max_linear_velocity_y = this->get_parameter("gait.max_linear_velocity_y").as_double();
    gait_config_.max_angular_velocity_z = this->get_parameter("gait.max_angular_velocity_z").as_double();
    gait_config_.com_x_translation = this->get_parameter("gait.com_x_translation").as_double();
    gait_config_.swing_height = this->get_parameter("gait.swing_height").as_double();
    gait_config_.stance_depth = this->get_parameter("gait.stance_depth").as_double();
    gait_config_.stance_duration = this->get_parameter("gait.stance_duration").as_double();
    gait_config_.nominal_height = this->get_parameter("gait.nominal_height").as_double();
    
    control_rate_ = this->get_parameter("control_rate").as_double();

    // 1. Initialize quadruped base with gait config
    quadruped_ = champ::QuadrupedBase(gait_config_);
    // Fix the leg pointers to point to quadruped_'s actual members
    quadruped_.legs[0] = &quadruped_.lf;
    quadruped_.legs[1] = &quadruped_.rf;
    quadruped_.legs[2] = &quadruped_.lh;
    quadruped_.legs[3] = &quadruped_.rh;
    
    quadruped_.lf.joint_chain[0] = &quadruped_.lf.hip;
    quadruped_.lf.joint_chain[1] = &quadruped_.lf.thigh;
    quadruped_.lf.joint_chain[2] = &quadruped_.lf.calf;
    
    quadruped_.rf.joint_chain[0] = &quadruped_.rf.hip;
    quadruped_.rf.joint_chain[1] = &quadruped_.rf.thigh;
    quadruped_.rf.joint_chain[2] = &quadruped_.rf.calf;
    
    quadruped_.lh.joint_chain[0] = &quadruped_.lh.hip;
    quadruped_.lh.joint_chain[1] = &quadruped_.lh.thigh;
    quadruped_.lh.joint_chain[2] = &quadruped_.lh.calf;
    
    quadruped_.rh.joint_chain[0] = &quadruped_.rh.hip;
    quadruped_.rh.joint_chain[1] = &quadruped_.rh.thigh;
    quadruped_.rh.joint_chain[2] = &quadruped_.rh.calf;
    
    // CRITICAL: Set the gait config pointer for each leg
    quadruped_.lf.setGaitConfig(&gait_config_);
    quadruped_.rf.setGaitConfig(&gait_config_);
    quadruped_.lh.setGaitConfig(&gait_config_);
    quadruped_.rh.setGaitConfig(&gait_config_);
    
    // 2. RETRIEVE THE URDF STRING
    std::string urdf_xml;
    this->declare_parameter("robot_description", std::string());

    RCLCPP_INFO(this->get_logger(), "=== DEBUG: Checking for parameters ===");
    std::vector<std::string> param_names = {"links_map.left_front", "links_map.right_front", 
                                             "links_map.left_hind", "links_map.right_hind"};
    for (const auto& name : param_names) {
        if (this->has_parameter(name)) {
            RCLCPP_INFO(this->get_logger(), "Found parameter: %s", name.c_str());
        } else {
            RCLCPP_ERROR(this->get_logger(), "MISSING parameter: %s", name.c_str());
        }
    }

    if (!this->get_parameter("robot_description", urdf_xml) || urdf_xml.empty())
    {
        RCLCPP_FATAL(this->get_logger(), "FATAL: 'robot_description' parameter not found or empty.");
        throw std::runtime_error("robot_description parameter missing");
    }
    
    RCLCPP_INFO(this->get_logger(), "Successfully retrieved robot_description parameter.");

    // 3. Load URDF and populate joint_chain with geometry
    try
    {
        champ::URDF::loadFromString(
            quadruped_, 
            this->get_node_parameters_interface(),
            urdf_xml
        );
        RCLCPP_INFO(this->get_logger(), "Successfully loaded kinematics from URDF");
        
        RCLCPP_INFO(this->get_logger(), "=== Final leg geometry check ===");
        for(int leg = 0; leg < 4; leg++)
        {
            RCLCPP_INFO(this->get_logger(), "Leg %d:", leg);
            RCLCPP_INFO(this->get_logger(), "  hip: (%.4f, %.4f, %.4f)", 
                        quadruped_.legs[leg]->hip.x(),
                        quadruped_.legs[leg]->hip.y(),
                        quadruped_.legs[leg]->hip.z());
            RCLCPP_INFO(this->get_logger(), "  thigh: (%.4f, %.4f, %.4f)", 
                        quadruped_.legs[leg]->thigh.x(),
                        quadruped_.legs[leg]->thigh.y(),
                        quadruped_.legs[leg]->thigh.z());
            RCLCPP_INFO(this->get_logger(), "  calf: (%.4f, %.4f, %.4f)", 
                        quadruped_.legs[leg]->calf.x(),
                        quadruped_.legs[leg]->calf.y(),
                        quadruped_.legs[leg]->calf.z());
        }
    }
    catch (const std::exception& e)
    {
        RCLCPP_FATAL(this->get_logger(), "FATAL: URDF loading failed: %s", e.what());
        throw;
    }
    
    RCLCPP_INFO(this->get_logger(), "Setting foot offsets...");
    quadruped_.lf.foot_offset_x = 0.0f;
    quadruped_.lf.foot_offset_y = 0.0f;
    quadruped_.lf.foot_offset_z = -0.213f;
    
    quadruped_.rf.foot_offset_x = 0.0f;
    quadruped_.rf.foot_offset_y = 0.0f;
    quadruped_.rf.foot_offset_z = -0.213f;
    
    quadruped_.lh.foot_offset_x = 0.0f;
    quadruped_.lh.foot_offset_y = 0.0f;
    quadruped_.lh.foot_offset_z = -0.213f;
    
    quadruped_.rh.foot_offset_x = 0.0f;
    quadruped_.rh.foot_offset_y = 0.0f;
    quadruped_.rh.foot_offset_z = -0.213f;
    
    RCLCPP_INFO(this->get_logger(), "=== Checking zero_stance positions ===");
    for(int i = 0; i < 4; i++)
    {
        auto zero_pos = quadruped_.legs[i]->zero_stance();
        RCLCPP_INFO(this->get_logger(), "Leg %d zero_stance: (%.4f, %.4f, %.4f)", 
            i, zero_pos.X(), zero_pos.Y(), zero_pos.Z());
    }
    
    RCLCPP_INFO(this->get_logger(), "=== Knee directions ===");
    for(int i = 0; i < 4; i++)
    {
        RCLCPP_INFO(this->get_logger(), "Leg %d knee_direction: %d", 
            i, quadruped_.legs[i]->knee_direction());
    }
    
    // 4. Initialize controllers AFTER URDF loading
    body_controller_ = new champ::BodyController(quadruped_);
    leg_controller_ = new champ::LegController(quadruped_);
    kinematics_ = new champ::Kinematics(quadruped_);
    odometry_ = new champ::Odometry(quadruped_);

    // CRITICAL: Initialize joint positions to match URDF initial state
    // From URDF: thigh joints start at 0.58 rad, calf joints at -1.16 rad
    float initial_joints[12] = {
        0.0f, 0.58f, -1.16f,  // FL
        0.0f, 0.58f, -1.16f,  // FR  
        0.0f, 0.58f, -1.16f,  // RL
        0.0f, 0.58f, -1.16f   // RR
    };
    
    // Set these joint angles in the quadruped model
    for(int i = 0; i < 4; i++)
    {
        quadruped_.legs[i]->joints(initial_joints[i*3], initial_joints[i*3+1], initial_joints[i*3+2]);
        
        // VERIFY angles were set
        float hip_check = quadruped_.legs[i]->hip.theta();
        float thigh_check = quadruped_.legs[i]->thigh.theta();
        float calf_check = quadruped_.legs[i]->calf.theta();
        RCLCPP_INFO(this->get_logger(), "Leg %d angles AFTER setting: hip=%.4f, thigh=%.4f, calf=%.4f",
                    i, hip_check, thigh_check, calf_check);
    }
    
    // Now calculate foot positions using forward kinematics from actual joint angles
    for(int i = 0; i < 4; i++)
    {
        target_foot_positions_[i] = quadruped_.legs[i]->foot_from_base();
        RCLCPP_INFO(this->get_logger(), "Initial foot %d position: (%.4f, %.4f, %.4f)", 
                    i, target_foot_positions_[i].X(), 
                    target_foot_positions_[i].Y(), 
                    target_foot_positions_[i].Z());
    }

    // Initialize joint positions array
    for(int i = 0; i < 12; i++)
    {
        current_joint_positions_[i] = initial_joints[i];
    }

    // Initialize velocities
    req_velocities_.linear.x = 0.0f;
    req_velocities_.linear.y = 0.0f;
    req_velocities_.linear.z = 0.0f;
    req_velocities_.angular.x = 0.0f;
    req_velocities_.angular.y = 0.0f;
    req_velocities_.angular.z = 0.0f;

    // Initialize pose - CRITICAL: z must be nominal height, not zero!
    req_pose_.position.z = gait_config_.nominal_height;  // Use configured nominal height
    req_pose_.position.x = 0.0f;
    req_pose_.position.y = 0.0f;
    req_pose_.orientation.roll = 0.0f;
    req_pose_.orientation.pitch = 0.0f;
    req_pose_.orientation.yaw = 0.0f;

    RCLCPP_INFO(this->get_logger(), "Gait config: nominal_height=%.4f, swing_height=%.4f", 
        gait_config_.nominal_height, gait_config_.swing_height);

    // ROS2 subscribers
    cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "cmd_vel", 10,
        std::bind(&QuadrupedController::cmdVelCallback, this, std::placeholders::_1));

    body_pose_sub_ = this->create_subscription<unitree_go2_msgs::msg::Pose>(
        "body_pose", 10,
        std::bind(&QuadrupedController::bodyPoseCallback, this, std::placeholders::_1));

    joint_state_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
        "joint_states", 10,
        std::bind(&QuadrupedController::jointStateCallback, this, std::placeholders::_1));

    // ROS2 publishers
    joint_cmd_pub_ = this->create_publisher<trajectory_msgs::msg::JointTrajectory>(
        "joint_group_effort_controller/joint_trajectory", 10);

    odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>(
        "odom", 50);

    // Control loop timer
    auto timer_period = std::chrono::duration<double>(1.0 / control_rate_);
    control_timer_ = this->create_wall_timer(
        timer_period,
        std::bind(&QuadrupedController::controlLoop, this));

    RCLCPP_INFO(this->get_logger(), "Quadruped Controller initialized");
    RCLCPP_INFO(this->get_logger(), "Nominal height: %.3f m", gait_config_.nominal_height);
    RCLCPP_INFO(this->get_logger(), "Control rate: %.1f Hz", control_rate_);
}

void QuadrupedController::cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
    req_velocities_.linear.x = msg->linear.x;
    req_velocities_.linear.y = msg->linear.y;
    req_velocities_.angular.z = msg->angular.z;

    in_motion_ = (msg->linear.x != 0.0 || msg->linear.y != 0.0 || msg->angular.z != 0.0);
}

void QuadrupedController::bodyPoseCallback(const unitree_go2_msgs::msg::Pose::SharedPtr msg)
{
    // Define deadzones to filter out IMU/sensor noise
    const float pos_deadzone = 0.01f; // 1 cm
    const float rot_deadzone = 0.02f; // ~1.15 degrees

    // Apply deadzones to filter out noise.
    // We treat incoming messages as *offsets* from the default pose.
    // This stops the controller from "shaking" as it tries to
    // correct for tiny, insignificant sensor noise.
    
    // X, Y, Roll, Pitch, Yaw are offsets from 0
    req_pose_.position.x = applyDeadzone(msg->x, pos_deadzone);
    req_pose_.position.y = applyDeadzone(msg->y, pos_deadzone);
    req_pose_.orientation.roll = applyDeadzone(msg->roll, rot_deadzone);
    req_pose_.orientation.pitch = applyDeadzone(msg->pitch, rot_deadzone);
    req_pose_.orientation.yaw = applyDeadzone(msg->yaw, rot_deadzone);
    
    // Z is an offset from the configured nominal_height
    // We deadzone the *offset* (msg->z), not the final value.
    req_pose_.position.z = gait_config_.nominal_height + applyDeadzone(msg->z, pos_deadzone);
}

void QuadrupedController::jointStateCallback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    // Map joint states to our internal representation
    // Expected order: FL, FR, RL, RR (hip, thigh, calf for each)
    const std::vector<std::string> expected_joints = {
        "FL_hip_joint", "FL_thigh_joint", "FL_calf_joint",
        "FR_hip_joint", "FR_thigh_joint", "FR_calf_joint",
        "RL_hip_joint", "RL_thigh_joint", "RL_calf_joint",
        "RR_hip_joint", "RR_thigh_joint", "RR_calf_joint"
    };
    
    // Update current joint positions
    for(size_t i = 0; i < expected_joints.size() && i < 12; i++)
    {
        auto it = std::find(msg->name.begin(), msg->name.end(), expected_joints[i]);
        if(it != msg->name.end())
        {
            size_t idx = std::distance(msg->name.begin(), it);
            if(idx < msg->position.size())
            {
                current_joint_positions_[i] = msg->position[idx];
            }
        }
    }
    
    // Always update the quadruped model with current joint states
    for(int leg = 0; leg < 4; leg++)
    {
        int idx = leg * 3;
        quadruped_.legs[leg]->joints(
            current_joint_positions_[idx],
            current_joint_positions_[idx + 1],
            current_joint_positions_[idx + 2]
        );
    }
    
    // On first reception, initialize foot positions and log
    if(!joint_states_received_)
    {
        for(int leg = 0; leg < 4; leg++)
        {
            int idx = leg * 3;
            quadruped_.legs[leg]->joints(
                current_joint_positions_[idx],
                current_joint_positions_[idx + 1],
                current_joint_positions_[idx + 2]
            );
        }
    
        // Calculate initial foot positions from spawn joint states
        for(int i = 0; i < 4; i++)
        {
            startup_foot_positions_[i] = quadruped_.legs[i]->foot_from_base();
            target_foot_positions_[i] = startup_foot_positions_[i];
            
            RCLCPP_INFO(this->get_logger(), "Spawn foot %d at: (%.3f, %.3f, %.3f)", 
                i,
                startup_foot_positions_[i].X(),
                startup_foot_positions_[i].Y(),
                startup_foot_positions_[i].Z());
        }
        
        // Log what zero_stance would be
        for(int i = 0; i < 4; i++)
        {
            auto zero_pos = quadruped_.legs[i]->zero_stance();
            RCLCPP_INFO(this->get_logger(), "Zero stance foot %d at: (%.3f, %.3f, %.3f)",
                i, zero_pos.X(), zero_pos.Y(), zero_pos.Z());
        }
        
        RCLCPP_INFO(this->get_logger(), "Joint states received - starting smooth transition to stance");
        joint_states_received_ = true;
    }
}

void QuadrupedController::controlLoop()
{
    static int loop_count = 0;
    static bool was_moving = false;
    const int STARTUP_BLEND_CYCLES = 100;  // 2 seconds at 50Hz
    
    // Wait for joint states before processing commands
    if(!joint_states_received_)
    {
        if(loop_count % 50 == 0)  // Log every second at 50Hz
        {
            RCLCPP_WARN(this->get_logger(), "Waiting for joint states...");
        }
        loop_count++;
        return;
    }
    
    for(int leg = 0; leg < 4; leg++)
    {
        int idx = leg * 3;
        quadruped_.legs[leg]->joints(
            current_joint_positions_[idx],
            current_joint_positions_[idx + 1],
            current_joint_positions_[idx + 2]
        );
    }
    
    try {
        // Check if we have velocity commands
        bool has_velocity = (req_velocities_.linear.x != 0.0 || 
                            req_velocities_.linear.y != 0.0 || 
                            req_velocities_.angular.z != 0.0);
        
        // Smooth startup transition from spawn to stance
        if(!startup_transition_complete_)
        {
            // Get zero_stance positions (target standing pose)
            geometry::Transformation zero_stance_positions[4];
            for(int i = 0; i < 4; i++)
            {
                zero_stance_positions[i] = quadruped_.legs[i]->zero_stance();
            }
            
            float blend_factor = static_cast<float>(startup_blend_counter_) / STARTUP_BLEND_CYCLES;
            blend_factor = std::min(blend_factor, 1.0f);
            
            blend_factor = (1.0f - cosf(blend_factor * M_PI)) / 2.0f;
            
            // Blend from startup position to zero_stance
            for(int i = 0; i < 4; i++)
            {
                target_foot_positions_[i].X() = 
                    startup_foot_positions_[i].X() + blend_factor * (zero_stance_positions[i].X() - startup_foot_positions_[i].X());
                target_foot_positions_[i].Y() = 
                    startup_foot_positions_[i].Y() + blend_factor * (zero_stance_positions[i].Y() - startup_foot_positions_[i].Y());
                target_foot_positions_[i].Z() = 
                    startup_foot_positions_[i].Z() + blend_factor * (zero_stance_positions[i].Z() - startup_foot_positions_[i].Z());
            }
            
            startup_blend_counter_++;
            
            if(startup_blend_counter_ >= STARTUP_BLEND_CYCLES)
            {
                startup_transition_complete_ = true;
                RCLCPP_INFO(this->get_logger(), "Startup transition complete - ready for commands");
            }
            else if(startup_blend_counter_ % 25 == 0)
            {
                RCLCPP_INFO(this->get_logger(), "Transition to stance: %.0f%%", blend_factor * 100.0);
            }
        }
        else
        {
            // Normal operation after startup
            
            if(has_velocity)
            {
                if(!was_moving)
                {
                    RCLCPP_INFO(this->get_logger(), "Starting motion");
                    
                    // ONLY on transition to moving: reset to zero_stance
                    for(int i = 0; i < 4; i++)
                    {
                        target_foot_positions_[i] = quadruped_.legs[i]->zero_stance();
                    }
                    
                    was_moving = true;
                }
                
                // --- START NEW FIX ---
                
                // Create a temporary pose for walking.
                // We need the body's X, Y, Z position from req_pose_...
                champ::Pose walking_pose = req_pose_; 
                
                // ...but we MUST zero out the orientation.
                // This stops the body_controller from fighting the
                // natural sway (roll/pitch) of the walking gait.
                walking_pose.orientation.roll = 0.0f;
                walking_pose.orientation.pitch = 0.0f;
                walking_pose.orientation.yaw = 0.0f; // Yaw is handled by the leg_controller

                // Apply body pose to maintain height/translation using the CLEANED pose
                body_controller_->poseCommand(target_foot_positions_, walking_pose);
                
                // Apply gait trajectory - this MODIFIES target_foot_positions_ over time
                // DO NOT reset to zero_stance after this - let it evolve!
                leg_controller_->velocityCommand(target_foot_positions_, req_velocities_);
                
                if(loop_count % 50 == 0)
                {
                    RCLCPP_DEBUG(this->get_logger(), "Walking - vel: (%.2f, %.2f, %.2f)", 
                        req_velocities_.linear.x,
                        req_velocities_.linear.y, 
                        req_velocities_.angular.z);
                }
            }
            else
            {
                if(was_moving)
                {
                    RCLCPP_INFO(this->get_logger(), "Stopped - returning to stance");
                    
                    // When stopping, reset to zero_stance
                    for(int i = 0; i < 4; i++)
                    {
                        target_foot_positions_[i] = quadruped_.legs[i]->zero_stance();
                    }
                    
                    was_moving = false;
                }
                
                // While stationary, maintain stance with body pose
                body_controller_->poseCommand(target_foot_positions_, req_pose_);
                
                if(loop_count == STARTUP_BLEND_CYCLES + 1)
                {
                    RCLCPP_INFO(this->get_logger(), "Ready - holding standing stance");
                }
            }
        }

        // Solve inverse kinematics
        float target_joints[12];
        kinematics_->inverse(target_joints, target_foot_positions_);

        // Validate joint commands before publishing
        bool valid = true;
        for(int i = 0; i < 12; i++)
        {
            if(std::isnan(target_joints[i]) || std::isinf(target_joints[i]))
            {
                RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                    "Invalid joint %d: %.4f - skipping command", i, target_joints[i]);
                
                // Log the foot positions that caused the IK failure
                if(loop_count % 50 == 0)
                {
                    for(int leg = 0; leg < 4; leg++)
                    {
                        RCLCPP_ERROR(this->get_logger(), "Foot %d target: (%.3f, %.3f, %.3f)",
                            leg,
                            target_foot_positions_[leg].X(),
                            target_foot_positions_[leg].Y(),
                            target_foot_positions_[leg].Z());
                    }
                }
                
                valid = false;
                break;
            }
        }

        if(valid)
        {
            // Log first few joint commands for verification
            if(loop_count < 3 || (loop_count == STARTUP_BLEND_CYCLES) || (loop_count == STARTUP_BLEND_CYCLES + 1))
            {
                RCLCPP_INFO(this->get_logger(), 
                    "Loop %d - Commanded joints: FL=(%.2f,%.2f,%.2f) FR=(%.2f,%.2f,%.2f)", 
                    loop_count,
                    target_joints[0], target_joints[1], target_joints[2],
                    target_joints[3], target_joints[4], target_joints[5]);
                    
                RCLCPP_INFO(this->get_logger(),
                    "Loop %d - Actual joints:    FL=(%.2f,%.2f,%.2f) FR=(%.2f,%.2f,%.2f)",
                    loop_count,
                    current_joint_positions_[0], current_joint_positions_[1], current_joint_positions_[2],
                    current_joint_positions_[3], current_joint_positions_[4], current_joint_positions_[5]);
            }
            
            publishJoints(target_joints);
        }
        else
        {
            RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                "IK failed - invalid joint values detected");
        }

        // Update odometry
        float dt = 1.0 / control_rate_;
        publishOdometry(dt);
        
        loop_count++;
    }
    catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Control loop exception: %s", e.what());
    }
}

void QuadrupedController::publishJoints(float joints[12])
{
    auto joint_msg = trajectory_msgs::msg::JointTrajectory();
    // Don't set header stamp - let controller use its own time for immediate execution
    // joint_msg.header.stamp = this->now();
    
    // Joint names (matching Go2 URDF)
    joint_msg.joint_names = {
        "FL_hip_joint", "FL_thigh_joint", "FL_calf_joint",
        "FR_hip_joint", "FR_thigh_joint", "FR_calf_joint",
        "RL_hip_joint", "RL_thigh_joint", "RL_calf_joint",
        "RR_hip_joint", "RR_thigh_joint", "RR_calf_joint"
    };

    trajectory_msgs::msg::JointTrajectoryPoint point;
    point.positions.resize(12);
    point.velocities.resize(12);
    point.effort.resize(12);

    for(int i = 0; i < 12; i++)
    {
        point.positions[i] = joints[i];
        point.velocities[i] = 0.0;
        point.effort[i] = 0.0;
    }

    // At high control rates (>=1000Hz), use immediate execution
    // to avoid timing issues with message transport delays
    point.time_from_start = rclcpp::Duration::from_seconds(0.0);
    joint_msg.points.push_back(point);

    joint_cmd_pub_->publish(joint_msg);
}

void QuadrupedController::publishOdometry(float /*dt*/)
{
    // Update foot contact states (simplified - assume all feet in contact when not moving fast)
    for(int i = 0; i < 4; i++)
    {
        bool in_contact = !in_motion_ || quadruped_.legs[i]->gait_phase();
        quadruped_.legs[i]->in_contact(in_contact);
    }

    // Get velocities from odometry
    champ::Velocities current_velocities;
    odometry_->getVelocities(current_velocities);

    // Publish odometry
    auto odom_msg = nav_msgs::msg::Odometry();
    odom_msg.header.stamp = this->now();
    odom_msg.header.frame_id = "odom";
    odom_msg.child_frame_id = "base_link";

    // Velocity in body frame
    odom_msg.twist.twist.linear.x = current_velocities.linear.x;
    odom_msg.twist.twist.linear.y = current_velocities.linear.y;
    odom_msg.twist.twist.angular.z = current_velocities.angular.z;

    odom_pub_->publish(odom_msg);
}
