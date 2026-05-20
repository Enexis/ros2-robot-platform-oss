#ifndef GO2_QUADRUPED_CONTROLLER_H
#define GO2_QUADRUPED_CONTROLLER_H

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <trajectory_msgs/msg/joint_trajectory.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <unitree_go2_msgs/msg/velocities.hpp>
#include <unitree_go2_msgs/msg/pose.hpp>

#include <champ/quadruped_base/quadruped_base.h>
#include <champ/quadruped_base/quadruped_leg.h>
#include <champ/quadruped_base/quadruped_joint.h>
#include <champ/kinematics/kinematics.h>
#include <go2_quadruped_description.h>
#include <champ/body_controller/body_controller.h>
#include <champ/leg_controller/leg_controller.h>
#include <champ/odometry/odometry.h>

class QuadrupedController : public rclcpp::Node
{
public:
    QuadrupedController();
    
private:
    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg);
    void bodyPoseCallback(const unitree_go2_msgs::msg::Pose::SharedPtr msg);
    void jointStateCallback(const sensor_msgs::msg::JointState::SharedPtr msg);
    void publishJoints(float joints[12]);
    void publishOdometry(float dt);
    void controlLoop();

    // CHAMP components
    champ::QuadrupedBase quadruped_;
    champ::GaitConfig gait_config_;
    champ::BodyController *body_controller_;
    champ::LegController *leg_controller_;
    champ::Kinematics *kinematics_;
    champ::Odometry *odometry_;

    // ROS2 interfaces
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
    rclcpp::Subscription<unitree_go2_msgs::msg::Pose>::SharedPtr body_pose_sub_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;
    
    rclcpp::Publisher<trajectory_msgs::msg::JointTrajectory>::SharedPtr joint_cmd_pub_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    
    rclcpp::TimerBase::SharedPtr control_timer_;

    // State variables
    champ::Velocities req_velocities_;
    champ::Pose req_pose_;
    geometry::Transformation target_foot_positions_[4];
    geometry::Transformation startup_foot_positions_[4];  // Store initial spawn position
    float current_joint_positions_[12];
    bool joint_states_received_;
    bool startup_transition_complete_;
    int startup_blend_counter_;
    
    double control_rate_;
    bool in_motion_;
};

#endif
