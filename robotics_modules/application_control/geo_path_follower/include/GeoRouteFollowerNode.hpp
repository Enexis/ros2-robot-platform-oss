#include "rclcpp/rclcpp.hpp"       // ROS 2 C++ client library
#include "std_msgs/msg/string.hpp" // Standard message type for strings
#include <rclcpp_action/rclcpp_action.hpp>
#include <nav2_msgs/action/follow_waypoints.hpp>
#include <nav2_msgs/action/follow_gps_waypoints.hpp>
#include <string>

using FollowGPSWaypoints = nav2_msgs::action::FollowGPSWaypoints;
class GeoRouteFollowerNode : public rclcpp::Node
{
public:
    GeoRouteFollowerNode();

private:
    std::string filepath_ = "";
    rclcpp_action::Client<FollowGPSWaypoints>::SharedPtr gps_waypoint_follower_action_client_;
    void feedback_callback(
        rclcpp_action::ClientGoalHandle<FollowGPSWaypoints>::SharedPtr,
        const std::shared_ptr<const FollowGPSWaypoints::Feedback> feedback);

    void result_callback(
        const rclcpp_action::ClientGoalHandle<FollowGPSWaypoints>::WrappedResult &result);
};