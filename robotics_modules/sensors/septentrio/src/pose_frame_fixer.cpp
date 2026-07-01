#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>

class PoseFrameFixer : public rclcpp::Node
{
public:
    PoseFrameFixer()
    : Node("pose_frame_fixer")
    {
        pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "pose/reframed", 10);

        pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "pose", 10,
            std::bind(&PoseFrameFixer::callback, this, std::placeholders::_1));
    }

private:
    void callback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg)
    {
        geometry_msgs::msg::PoseWithCovarianceStamped out_pose = *msg;
        out_pose.header.frame_id = "map";
        pose_pub_->publish(out_pose);
    }
    rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr pose_pub_;
    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr pose_sub_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PoseFrameFixer>());
    rclcpp::shutdown();
    return 0;
}
