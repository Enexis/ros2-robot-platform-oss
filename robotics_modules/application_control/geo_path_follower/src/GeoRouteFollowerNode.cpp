#include "rclcpp/rclcpp.hpp"       // ROS 2 C++ client library
#include "std_msgs/msg/string.hpp" // Standard message type for strings
#include <geographic_msgs/msg/geo_pose.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <nav2_msgs/action/follow_waypoints.hpp>
#include <nav2_msgs/action/follow_gps_waypoints.hpp>
#include <vector>
#include <string>

#include "GeoRouteFollowerNode.hpp"
#include "GeoRouteParser.hpp"

using namespace std::chrono_literals;
using ClientT = nav2_msgs::action::FollowGPSWaypoints;

GeoRouteFollowerNode::GeoRouteFollowerNode() : Node("GeoRouteFollower")
{
    this->declare_parameter<std::string>("geoJson_file_path", "");
    this->get_parameter("geoJson_file_path", filepath_);

    gps_waypoint_follower_action_client_ =
        rclcpp_action::create_client<FollowGPSWaypoints>(
            this, "follow_gps_waypoints");

    FollowGPSWaypoints::Goal goal;
    goal.gps_poses = GeoRouteParser::parse(filepath_.c_str());
    RCLCPP_INFO(get_logger(), "Loaded filepath parameter: %s\n", filepath_.c_str());

    if (!gps_waypoint_follower_action_client_->wait_for_action_server(5s))
    {
        RCLCPP_ERROR(get_logger(), "Action server not available");
        rclcpp::shutdown();
        return;
    }

    rclcpp_action::Client<FollowGPSWaypoints>::SendGoalOptions opts;
    opts.feedback_callback =
        std::bind(&GeoRouteFollowerNode::feedback_callback,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2);

    opts.result_callback =
        std::bind(&GeoRouteFollowerNode::result_callback,
                  this,
                  std::placeholders::_1);

    for (const auto &pose : goal.gps_poses)
    {
        RCLCPP_INFO(get_logger(), "lat: %f lon:%f alt:%f\n", pose.position.latitude, pose.position.longitude, pose.position.altitude);
    }
    gps_waypoint_follower_action_client_->async_send_goal(goal, opts);
}

void GeoRouteFollowerNode::feedback_callback(
    rclcpp_action::ClientGoalHandle<FollowGPSWaypoints>::SharedPtr,
    const std::shared_ptr<const FollowGPSWaypoints::Feedback> feedback)
{
    static uint32_t current_waypoint = UINT32_MAX;
    if (current_waypoint != feedback->current_waypoint)
    {
        current_waypoint = feedback->current_waypoint;
        RCLCPP_INFO(
            this->get_logger(),
            "Moving to waypoint: %u",
            feedback->current_waypoint);
    }
}

void GeoRouteFollowerNode::result_callback(
    const rclcpp_action::ClientGoalHandle<FollowGPSWaypoints>::WrappedResult &result)
{
    RCLCPP_INFO(
        this->get_logger(),
        "Result code: %d",
        static_cast<int>(result.code));
    rclcpp::shutdown();
}

#ifndef TESTING_EXCLUDE_MAIN
int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    std::shared_ptr<GeoRouteFollowerNode> gpsWaypointNode = std::make_shared<GeoRouteFollowerNode>();
    rclcpp::spin(gpsWaypointNode);
    rclcpp::shutdown();
    return 0;
}
#endif